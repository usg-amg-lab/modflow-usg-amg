# Validation of MODFLOW-USG Serial AMG v0.1.1-beta

Status: Release candidate under validation.
Not yet approved for public release.

## Build environment

- Windows x64, MSYS2 UCRT64
- GNU Fortran / GCC 16.2.0-6
- hypre 2.33.0, serial static build
- MODFLOW-USG 1.5.00

## Stage 4 change

The only functional source modification relative to Stage 3
is in src/openspec.inc:

    ACCESS='SEQUENTIAL' -> ACCESS='STREAM'

FORM='UNFORMATTED' remains unchanged.

This removes Fortran record markers from binary MODFLOW
output files.

## Public USGS regression tests

Three models completed with normal termination:

- 01A: nested grid without GNC
- 01B: nested grid with GNC
- 03: confined conduit model

The Stage 4 HDS outputs were byte-for-byte identical
to the stored Stage 3 reference HDS files for all
three models, verified by SHA-256.

These tests establish output equivalence for the
specified models, not universal numerical equivalence.

## Tizert calibration model

Private test model; model files are not included
in the public repository.

- 31 stress periods completed
- Runtime: 58.518 seconds
- Normal termination
- HYPRE calls reported status=0 in the inspected log
- HED, CCF and CCF2 files generated
- GMS successfully imported heads and the main CCF
- GMS displayed the flow budget successfully

The secondary CCF2 was not shown in the imported
solution tree. GMS also crashed during a subsequent
Read Solution attempt.

Further CCF2 import testing is required.

## Relinking tests

The executable was successfully relinked from
36 existing application object files without
recompiling the source code.

Relinking was also demonstrated using a separately
selected copy of libquadmath.a.

USGS model 01A completed successfully with the
relinked executable and produced an HDS file
byte-for-byte identical to the Stage 4 reference.

The replacement library was an unmodified copy.
Relinking with a genuinely modified LGPL library
has not yet been demonstrated.

## Additional validation required

- Verify water budgets and CBC/CCF results against
  established reference values.
- Exercise XMD and PCGU solver paths.
- Test expected failures and non-convergence handling.
- Test additional nonlinear model configurations.
- Verify behavior on a clean Windows installation.
- Verify that the public repository itself can be
  rebuilt using the documented instructions.
- Resolve the GMS CCF2 import question.

Successful completion of these tests is required
before presenting the release as fully validated.

This project is not affiliated with or endorsed by USGS.
