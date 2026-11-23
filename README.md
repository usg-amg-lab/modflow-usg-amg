# MODFLOW-USG-AMG

MODFLOW-USG-AMG is a Windows build of MODFLOW-USG 1.5 with a HYPRE linear solver. For `LINMETH = 3` the program solves the unstructured flow matrix with GMRES and BoomerAMG preconditioning. MPI is used for that linear solve.

This repository is version 1.0.0. The solver integration is the verified MPI-178 implementation: HYPRE IJ matrix and IJ vector objects are reused when the sparsity pattern is unchanged, and GMRES and BoomerAMG are created for each solver call.

## Relationship to USGS MODFLOW-USG

The groundwater model is USGS MODFLOW-USG, version 1.5.00 (27 February 2019). The numerical routines of that program are unchanged. This project adds the HYPRE/MPI solver path, the Windows launcher, and the makefile that links them. Those additions are under the MIT License. The USGS code is not relicensed. See `LICENSING.md`.

## What runs in parallel

Rank 0 reads the model, assembles the equations, and writes the results. The other MPI ranks take part in the HYPRE solve. Package formulation, Newton updates, budgets, and output stay on rank 0. MPI does not make the rest of the MODFLOW simulation parallel.

On each solver call the integration distributes the CSR matrix and the vectors, writes the current values into HYPRE, calls `HYPRE_ParCSRGMRESSetup`, and solves. If the row partition and the sparsity pattern match the previous call, the IJ matrix and both IJ vectors are kept and their values are replaced. A different pattern creates new IJ objects.

A variant that also kept the GMRES and BoomerAMG objects was tested. It was not faster in that test, and it is not this release. BoomerAMG parameters are the ones used for the verified binary. Changing them can change the numerical results.

## Requirements

The published program is a 64-bit Windows application.

- Windows 10 or Windows 11, x64, for the prebuilt executable.
- Microsoft MPI runtime v10.1.3 (file version 10.1.12498.52). The SDK is not required to run the program.
- A groundwater model whose SMS file selects this solver, as described below.

Building from source was verified with MSYS2 UCRT64, GCC 16.2.0, and HYPRE 2.33.0 built with MPI. Other compilers, HYPRE versions, and operating systems are not covered by this release.

## Windows binary

Download `MODFLOW-USG-AMG-Windows-x64.zip` from the GitHub Release v1.0.0. The archive is not part of this source tree.

SHA-256 of the archive:

```
4c6f648ed55dd00875120b55d3531701a1fc56986f637a35a47ba9a21faab5b4
```

The executable inside it is the verified MPI-178 binary, unmodified:

```
da67fe6e96f3550bbed17d7256edbdcd076443c8acbba94f63586b78ec1b3036
```

Unzip the archive and leave these files in one directory:

- `mfusg_amg_mpi.exe`
- `libquadmath-0.dll`
- `libgcc_s_seh-1.dll`
- `libwinpthread-1.dll`

Windows loads the three DLLs from that directory. An MSYS2 installation is not required, and `C:\msys64` does not need to be on `PATH`.

### Microsoft MPI runtime

Install the runtime once per machine. From the unpacked archive run:

```
redist\msmpisetup.exe
```

The same installer is published by Microsoft as `msmpisetup.exe` for MS-MPI v10.1.3:

https://www.microsoft.com/en-us/download/details.aspx?id=105289

Direct link:

https://download.microsoft.com/download/7/2/7/72731ebb-b63c-4170-ade7-836966263a8f/msmpisetup.exe

Do not install `msmpisdk.msi` unless you are compiling against the MPI headers. The installer copies `mpiexec.exe` to `C:\Program Files\Microsoft MPI\Bin` and `msmpi.dll` to the Windows system directory. Running it displays Microsoft's license terms.

### Run a model

In the model directory:

```
path\to\MODFLOW-USG-AMG-Windows-x64\mfusg_amg_mpi.exe model.nam
```

`run-mfusg.cmd` runs the same executable and reports a missing MPI runtime. If the `AMG` line requests more than one process, the executable starts `mpiexec` itself and returns that process's exit code. The parent process does not open the model files. You may instead launch it with `mpiexec -n N`. `N` must be the process count on the `AMG` line.

## SMS configuration

The solver option line is:

```
HCLOSE HICLOSE MXITER ITER1 IPRSMS NONLINMETH LINMETH
```

`LINMETH` 3 selects HYPRE GMRES with BoomerAMG. The following line is read only for that choice:

```
AMG <processes> <gmres_rtol>
```

Example:

```
0.01 0.1 500 100 0 2 3
AMG 8 1.0e-11
```

- `<processes>` is an integer from 1 to 256 and is the MPI process count.
- `<gmres_rtol>` is the relative tolerance passed to GMRES, from `1e-15` to `1e-2`.
- `ITER1` is the GMRES iteration limit.
- `HCLOSE` remains the outer head closure used by SMS.
- `HICLOSE` is not passed to GMRES.

Without an `AMG` line the integration uses 1 process and a GMRES tolerance of `1e-9`. For one process the executable does not start `mpiexec`.

## Build from source

The verified build environment is an MSYS2 UCRT64 shell with `gfortran`, GNU Make, and the package `mingw-w64-ucrt-x86_64-msmpi`. That package provides `mpicc` and `/ucrt64/lib/libmsmpi.dll.a`, which the makefile uses. HYPRE is not included. Install HYPRE 2.33.0 yourself as a static library built with MPI (`HYPRE_HAVE_MPI`), then from `make/`:

```
make -f makefile.mpi -j1 HYPRE_PREFIX=/path/to/hypre-2.33.0/install-mpi
```

`HYPRE_PREFIX` is required. A serial HYPRE library will not link or run this solver. The makefile links `libHYPRE.a`, `libgfortran`, and `libgcc` statically and links MS-MPI through its import library. Objects go to `make/obj_mpi/`, which is gitignored. A new link of these sources can differ from the published executable in the PE timestamp and checksum even when the code is the same. The executable attached to the v1.0.0 release is the verified binary, not a fresh local link.

## Tests and limitations

On the development machine, USGS test 01A produced `flow.hds` and `flow.cbc` byte for byte equal to the MPI-178 reference when run with this executable. The same Windows package later passed the 01A regression test on a separate Lenovo Yoga 7 computer that did not have MSYS2 or HYPRE installed.

This release does not claim a speedup, a Linux or macOS port, or compatibility with other HYPRE versions. BoomerAMG options are unchanged from the verified build. The Windows package still requires the Microsoft MPI runtime. Source builds require the MSYS2 UCRT64 toolchain described above.
