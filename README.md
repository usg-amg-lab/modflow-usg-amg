# MODFLOW-USG Serial AMG

**Version:** v0.1.1-beta (release candidate)

**Status:** Under validation. Not yet approved for public release.

MODFLOW-USG Serial AMG is an independent extension of
USGS MODFLOW-USG 1.5.00 (27 February 2019).

It integrates the hypre 2.33.0 linear solver library,
using GMRES preconditioned by BoomerAMG.

The new solver is available through LINMETH=3 in the
MODFLOW-USG SMS package. The original xMD and PCGU
solver branches are retained.

## Features

- Serial groundwater flow simulation (no MPI required).
- GMRES with BoomerAMG preconditioning.
- Support for MODFLOW-USG unstructured grids.
- Standalone Windows x64 executable.
- No external GNU runtime DLLs required.
- Binary output compatible with standard MODFLOW readers.
- Original MODFLOW-USG solver options retained.

## Version v0.1.1-beta

This release candidate corrects binary input/output
compatibility with standard MODFLOW readers.

The src/openspec.inc configuration now uses:

    ACCESS='STREAM'
    FORM='UNFORMATTED'

This prevents GNU Fortran record markers from being
included in binary output files.

The change does not modify the GMRES/BoomerAMG
numerical algorithms.

## Validation

Three original USGS test models completed successfully:

- 01A: nested grid without GNC
- 01B: nested grid with GNC
- 03: confined conduit model

All three Stage 4 HDS files were byte-for-byte identical
to their stored Stage 3 reference results.

A private Tizert calibration model completed 31 stress
periods using the hypre solver.

GMS 10.8.9 successfully imported the calculated heads
and main cell-by-cell flow budget without conversion.

Further validation is required, including additional
solver configurations, error handling and GMS CCF2 import.

See docs/VALIDATION.md.

## Repository structure

- src/       MODFLOW-USG source code and AMG integration
- make/      Windows build configuration
- tests/     USGS regression models and test runner
- docs/      Build instructions and validation documentation
- licenses/  Component licensing and redistribution information

## Building from source

See docs/BUILD_WINDOWS.md.

The tested environment uses:

- MSYS2 UCRT64
- GNU Fortran / GCC 16.2.0
- hypre 2.33.0 (serial static build)

## Distribution

The intended Windows distribution consists of one
standalone executable:

    mfusg_amg.exe

Because GNU libquadmath is statically linked, separate
relinking materials are being prepared to support the
requirements of its LGPL license.

See licenses/gnu/LIBQUADMATH.md.

## Licensing

This project combines components with different
licensing and redistribution terms.

- USGS MODFLOW-USG: original attribution retained.
- hypre 2.33.0: MIT / Apache 2.0 licensing.
- XMD (Motomu Ibaraki): redistribution permitted
  subject to preservation of original copyright
  notice and attribution.
- GNU libquadmath: LGPL 2.1 or later.
- Original project additions: MIT license (see licenses/PROJECT_ADDITIONS_MIT.txt).

See the licenses/ directory.

## Known limitations

The GMRES relative tolerance is capped at 1e-9 when
HICLOSE is larger. This is not equivalent to a
physical head-change tolerance.

Further testing is required before the software
is considered production-ready across the full
range of MODFLOW-USG model configurations.

## Disclaimer

This is an independent project. It is not affiliated
with, endorsed by, or maintained by the USGS.

The software is provided for testing and evaluation.
