# Licensing

This repository combines several separately licensed pieces. No single license covers every file.

## USGS MODFLOW-USG

The Fortran and C sources that implement MODFLOW-USG 1.5 are a program of the U.S. Geological Survey, version 1.5.00 (27 February 2019). They are a work of the United States Government. This project does not claim a copyright in that code and does not relicense it. See `licenses/USGS-MODFLOW-USG.txt`.

Files with added AMG/MPI calls, including `src/mfusg.f` and `src/glo2sms-u1.f`, remain USGS code apart from those additions.

## AMG/MPI integration

The following additions are licensed under the MIT License, Copyright (c) 2026 Jiří Beránek. The full text is `licenses/PROJECT_ADDITIONS_MIT.txt`.

- `src/amg_hypre.f90`
- `src/amg_hypre_mpi.c`
- `src/amg_mpi_control.c`
- `src/amg_mpi_fortran.f90`
- `src/amg_mpi_launcher.c`
- the AMG/MPI additions in `src/mfusg.f` and `src/glo2sms-u1.f`
- `make/makefile.mpi`

The MIT license does not apply to the USGS routines, HYPRE, the GCC runtimes, winpthreads, or Microsoft MPI.

## HYPRE

Binaries built with `make/makefile.mpi` link HYPRE statically. HYPRE 2.33.0 is licensed under the Apache License 2.0 or the MIT License, at the recipient's option. This repository does not contain the HYPRE source or library. The license texts and notices are in `licenses/hypre/` so a binary built from this tree can carry them.

## GCC runtimes and winpthreads

The verified Windows executable links `libgfortran` statically and ships `libquadmath-0.dll`, `libgcc_s_seh-1.dll`, and `libwinpthread-1.dll`. Those libraries are not in this source tree. Their notices are included because the published Windows binary contains them:

- GCC `libgfortran` and `libgcc`: GPL-3.0-or-later with the GCC Runtime Library Exception 3.1 (`licenses/gcc/`)
- `libquadmath`: GNU Library General Public License, version 2 or later; the shipped text is LGPL 2.1 (`licenses/libquadmath/COPYING.LIB`)
- winpthreads: MinGW-w64 MIT terms and the Lockless Inc. BSD terms (`licenses/winpthreads/COPYING`)

The GCC Runtime Library Exception allows the compiled program to be distributed under the terms of its own modules. It does not place the USGS code or the MIT integration under the GPL.

## Microsoft MPI

Microsoft MPI is not included in this source tree. The Windows release asset includes Microsoft's unmodified runtime installer, `msmpisetup.exe`. Its license is presented by that installer. `licenses/ms-mpi/MPI-Redistributables-TPN.txt` is Microsoft's third-party notice for components in MS-MPI. Microsoft states that the MS-MPI installers may be redistributed with an application: https://www.microsoft.com/en-us/download/details.aspx?id=105289

The Microsoft redistributable terms also require the distributor to protect Microsoft at least as much as those terms do, and they include an indemnification obligation for claims arising from distribution of the application, except where a claim is based solely on the unmodified Microsoft code.
