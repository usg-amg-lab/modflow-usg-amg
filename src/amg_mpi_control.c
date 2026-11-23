/*
 * MODFLOW-USG AMG v0.2.0 - MPI process control
 * Copyright (c) 2026 Jiri Beranek
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "HYPRE.h"
#include "HYPRE_utilities.h"

/* Implemented by the distributed hypre solver. */
int mfusg_hypre_mpi_worker_solve(void);
void mfusg_hypre_mpi_report_timing(void);

static int mpi_initialized_here = 0;
static int hypre_initialized_here = 0;
static int mpi_rank = 0;
static int mpi_size = 1;
static int mpi_shutdown_complete = 0;

/*
 * Fortran STOP may terminate MODFLOW before MPI_Finalize.
 * Abort the MPI job rather than leaving worker ranks waiting.
 */
static void mfusg_mpi_exit_handler(void)
{
    if (!mpi_shutdown_complete && mpi_initialized_here) {
        fprintf(stderr,
                "MFUSG MPI: premature termination; aborting MPI job\n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 30);
    }
}

/* Called once from the main MODFLOW-USG program. */
int mfusg_mpi_initialize(void)
{
    int initialized = 0;

    if (MPI_Initialized(&initialized) != MPI_SUCCESS)
        return -1;

    if (!initialized) {
        if (MPI_Init(NULL, NULL) != MPI_SUCCESS)
            return -2;
        mpi_initialized_here = 1;
    }

    if (MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank) != MPI_SUCCESS)
        return -3;

    if (MPI_Comm_size(MPI_COMM_WORLD, &mpi_size) != MPI_SUCCESS)
        return -4;

    if (HYPRE_Initialize() != 0) {
        MPI_Abort(MPI_COMM_WORLD, 3);
        return -6;
    }
    hypre_initialized_here = 1;

    if (atexit(mfusg_mpi_exit_handler) != 0) {
        MPI_Abort(MPI_COMM_WORLD, 31);
        return -7;
    }

    if (mpi_rank == 0) {
        printf("MFUSG MPI: initialized, processes=%d\n", mpi_size);
        fflush(stdout);
    }

    return mpi_rank;
}

/* Worker ranks wait for commands broadcast by rank 0. */
int mfusg_mpi_worker_loop(void)
{
    int command = 0;

    if (mpi_rank == 0)
        return -1;

    for (;;) {
        if (MPI_Bcast(&command, 1, MPI_INT, 0,
                      MPI_COMM_WORLD) != MPI_SUCCESS)
            return -2;

        if (command == 0)
            break;

        /* Command 1 will later invoke distributed hypre solve. */
        if (command != 1) {
            fprintf(stderr,
                    "MFUSG MPI: rank %d unknown command %d\n",
                    mpi_rank, command);
            return -3;
        }

        /* Participate in the distributed GMRES + BoomerAMG solve. */
        if (mfusg_hypre_mpi_worker_solve() != 0) {
            fprintf(stderr,
                    "MFUSG MPI: rank %d solver failed\n", mpi_rank);
            MPI_Abort(MPI_COMM_WORLD, 4);
            return -4;
        }
    }

    return 0;
}

/* Rank 0 broadcasts shutdown and finalizes MPI. */
extern void mfusg_hypre_mpi_cache_clear(void);

int mfusg_mpi_shutdown(void)
{
    int command = 0;

    if (mpi_rank == 0 && mpi_size > 1) {
        if (MPI_Bcast(&command, 1, MPI_INT, 0,
                      MPI_COMM_WORLD) != MPI_SUCCESS)
            return -1;
    }

    if (mpi_rank == 0)
        mfusg_hypre_mpi_report_timing();

    if (hypre_initialized_here) {
        mfusg_hypre_mpi_cache_clear();
        if (HYPRE_Finalize() != 0)
            return -3;
        hypre_initialized_here = 0;
    }

    if (mpi_initialized_here) {
        if (MPI_Finalize() != MPI_SUCCESS)
            return -2;
    }

    mpi_shutdown_complete = 1;
    return 0;
}

int mfusg_mpi_process_count(void)
{
    return mpi_size;
}
