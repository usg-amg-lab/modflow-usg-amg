# Validation plan for v0.1.0-beta

Status: **Release candidate source; NOT final release**.

Public regression models (original USGS distribution): 01A nested grid without GNC, 01B nested grid with GNC, 03 confined conduit.

`python tests/run_regression.py --exe /path/to/mfusg_amg.exe --json test-results.json`

Runner checks normal termination, exit code, HDS record identities and all finite head values against bundled original USGS outputs, with default max difference 0.10 m. This broad threshold is an initial safety bound, **not** a final accepted accuracy criterion. Tighten per case after test results are reviewed.

Further gates before publishing:

- Test xMD/PCGU unchanged, including a deliberately invalid / non-convergent model; no false successful exit.
- Test both nonlinear Newton and non-Newton model paths.
- Compare CBC terms and water balance, not only HDS.
- Freeze test-results.json and compiler, build flags, hypre version.
- Test transfer on a clean Windows machine; no MSYS2 or MPI dependencies.
- Keep proprietary Tizert inputs and outputs OFF public repository unless expressly authorized.

Stage 2e baseline (prior user validation on Tizert): 31 stress periods, close head and budget match against GMS/SAMG. Changes to source in Stage 3 require fresh tests; Stage 2e evidence cannot be passed off as Stage 3 test results.
