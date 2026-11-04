# MODFLOW-USG Serial AMG — Stage 3 release-candidate source

**Status: private development repository; NOT a public release.**

Independent experimental extension to USGS MODFLOW-USG 1.5, adding hypre 2.33.0 GMRES + BoomerAMG as `LINMETH=3` to SMS. Original xMD and PCGU branches retained. Serial, static Windows x64 target; no MPI/OpenMP.

Files:

- `src/`: complete source tree with diagnostic-only Stage 3 error reporting modification.
- `make/`: portable makefile (`HYPRE_PREFIX` override).
- `tests/`: 3 USGS sample models and an initial regression harness.
- `docs/`: reproducible build, test plan and release checklist.
- `reference/`: **FROZEN Stage 2e EXE** from user (SHA-256 `b0fa13204709597884a4f4c9eb77d9e7bb4aae64443d8baa3377de131cc4bfc3`); **NOT a Stage 3 validation executable**. For internal comparison; exclude from public source repository/release until licenses finalized.

See `docs/BUILD_WINDOWS.md` and `docs/VALIDATION.md`.

**Known limitations:** initial GMRES relative tolerance is hard-limited to 1e-9 when `HICLOSE` is larger; this is not identical to physical head-change tolerance. This code has been validated by the user on one complex project in prior Stage 2e, but the Stage 3 changes need a new build and further independent validation.

For current licensing and redistribution status see [`PRIVATE_REPOSITORY_STATUS.md`](PRIVATE_REPOSITORY_STATUS.md).
