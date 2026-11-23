# Third-party notices

MODFLOW-USG-AMG v1.0.0 publishes source in this repository and a separate Windows x64 binary on the GitHub Release. The source tree does not vendor HYPRE, MSYS2, MS-MPI, or the GCC runtime DLLs.

## USGS

`licenses/USGS-MODFLOW-USG.txt`

## Integration

`licenses/PROJECT_ADDITIONS_MIT.txt`

The MIT license covers the AMG/MPI integration only.

## HYPRE 2.33.0

Used as an external static library. Not modified by this project. Dual licensed Apache-2.0 or MIT. Copies:

- `licenses/hypre/LICENSE-APACHE`
- `licenses/hypre/LICENSE-MIT`
- `licenses/hypre/COPYRIGHT`
- `licenses/hypre/NOTICE`

## Windows binary only

The v1.0.0 ZIP contains the verified executable and these unmodified runtime libraries from GCC 16.2.0 (MSYS2 UCRT64, Rev6) and MinGW-w64:

| File | Terms |
| --- | --- |
| static `libgfortran` inside the executable | GPL-3.0-or-later with the GCC Runtime Library Exception 3.1 |
| `libgcc_s_seh-1.dll` | GPL-3.0-or-later with the GCC Runtime Library Exception 3.1 |
| `libquadmath-0.dll` | GNU Library GPL, version 2 or later (`licenses/libquadmath/COPYING.LIB` is LGPL 2.1) |
| `libwinpthread-1.dll` | `licenses/winpthreads/COPYING` |

Corresponding source for the unmodified GCC 16.2.0 runtime libraries is GCC 16.2.0, https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz, built with the MSYS2 recipe https://github.com/msys2/MINGW-packages/tree/master/mingw-w64-gcc. For at least three years from 23 November 2026, the publisher will provide that corresponding source on request. `libquadmath-0.dll` is a shared library and can be replaced.

The same ZIP includes Microsoft's `msmpisetup.exe` for MS-MPI v10.1.3, file version 10.1.12498.52. SHA-512:

```
d09862be9fec0fe5377e2c472dd8c9aff9fbc2db7f2c1f4880a59ed64d51339bdfe3cd70af0f3443ba864bfedf42d684c943ba8c805774816c7760f70749d0f0
```

That hash matches the installer Microsoft published for this version. Individual Microsoft DLL files are not in the archive. See `licenses/ms-mpi/MPI-Redistributables-TPN.txt`.
