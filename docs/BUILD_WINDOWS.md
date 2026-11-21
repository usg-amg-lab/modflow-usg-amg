# Building MODFLOW-USG Serial AMG on Windows

## Version

MODFLOW-USG Serial AMG v0.1.1-beta
Based on USGS MODFLOW-USG 1.5.00 (27 February 2019).

## Build environment

- Windows x64
- MSYS2 UCRT64
- GCC / GNU Fortran 16.2.0 (MSYS2 packages 16.2.0-6)
- MinGW-w64 CRT 14.0.0
- GNU Make
- CMake and Ninja
- hypre 2.33.0

## Build hypre

Build hypre 2.33.0 as a serial static library:

    cmake -S /path/to/hypre-2.33.0/src \
      -B /path/to/hypre-2.33.0/build-serial \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DHYPRE_ENABLE_MPI=OFF \
      -DHYPRE_ENABLE_OPENMP=OFF \
      -DHYPRE_ENABLE_DSUPERLU=OFF \
      -DHYPRE_ENABLE_HYPRE_BLAS=ON \
      -DHYPRE_ENABLE_HYPRE_LAPACK=ON

    cmake --build /path/to/hypre-2.33.0/build-serial -j 4

    cmake --install /path/to/hypre-2.33.0/build-serial \
      --prefix /path/to/hypre-install-serial

## Build MODFLOW-USG Serial AMG

From the repository root:

    cd make
    make clean
    make -j1 HYPRE_PREFIX=/path/to/hypre-install-serial

The executable is created as make/mfusg_amg.exe.

Do not use parallel make for MODFLOW-USG because the
original Fortran module dependencies are incomplete.

## Binary input/output compatibility

Version v0.1.1-beta uses these settings in src/openspec.inc:

    ACCESS='STREAM'
    FORM='UNFORMATTED'

This prevents GNU Fortran sequential unformatted record
markers from appearing in MODFLOW binary output files.

Binary HED, CCF and CCF2 outputs were generated successfully
during the Tizert calibration run.

GMS successfully imported heads and the main cell-by-cell
budget without conversion.

Import of CCF2 and repeated Read Solution behavior in GMS
remain subject to further testing.

## Windows runtime

The executable is statically linked with hypre and
GNU runtime libraries.

Check DLL dependencies using:

    objdump -p mfusg_amg.exe

No external MSYS2 DLLs should be required.

## LGPL relinking

GNU libquadmath is statically linked and subject to
LGPL 2.1 or later.

See licenses/gnu/LIBQUADMATH.md and the separate
relinking package supplied with the release.

## Validation

See docs/VALIDATION.md.

This is an independent extension and is not an official
USGS distribution.
