# MODFLOW-USG-AMG v1.0.0

First public release of the MPI-178 HYPRE solver integration for USGS MODFLOW-USG 1.5.

The Windows executable is the verified MPI-178 binary. It was not rebuilt for this release. GMRES and BoomerAMG are created on every solver call. The HYPRE IJ matrix and both IJ vectors are reused when the sparsity pattern is unchanged. `HYPRE_ParCSRGMRESSetup` runs on every call.

Rank 0 executes MODFLOW. The MPI ranks cooperate in the linear solve.

## Windows package

Asset: `MODFLOW-USG-AMG-Windows-x64.zip`

SHA-256 of the ZIP:

```
4c6f648ed55dd00875120b55d3531701a1fc56986f637a35a47ba9a21faab5b4
```

SHA-256 of `mfusg_amg_mpi.exe` inside the ZIP:

```
da67fe6e96f3550bbed17d7256edbdcd076443c8acbba94f63586b78ec1b3036
```

Install Microsoft MPI runtime v10.1.3 with `redist\msmpisetup.exe` from the archive, or from https://www.microsoft.com/en-us/download/details.aspx?id=105289. The SDK is not required. Keep the executable and the three runtime DLLs in the same directory. See `README.md` for the SMS `AMG` line.

## Tests

- USGS 01A, development machine: `flow.hds` and `flow.cbc` matched the MPI-178 reference byte for byte. The run used 2 MPI processes and exited normally.
- USGS 01A, Lenovo Yoga 7, without MSYS2 or HYPRE: the same Windows package passed the regression test.

## Limitations

- The prebuilt program is Windows x64 only.
- The supported source build is MSYS2 UCRT64 with HYPRE 2.33.0 built with MPI.
- `HICLOSE` does not set the GMRES tolerance. The `AMG` line does.
- Changing BoomerAMG parameters can change results. This release does not change them.
- A persistent GMRES/BoomerAMG variant was tested and was not adopted.
