/*
 * MODFLOW-USG AMG v0.2.0 - Distributed MPI GMRES + BoomerAMG
 * Copyright (c) 2026 Jiri Beranek
 * SPDX-License-Identifier: MIT
 *
 * Rank 0 receives original MODFLOW-USG CSR (1-based).
 * Rows are distributed to MPI ranks, solved with hypre,
 * and the resulting vector is gathered on rank 0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include "HYPRE.h"
#include "HYPRE_IJ_mv.h"
#include "HYPRE_parcsr_ls.h"

_Static_assert(sizeof(HYPRE_Int) == sizeof(int),
               "This bridge requires 32-bit HYPRE_Int");
_Static_assert(sizeof(HYPRE_Complex) == sizeof(double),
               "This bridge requires real double-precision hypre");

#define MPI_CHECK(call) do {                                  \
    int rc_ = (call);                                         \
    if (rc_ != MPI_SUCCESS) {                                 \
        fprintf(stderr, "Rank %d: MPI error at line %d\n",    \
                rank, __LINE__);                              \
        MPI_Abort(MPI_COMM_WORLD, 10);                        \
    }                                                         \
} while (0)

#define HYPRE_CHECK(call) do { \
    int rc_ = (call); \
    if (rc_ != 0) { \
        fprintf(stderr, "Rank %d: hypre error %d at %s:%d in %s\n", \
                rank, rc_, __FILE__, __LINE__, #call); \
        fflush(stderr); \
        MPI_Abort(MPI_COMM_WORLD, 11); \
    } \
} while (0)

/*
 * Release persistent hypre objects on the calling MPI rank.
 * The cache will be implemented in the next optimization step.
 */
static struct {
    HYPRE_IJMatrix mat;
    HYPRE_IJVector vb, vx;
    HYPRE_Int *lengths;
    HYPRE_BigInt *cols;
    int first, last, local_n, local_nnz;
    unsigned long long hits, misses;
} ij_cache = {0};

void mfusg_hypre_mpi_cache_clear(void)
{
    if (ij_cache.vx) {
        int rc = HYPRE_IJVectorDestroy(ij_cache.vx);
        if (rc != 0) {
            fprintf(stderr, "MFUSG MPI: vx cache destroy error %d\n", rc);
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 11);
        }
    }

    if (ij_cache.vb) {
        int rc = HYPRE_IJVectorDestroy(ij_cache.vb);
        if (rc != 0) {
            fprintf(stderr, "MFUSG MPI: vb cache destroy error %d\n", rc);
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 11);
        }
    }

    if (ij_cache.mat) {
        int rc = HYPRE_IJMatrixDestroy(ij_cache.mat);
        if (rc != 0) {
            fprintf(stderr, "MFUSG MPI: IJ cache destroy error %d\n", rc);
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 11);
        }
    }

    free(ij_cache.lengths);
    free(ij_cache.cols);

    ij_cache.mat = NULL;
    ij_cache.vb = NULL;
    ij_cache.vx = NULL;
    ij_cache.lengths = NULL;
    ij_cache.cols = NULL;
    ij_cache.local_n = 0;
    ij_cache.local_nnz = 0;
}

static unsigned long long timing_calls = 0;
static double timing_total = 0.0;
static double timing_scatter = 0.0;
static double timing_setup = 0.0;
static double timing_ij = 0.0;
static double timing_solver_config = 0.0;
static double timing_amg_setup = 0.0;
static double timing_solve = 0.0;
static double timing_gather = 0.0;

void mfusg_hypre_mpi_report_timing(void)
{
    printf("\nMFUSG MPI TIMING SUMMARY (rank 0)\n");
    printf("  Solver calls:       %llu\n", timing_calls);
    printf("  IJ cache hits:      %llu\n", ij_cache.hits);
    printf("  IJ cache misses:    %llu\n", ij_cache.misses);
    printf("  Total solver time:  %.6f s\n", timing_total);
    printf("  CSR distribution:   %.6f s\n", timing_scatter);
    printf("  Hypre setup:        %.6f s\n", timing_setup);
    printf("    IJ matrix/vector: %.6f s\n", timing_ij);
    printf("    Solver config:    %.6f s\n", timing_solver_config);
    printf("    GMRES/AMG setup:  %.6f s\n", timing_amg_setup);
    printf("  GMRES solve:        %.6f s\n", timing_solve);
    printf("  Solution gathering: %.6f s\n", timing_gather);
    fflush(stdout);
}

