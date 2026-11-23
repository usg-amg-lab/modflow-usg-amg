! MODFLOW-USG AMG v0.2.0 - MPI Fortran interface
! Copyright (c) 2026 Jiri Beranek
! SPDX-License-Identifier: MIT

subroutine mfusg_mpi_begin()
  use iso_c_binding, only: c_int
  implicit none

  integer(c_int) :: rank, status

  interface
    function mpi_autolaunch() bind(C,name="mfusg_mpi_autolaunch") result(rc)
      import c_int
      integer(c_int) :: rc
    end function

    function mpi_initialize() bind(C,name="mfusg_mpi_initialize") result(rc)
      import c_int
      integer(c_int) :: rc
    end function

    function mpi_worker_loop() bind(C,name="mfusg_mpi_worker_loop") result(rc)
      import c_int
      integer(c_int) :: rc
    end function

    function mpi_shutdown() bind(C,name="mfusg_mpi_shutdown") result(rc)
      import c_int
      integer(c_int) :: rc
    end function
  end interface

  status = mpi_autolaunch()
  if (status /= 0) error stop 'MPI launcher failed'

  rank = mpi_initialize()

  if (rank < 0) error stop 'MPI initialization failed'

  if (rank > 0) then
    status = mpi_worker_loop()
    if (status /= 0) error stop 'MPI worker loop failed'

    status = mpi_shutdown()
    if (status /= 0) error stop 'MPI worker shutdown failed'

    stop
  endif

end subroutine mfusg_mpi_begin


subroutine mfusg_mpi_end()
  use iso_c_binding, only: c_int
  implicit none

  integer(c_int) :: status

  interface
    function mpi_shutdown() bind(C,name="mfusg_mpi_shutdown") result(rc)
      import c_int
      integer(c_int) :: rc
    end function
  end interface

  status = mpi_shutdown()
  if (status /= 0) error stop 'MPI shutdown failed'

end subroutine mfusg_mpi_end

! Return nonzero status to PEST if the final convergence flag is false.
! Called only after successful MPI shutdown.
subroutine mfusg_mpi_check_convergence(icnvg)
  implicit none
  integer, intent(in) :: icnvg

  if (icnvg == 0) then
    write(*,'(a)') 'MFUSG MPI: simulation did not converge'
    stop 2
  endif
end subroutine mfusg_mpi_check_convergence
