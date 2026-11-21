! MODFLOW-USG Serial AMG - hypre integration
! Copyright (c) 2026 Jiří Beránek
! SPDX-License-Identifier: MIT
! See licenses/PROJECT_ADDITIONS_MIT.txt
! Thin ISO_C_BINDING interface. No numerical operations or matrix changes.
subroutine amg_hypre_solve(amat,rhs,soln,ia,ja,neqs,nja,icnvg, &
                           in_iter,iter1,hiclose,iout)
  use iso_c_binding, only: c_int, c_double
  use, intrinsic :: ieee_arithmetic, only: ieee_is_finite
  implicit none
  integer, intent(in) :: neqs,nja,ia(neqs+1),ja(nja),iter1,iout
  integer, intent(inout) :: icnvg,in_iter
  real(c_double), intent(in) :: amat(nja),rhs(neqs),hiclose
  real(c_double), intent(inout) :: soln(neqs)
  integer(c_int) :: status, nit
  real(c_double) :: relres, max_abs_res, rel_l2_res, max_head_change, max_abs_rhs
  integer :: i,j
  logical, save :: first=.true.
  interface
    function mfusg_hypre_solve(n,nnz,rowptr,colind,aval,b,x,maxit,tol, &
                                iterations,rel_residual,abs_residual,rel_l2_residual,head_change,abs_rhs) bind(C,name='mfusg_hypre_solve') result(rc)
      import c_int,c_double
      integer(c_int), value :: n,nnz,maxit
      integer(c_int), intent(in) :: rowptr(*),colind(*)
      real(c_double), intent(in) :: aval(*),b(*)
      real(c_double), intent(inout) :: x(*)
      real(c_double), value :: tol
      integer(c_int), intent(out) :: iterations
      real(c_double), intent(out) :: rel_residual,abs_residual,rel_l2_residual,head_change,abs_rhs
      integer(c_int) :: rc
    end function
  end interface
  if (ia(1)/=1 .or. ia(neqs+1)/=nja+1) error stop 'HYPRE: invalid CSR row pointers'
  do i=1,neqs
    if(ia(i+1)<=ia(i)) error stop 'HYPRE: invalid CSR row'
  end do
  do j=1,nja
    if(ja(j)<1 .or. ja(j)>neqs) error stop 'HYPRE: invalid CSR column'
  end do
  if(.not.all(ieee_is_finite(amat))) error stop 'HYPRE: nonfinite matrix'
  if(.not.all(ieee_is_finite(rhs))) error stop 'HYPRE: nonfinite RHS'
  if(.not.all(ieee_is_finite(soln))) error stop 'HYPRE: nonfinite initial guess'
  if(first) then
    write(iout,'(a,i10,a,i12)') 'HYPRE: CSR accepted; NEQS=',neqs,' NJA=',nja
    first=.false.
  end if
  nit=0
  relres=0d0
  status=mfusg_hypre_solve(int(neqs,c_int),int(nja,c_int),ia,ja,amat, &
                           rhs,soln,int(iter1,c_int),max(1d-12,min(1d-9,hiclose)), &
                           nit,relres,max_abs_res,rel_l2_res,max_head_change,max_abs_rhs)
  in_iter=int(nit)
  icnvg=0
  write(iout,'(a,i0,a,i0,a,es12.4)') 'HYPRE: status=',status, &
    ' iterations=',nit,' relative residual=',relres
  if(status==0) then
    write(iout,'(a,es12.4,a,es12.4,a,es12.4,a,es12.4)') &
      'HYPRE DIAG: max|b-Ax|=',max_abs_res, &
      ' ||b-Ax||2/||b||2=',rel_l2_res, &
      ' max|delta_h|=',max_head_change, &
      ' max|b|=',max_abs_rhs
  endif
  if(status/=0) then
    write(iout,*) 'FATAL: HYPRE inner solve did not converge or failed.'
    error stop 42
  endif
end subroutine