static void *checked_malloc(size_t bytes, int rank)
{
    void *p = malloc(bytes ? bytes : 1);

    if (!p) {
        fprintf(stderr, "Rank %d: memory allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 12);
    }

    return p;
}

/*
 * Collective solver.
 * Called by rank 0 after broadcasting command 1.
 * Worker ranks enter through mfusg_hypre_mpi_worker_solve().
 */
static int distributed_solve(
    int n, int nnz,
    const int *ia, const int *ja,
    const double *a, const double *b, double *x,
    int maxit, double tol,
    int *iterations, double *relres,
    double *max_abs_res, double *rel_l2_res,
    double *max_head_change, double *max_abs_rhs)
{
    int rank, size;
    int meta[3];
    double tolerance = tol;

    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &size));

    double t0 = MPI_Wtime();
    double t1, t2, t3, t4, t5;

    if (rank == 0) {
        meta[0] = n;
        meta[1] = nnz;
        meta[2] = maxit;
    }

    MPI_CHECK(MPI_Bcast(meta, 3, MPI_INT, 0, MPI_COMM_WORLD));
    MPI_CHECK(MPI_Bcast(&tolerance, 1, MPI_DOUBLE,
                        0, MPI_COMM_WORLD));

    n = meta[0];
    nnz = meta[1];
    maxit = meta[2];

    if (n < size || nnz < n || maxit <= 0 ||
        !isfinite(tolerance) || tolerance <= 0.0) {
        fprintf(stderr, "Rank %d: invalid solver dimensions\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 13);
    }

    int *counts = checked_malloc(size * sizeof(int), rank);
    int *displs = checked_malloc(size * sizeof(int), rank);
    int *nzcounts = checked_malloc(size * sizeof(int), rank);
    int *nzdispls = checked_malloc(size * sizeof(int), rank);

    int position = 0;

    for (int r = 0; r < size; ++r) {
        counts[r] = n / size + (r < n % size);
        displs[r] = position;
        position += counts[r];
    }

    int first = displs[rank];
    int local_n = counts[rank];
    int last = first + local_n - 1;
    int local_nnz = 0;

    int *root_lengths = NULL;

    if (rank == 0) {
        root_lengths = checked_malloc(n * sizeof(int), rank);

        for (int i = 0; i < n; ++i)
            root_lengths[i] = ia[i + 1] - ia[i];

        for (int r = 0; r < size; ++r) {
            nzdispls[r] = ia[displs[r]] - 1;
            nzcounts[r] =
                ia[displs[r] + counts[r]] - ia[displs[r]];
        }
    }

    MPI_CHECK(MPI_Scatter(nzcounts, 1, MPI_INT,
                          &local_nnz, 1, MPI_INT,
                          0, MPI_COMM_WORLD));

    HYPRE_Int *lengths =
        checked_malloc(local_n * sizeof(HYPRE_Int), rank);

    int *local_ja =
        checked_malloc(local_nnz * sizeof(int), rank);

    HYPRE_Complex *local_a =
        checked_malloc(local_nnz * sizeof(HYPRE_Complex), rank);

    double *local_b =
        checked_malloc(local_n * sizeof(double), rank);

    double *local_x =
        checked_malloc(local_n * sizeof(double), rank);

    HYPRE_BigInt *rows =
        checked_malloc(local_n * sizeof(HYPRE_BigInt), rank);

    HYPRE_BigInt *cols =
        checked_malloc(local_nnz * sizeof(HYPRE_BigInt), rank);

    MPI_CHECK(MPI_Scatterv(
        root_lengths, counts, displs, MPI_INT,
        lengths, local_n, MPI_INT, 0, MPI_COMM_WORLD));

    MPI_CHECK(MPI_Scatterv(
        ja, nzcounts, nzdispls, MPI_INT,
        local_ja, local_nnz, MPI_INT, 0, MPI_COMM_WORLD));

    MPI_CHECK(MPI_Scatterv(
        a, nzcounts, nzdispls, MPI_DOUBLE,
        local_a, local_nnz, MPI_DOUBLE, 0, MPI_COMM_WORLD));

    MPI_CHECK(MPI_Scatterv(
        b, counts, displs, MPI_DOUBLE,
        local_b, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD));

    MPI_CHECK(MPI_Scatterv(
        x, counts, displs, MPI_DOUBLE,
        local_x, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD));

    t1 = MPI_Wtime();

    for (int i = 0; i < local_n; ++i)
        rows[i] = (HYPRE_BigInt)(first + i);

    for (int i = 0; i < local_nnz; ++i)
        cols[i] = (HYPRE_BigInt)(local_ja[i] - 1);

    HYPRE_IJMatrix mat = NULL;
    HYPRE_IJVector vb = NULL, vx = NULL;
    HYPRE_ParCSRMatrix parmat = NULL;
    HYPRE_ParVector parb = NULL, parx = NULL;
    HYPRE_Solver gmres = NULL, amg = NULL;

    MPI_Comm comm = MPI_COMM_WORLD;

    int reuse_matrix =
        ij_cache.mat != NULL &&
        ij_cache.first == first &&
        ij_cache.last == last &&
        ij_cache.local_n == local_n &&
        ij_cache.local_nnz == local_nnz &&
        memcmp(ij_cache.lengths, lengths,
               (size_t)local_n * sizeof(HYPRE_Int)) == 0 &&
        memcmp(ij_cache.cols, cols,
               (size_t)local_nnz * sizeof(HYPRE_BigInt)) == 0;

    if (!reuse_matrix) {
        mfusg_hypre_mpi_cache_clear();

        HYPRE_CHECK(HYPRE_IJMatrixCreate(
            comm, first, last, first, last, &mat));

        HYPRE_CHECK(HYPRE_IJMatrixSetObjectType(
            mat, HYPRE_PARCSR));

        HYPRE_CHECK(HYPRE_IJMatrixSetRowSizes(mat, lengths));

        ij_cache.mat = mat;
        ij_cache.first = first;
        ij_cache.last = last;
        ij_cache.local_n = local_n;
        ij_cache.local_nnz = local_nnz;

        ij_cache.lengths =
            checked_malloc((size_t)local_n * sizeof(HYPRE_Int), rank);

        ij_cache.cols =
            checked_malloc((size_t)local_nnz * sizeof(HYPRE_BigInt), rank);

        memcpy(ij_cache.lengths, lengths,
               (size_t)local_n * sizeof(HYPRE_Int));

        memcpy(ij_cache.cols, cols,
               (size_t)local_nnz * sizeof(HYPRE_BigInt));

        ++ij_cache.misses;
    } else {
        mat = ij_cache.mat;
        ++ij_cache.hits;
    }

    HYPRE_CHECK(HYPRE_IJMatrixInitialize(mat));

    HYPRE_CHECK(HYPRE_IJMatrixSetValues(
        mat, local_n, lengths, rows, cols, local_a));

    HYPRE_CHECK(HYPRE_IJMatrixAssemble(mat));
    HYPRE_CHECK(HYPRE_IJMatrixGetObject(
        mat, (void **)&parmat));

    if (!reuse_matrix) {
        HYPRE_CHECK(HYPRE_IJVectorCreate(
            comm, first, last, &vb));
        HYPRE_CHECK(HYPRE_IJVectorSetObjectType(
            vb, HYPRE_PARCSR));

        ij_cache.vb = vb;

        HYPRE_CHECK(HYPRE_IJVectorCreate(
            comm, first, last, &vx));
        HYPRE_CHECK(HYPRE_IJVectorSetObjectType(
            vx, HYPRE_PARCSR));

        ij_cache.vx = vx;
    } else {
        vb = ij_cache.vb;
        vx = ij_cache.vx;
    }

    /* Refresh both vectors for every linear solve. */
    HYPRE_CHECK(HYPRE_IJVectorInitialize(vb));
    HYPRE_CHECK(HYPRE_IJVectorSetValues(
        vb, local_n, rows, local_b));
    HYPRE_CHECK(HYPRE_IJVectorAssemble(vb));
    HYPRE_CHECK(HYPRE_IJVectorGetObject(
        vb, (void **)&parb));

    HYPRE_CHECK(HYPRE_IJVectorInitialize(vx));
    HYPRE_CHECK(HYPRE_IJVectorSetValues(
        vx, local_n, rows, local_x));
    HYPRE_CHECK(HYPRE_IJVectorAssemble(vx));
    HYPRE_CHECK(HYPRE_IJVectorGetObject(
        vx, (void **)&parx));

    double t_ij = MPI_Wtime();

    HYPRE_CHECK(HYPRE_BoomerAMGCreate(&amg));
    HYPRE_CHECK(HYPRE_BoomerAMGSetPrintLevel(amg, 0));
    HYPRE_CHECK(HYPRE_BoomerAMGSetMaxIter(amg, 1));
    HYPRE_CHECK(HYPRE_BoomerAMGSetTol(amg, 0.0));

    HYPRE_CHECK(HYPRE_ParCSRGMRESCreate(comm, &gmres));
    HYPRE_CHECK(HYPRE_ParCSRGMRESSetMaxIter(gmres, maxit));
    HYPRE_CHECK(HYPRE_ParCSRGMRESSetKDim(gmres, 50));
    HYPRE_CHECK(HYPRE_ParCSRGMRESSetTol(gmres, tolerance));
    HYPRE_CHECK(HYPRE_ParCSRGMRESSetLogging(gmres, 1));
    HYPRE_CHECK(HYPRE_ParCSRGMRESSetPrintLevel(gmres, 0));

    HYPRE_CHECK(HYPRE_ParCSRGMRESSetPrecond(
        gmres, HYPRE_BoomerAMGSolve,
        HYPRE_BoomerAMGSetup, amg));

    double t_config = MPI_Wtime();

    HYPRE_CHECK(HYPRE_ParCSRGMRESSetup(
        gmres, parmat, parb, parx));

    t2 = MPI_Wtime();

    HYPRE_CHECK(HYPRE_ParCSRGMRESSolve(
        gmres, parmat, parb, parx));

    t3 = MPI_Wtime();

    HYPRE_Int its = 0;
    HYPRE_Real residual = 0.0;

    HYPRE_CHECK(HYPRE_ParCSRGMRESGetNumIterations(
        gmres, &its));

    HYPRE_CHECK(HYPRE_ParCSRGMRESGetFinalRelativeResidualNorm(
        gmres, &residual));

    HYPRE_CHECK(HYPRE_IJVectorGetValues(
        vx, local_n, rows, local_x));

    /*
     * Gather into a temporary array, preserving the original
     * MODFLOW initial guess for head-change diagnostics.
     */
    double *solution = NULL;

    if (rank == 0)
        solution = checked_malloc(n * sizeof(double), rank);

    MPI_CHECK(MPI_Gatherv(
        local_x, local_n, MPI_DOUBLE,
        solution, counts, displs, MPI_DOUBLE,
        0, MPI_COMM_WORLD));

    t4 = MPI_Wtime();

    int status = 0;

    if (rank == 0) {
        *iterations = (int)its;
        *relres = (double)residual;

        if (!isfinite(*relres) || *relres > tolerance * 1.05)
            status = -156;

        long double r2 = 0.0L, b2 = 0.0L;
        double rmax = 0.0, bmax = 0.0, dxmax = 0.0;

        if (status == 0) {
            for (int i = 0; i < n; ++i) {
                long double ax = 0.0L;

                if (!isfinite(solution[i])) {
                    status = -158;
                    break;
                }

                for (int j = ia[i] - 1; j < ia[i + 1] - 1; ++j)
                    ax += (long double)a[j] *
                          (long double)solution[ja[j] - 1];

                double ri = (double)((long double)b[i] - ax);

                if (!isfinite(ri)) {
                    status = -159;
                    break;
                }

                if (fabs(ri) > rmax) rmax = fabs(ri);
                if (fabs(b[i]) > bmax) bmax = fabs(b[i]);
                if (fabs(solution[i] - x[i]) > dxmax)
                    dxmax = fabs(solution[i] - x[i]);

                r2 += (long double)ri * (long double)ri;
                b2 += (long double)b[i] * (long double)b[i];
            }
        }

        if (status == 0) {
            *max_abs_res = rmax;
            *max_abs_rhs = bmax;
            *max_head_change = dxmax;
            *rel_l2_res = (double)(
                sqrtl(r2) / (b2 > 0.0L ? sqrtl(b2) : 1.0L));

            memcpy(x, solution, n * sizeof(double));
        }
    }

    MPI_CHECK(MPI_Bcast(
        &status, 1, MPI_INT, 0, MPI_COMM_WORLD));

    HYPRE_CHECK(HYPRE_ParCSRGMRESDestroy(gmres));
    HYPRE_CHECK(HYPRE_BoomerAMGDestroy(amg));
    /* IJ vectors are retained until cache invalidation or shutdown. */
    /* IJMatrix is retained until cache invalidation or MPI shutdown. */

    free(solution);
    free(counts);
    free(displs);
    free(nzcounts);
    free(nzdispls);
    free(root_lengths);
    free(lengths);
    free(local_ja);
    free(local_a);
    free(local_b);
    free(local_x);
    free(rows);
    free(cols);

    t5 = MPI_Wtime();

    if (rank == 0) {
        ++timing_calls;
        timing_total += t5 - t0;
        timing_scatter += t1 - t0;
        timing_setup += t2 - t1;
        timing_ij += t_ij - t1;
        timing_solver_config += t_config - t_ij;
        timing_amg_setup += t2 - t_config;
        timing_solve += t3 - t2;
        timing_gather += t4 - t3;
    }

    return status;
}

