# Changelog

## v0.1.1-beta - Stage 4 (release candidate)

### Fixed

- Changed binary I/O from sequential unformatted access to
  stream unformatted access in `src/openspec.inc`.
- Removed compiler-generated Fortran record markers from
  binary MODFLOW output files (HED/HDS and CCF/CBC).

### Validation

- USGS regression models 01A, 01B and 03 completed successfully.
- HDS results for all three models were byte-for-byte
  identical to the stored Stage 3 reference files.
- Tizert calibration model completed all 31 stress periods
  using hypre GMRES/BoomerAMG.
- GMS 10.8.9 successfully imported heads and the main CCF
  without binary conversion.
- Verified relinking of the standalone executable from
  36 compiled object files.
- Verified selection of an alternative libquadmath archive
  during linking.

### Distribution

- Prepared standalone Windows x64 executable.
- Added hypre and GNU runtime licensing information.
- Documented permission from Motomu Ibaraki for
  redistribution of xmdlib_2.f.
- Prepared LGPL relinking materials.

### Known limitations

- GMS CCF2 import has not been fully verified.
- GMS crashed during a repeated Read Solution operation;
  the cause has not been established.
- Additional regression, budget and clean-Windows tests
  are required before public release.
- Final licensing and release review remains open.

No intended changes to matrix construction, GMRES
tolerances or BoomerAMG numerical settings.

## Stage 3 RC source (unreleased)

- Make `HYPRE_PREFIX` configurable (no machine-specific absolute path).
- Print original hypre API failure status and operation step
  (diagnostic-only change).
- Add three USGS test fixtures and first automated HDS
  regression runner.
- Add build, validation and license/release checklists.
- Retain frozen Stage 2e EXE for internal comparison.

No intended change to matrix construction, GMRES tolerances
or BoomerAMG numerical settings.
