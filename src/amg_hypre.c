/* MODFLOW-USG Serial AMG - hypre integration
 * Copyright (c) 2026 Jiří Beránek
 * SPDX-License-Identifier: MIT
 * See licenses/PROJECT_ADDITIONS_MIT.txt
 */
/* Experimental Windows UCRT64: HYPRE 2.33.0 serial configuration.
 * C ABI uses 32-bit Fortran indices, double precision values.
 * Building with -DAMG_TEST_STUB provides an intentional fail-closed stub
 * for testing only the MODFLOW <-> C boundary (NOT an AMG solver).
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef AMG_TEST_STUB
int mfusg_hypre_solve(int n,int nnz,const int *ia,const int *ja,
 const double *a,const double *b,double *x,int maxit,double tol,
 int *iterations,double *relres,
 double *max_abs_res,double *rel_l2_res,double *max_head_change,double *max_abs_rhs)
{
 (void)n;(void)nnz;(void)ia;(void)ja;(void)a;(void)b;(void)x;
 (void)maxit;(void)tol;
 (void)max_abs_res;(void)rel_l2_res;(void)max_head_change;(void)max_abs_rhs;
 *iterations=0; *relres=NAN;
 fprintf(stderr,"HYPRE TEST STUB: no solver linked; refusing to compute.\n");
 return -999;
}
#else
#include "HYPRE.h"
#include "HYPRE_IJ_mv.h"
#include "HYPRE_parcsr_ls.h"
#include "HYPRE_config.h"
/* Serial hypre 2.33.0, built with HYPRE_ENABLE_MPI=OFF.
 * Use the headers installed by THAT build (HYPRE_SEQUENTIAL).
 * The serial API provides a local MPI_Comm typedef and communicator 0;
 * do not include mpi.h or link msmpi.dll.
 */
static int mfusg_hypre_initialized = 0;
static int mfusg_cleanup_registered = 0;

static void mfusg_hypre_shutdown(void)
{
  if (mfusg_hypre_initialized) {
    HYPRE_Finalize();
    mfusg_hypre_initialized = 0;
  }
}

static int mfusg_hypre_startup(void)
{
  if (!mfusg_cleanup_registered) {
    if (atexit(mfusg_hypre_shutdown)!=0) return -204;
    mfusg_cleanup_registered=1;
  }
  if (!mfusg_hypre_initialized) {
    if (HYPRE_Initialize()!=0) return -205;
    mfusg_hypre_initialized=1;
  }
  return 0;
}