/* Existing Fortran ABI. Called only by MODFLOW-USG rank 0. */
int mfusg_hypre_solve(
    int n, int nnz, const int *ia, const int *ja,
    const double *a, const double *b, double *x,
    int maxit, double tol,
    int *iterations, double *relres,
    double *max_abs_res, double *rel_l2_res,
    double *max_head_change, double *max_abs_rhs)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank != 0) {
        fprintf(stderr, "Worker called rank-0 solver entry\n");
        MPI_Abort(MPI_COMM_WORLD, 20);
    }

    *iterations = 0;
    *relres = NAN;
    *max_abs_res = NAN;
    *rel_l2_res = NAN;
    *max_head_change = NAN;
    *max_abs_rhs = NAN;

    int valid = n > 0 && nnz >= n && maxit > 0 &&
                isfinite(tol) && tol > 0.0 &&
                ia && ja && a && b && x;

    if (valid)
        valid = ia[0] == 1 && ia[n] == nnz + 1;

    if (valid) {
        for (int i = 0; i < n; ++i)
            if (ia[i + 1] <= ia[i])
                valid = 0;

        for (int j = 0; j < nnz; ++j)
            if (ja[j] < 1 || ja[j] > n || !isfinite(a[j]))
                valid = 0;

        for (int i = 0; i < n; ++i)
            if (!isfinite(b[i]) || !isfinite(x[i]))
                valid = 0;
    }

    if (!valid) {
        fprintf(stderr, "MFUSG MPI: invalid CSR input\n");
        MPI_Abort(MPI_COMM_WORLD, 21);
        return -1;
    }

    if (size > 1) {
        int command = 1;
        MPI_CHECK(MPI_Bcast(
            &command, 1, MPI_INT, 0, MPI_COMM_WORLD));
    }

    return distributed_solve(
        n, nnz, ia, ja, a, b, x, maxit, tol,
        iterations, relres, max_abs_res, rel_l2_res,
        max_head_change, max_abs_rhs);
}

/* Called by worker ranks after receiving command 1. */
int mfusg_hypre_mpi_worker_solve(void)
{
    return distributed_solve(
        0, 0, NULL, NULL, NULL, NULL, NULL,
        0, 0.0, NULL, NULL, NULL, NULL, NULL, NULL);
}
