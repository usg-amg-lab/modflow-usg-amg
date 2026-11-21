# Release checklist - v0.1.1-beta

Status: Release candidate; public release not yet approved.

## Completed

- [x] Stage 4 clean build with GCC / gfortran 16.2.0
- [x] Binary I/O corrected using ACCESS='STREAM'
- [x] USGS models 01A, 01B and 03 completed
- [x] Stage 3 / Stage 4 HDS SHA-256 comparisons passed
- [x] Tizert calibration: 31 stress periods completed
- [x] Tizert HED and main CCF imported successfully into GMS
- [x] Standalone Windows EXE without external GNU DLLs
- [x] Relinking from 36 application object files demonstrated
- [x] Alternative libquadmath.a selection demonstrated
- [x] Professor Ibaraki's XMD redistribution permission documented
- [x] hypre 2.33.0 license files collected
- [x] GNU runtime and LGPL license texts collected

## Remaining before public release

- [ ] Verify reproducible build from the Git repository
- [ ] Package and verify relinking materials
- [ ] Provide corresponding libquadmath source and notices
- [ ] Review component provenance and distribution rights
- [ ] Select and document license for original project additions
- [ ] Complete additional regression and error-handling tests
- [ ] Verify CBC/CCF budgets against reference results
- [ ] Investigate GMS CCF2 import and repeated Read Solution crash
- [ ] Test standalone EXE on a clean Windows installation
- [ ] Update README and CHANGELOG
- [ ] Verify release archive contents and SHA-256
- [ ] Ensure no proprietary client files enter the public release
- [ ] Final license and release review
- [ ] Commit, tag and publish only after review

## Release artifacts

Planned:

- mfusg_amg.exe - standalone Windows x64 executable
- Relinking package - objects, script and instructions
- Source repository with build and licensing documentation

This project is independent of USGS and is not an
official USGS distribution.