#define CHECK(call) do { if ((err=(call))!=0) { \
  fprintf(stderr,"MFUSG-AMG: hypre API error at step %d: status %d\n",line,err); \
  status=-100-line; goto cleanup; \
} } while(0)
int mfusg_hypre_solve(int n,int nnz,const int *ia,const int *ja,
 const double *a,const double *b,double *x,int maxit,double tol,
 int *iterations,double *relres,
 double *max_abs_res,double *rel_l2_res,double *max_head_change,double *max_abs_rhs)
{
 int status=-1,err=0,line=0,i,j;
 HYPRE_IJMatrix mat=NULL;
 HYPRE_IJVector vb=NULL,vx=NULL;
 HYPRE_ParCSRMatrix parmat=NULL;
 HYPRE_ParVector parb=NULL,parx=NULL;
 HYPRE_Solver gmres=NULL,amg=NULL;
 HYPRE_BigInt *rows=NULL,*cols=NULL;
 HYPRE_Int *ncols=NULL;
 double *scratch=NULL;
 HYPRE_Int its=0;
 HYPRE_Real rnorm=0.0;
 MPI_Comm comm=0; /* serial hypre communicator (HYPRE_ENABLE_MPI=OFF) */
 *iterations=0; *relres=NAN;
 *max_abs_res=NAN; *rel_l2_res=NAN; *max_head_change=NAN; *max_abs_rhs=NAN;
 if(n<=0 || nnz<n || maxit<=0 || !(tol>0.0)) return -1;
 if(ia[0]!=1 || ia[n]!=nnz+1) return -2;
 rows=malloc((size_t)n*sizeof(*rows));
 cols=malloc((size_t)nnz*sizeof(*cols));
 ncols=malloc((size_t)n*sizeof(*ncols));
 scratch=malloc((size_t)n*sizeof(*scratch));
 if(!rows||!cols||!ncols||!scratch) {status=-3;goto cleanup;}
 for(i=0;i<n;i++) {
   if(ia[i+1]<=ia[i]) {status=-4;goto cleanup;}
   rows[i]=(HYPRE_BigInt)i;
   ncols[i]=(HYPRE_Int)(ia[i+1]-ia[i]);
 }
 for(j=0;j<nnz;j++) {
   if(ja[j]<1||ja[j]>n||!isfinite(a[j])) {status=-5;goto cleanup;}
   cols[j]=(HYPRE_BigInt)(ja[j]-1);
 }
 for(i=0;i<n;i++) if(!isfinite(x[i])||!isfinite(b[i])) {status=-6;goto cleanup;}
 /* Initialize serial hypre once per process. */
 status=mfusg_hypre_startup();
 if(status!=0) goto cleanup;
 /* hypre defaults to host execution; CPU single-rank execution. */
 line=11; CHECK(HYPRE_IJMatrixCreate(comm,0,n-1,0,n-1,&mat));
 line=12; CHECK(HYPRE_IJMatrixSetObjectType(mat,HYPRE_PARCSR));
 line=13; CHECK(HYPRE_IJMatrixSetRowSizes(mat,ncols));
 line=14; CHECK(HYPRE_IJMatrixInitialize(mat));
 line=15; CHECK(HYPRE_IJMatrixSetValues(mat,n,ncols,rows,cols,a));
 line=16; CHECK(HYPRE_IJMatrixAssemble(mat));
 line=17; CHECK(HYPRE_IJMatrixGetObject(mat,(void**)&parmat));
 line=20; CHECK(HYPRE_IJVectorCreate(comm,0,n-1,&vb));
 line=21; CHECK(HYPRE_IJVectorSetObjectType(vb,HYPRE_PARCSR));
 line=22; CHECK(HYPRE_IJVectorInitialize(vb));
 line=23; CHECK(HYPRE_IJVectorSetValues(vb,n,rows,b));
 line=24; CHECK(HYPRE_IJVectorAssemble(vb));
 line=25; CHECK(HYPRE_IJVectorGetObject(vb,(void**)&parb));
 line=30; CHECK(HYPRE_IJVectorCreate(comm,0,n-1,&vx));
 line=31; CHECK(HYPRE_IJVectorSetObjectType(vx,HYPRE_PARCSR));
 line=32; CHECK(HYPRE_IJVectorInitialize(vx));
 line=33; CHECK(HYPRE_IJVectorSetValues(vx,n,rows,x));
 line=34; CHECK(HYPRE_IJVectorAssemble(vx));
 line=35; CHECK(HYPRE_IJVectorGetObject(vx,(void**)&parx));
 line=40; CHECK(HYPRE_BoomerAMGCreate(&amg));
 line=41; CHECK(HYPRE_BoomerAMGSetPrintLevel(amg,0));
 line=42; CHECK(HYPRE_BoomerAMGSetMaxIter(amg,1));
 line=43; CHECK(HYPRE_BoomerAMGSetTol(amg,0.0));
 line=45; CHECK(HYPRE_ParCSRGMRESCreate(comm,&gmres));
 line=46; CHECK(HYPRE_ParCSRGMRESSetMaxIter(gmres,maxit));
 line=47; CHECK(HYPRE_ParCSRGMRESSetKDim(gmres,50));
 line=48; CHECK(HYPRE_ParCSRGMRESSetTol(gmres,tol));
 line=49; CHECK(HYPRE_ParCSRGMRESSetLogging(gmres,1));
 line=50; CHECK(HYPRE_ParCSRGMRESSetPrintLevel(gmres,0));
 line=51; CHECK(HYPRE_ParCSRGMRESSetPrecond(gmres,
                            HYPRE_BoomerAMGSolve,HYPRE_BoomerAMGSetup,amg));
 line=52; CHECK(HYPRE_ParCSRGMRESSetup(gmres,parmat,parb,parx));
 line=53; err=HYPRE_ParCSRGMRESSolve(gmres,parmat,parb,parx);
 if(err) {fprintf(stderr,"MFUSG-AMG: GMRES solve failed (hypre status %d)\n",err);status=-153;goto cleanup;}
 line=54; CHECK(HYPRE_ParCSRGMRESGetNumIterations(gmres,&its));
 line=55; CHECK(HYPRE_ParCSRGMRESGetFinalRelativeResidualNorm(gmres,&rnorm));
 *iterations=(int)its;
 *relres=(double)rnorm;
 if(!isfinite((double)rnorm) || rnorm>tol*1.05) {status=-156;goto cleanup;}
 line=57; CHECK(HYPRE_IJVectorGetValues(vx,n,rows,scratch));
 for(i=0;i<n;i++) if(!isfinite(scratch[i])) {status=-158;goto cleanup;}
 /* Independent post-solve check of the ORIGINAL MODFLOW CSR equation.
  * Diagnostics ONLY: no change to the numerical solution or tolerance.
  * Use long double for accumulation to reduce cancellation error.
  */
 {
   long double r2=0.0L, b2=0.0L;
   double rmax=0.0, bmax=0.0, dxmax=0.0;
   for(i=0;i<n;i++) {
     long double ax=0.0L;
     double ri;
     for(j=ia[i]-1;j<ia[i+1]-1;j++) {
       ax+=(long double)a[j]*(long double)scratch[ja[j]-1];
     }
     ri=(double)((long double)b[i]-ax);
     if(!isfinite(ri)) {status=-159;goto cleanup;}
     if(fabs(ri)>rmax) rmax=fabs(ri);
     if(fabs(b[i])>bmax) bmax=fabs(b[i]);
     if(fabs(scratch[i]-x[i])>dxmax) dxmax=fabs(scratch[i]-x[i]);
     r2+=(long double)ri*(long double)ri;
     b2+=(long double)b[i]*(long double)b[i];
   }
   *max_abs_res=rmax;
   *max_abs_rhs=bmax;
   *max_head_change=dxmax;
   *rel_l2_res=(double)(sqrtl(r2)/(b2>0.0L?sqrtl(b2):1.0L));
 }
 memcpy(x,scratch,(size_t)n*sizeof(double));
 status=0;
cleanup:
 if(gmres) HYPRE_ParCSRGMRESDestroy(gmres);
 if(amg) HYPRE_BoomerAMGDestroy(amg);
 if(vx) HYPRE_IJVectorDestroy(vx);
 if(vb) HYPRE_IJVectorDestroy(vb);
 if(mat) HYPRE_IJMatrixDestroy(mat);
 free(rows);free(cols);free(ncols);free(scratch);
 return status;
}
#endif
