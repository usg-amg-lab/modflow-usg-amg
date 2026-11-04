# Reproducible Windows x64 build (MSYS2 UCRT64)

Prerequisites: UCRT64 GCC/gfortran, make, CMake, Ninja, and **hypre 2.33.0 source**.

## Build hypre separately

From MSYS2 UCRT64 (replace paths):

```bash
cmake -S /e/MODFLOW-USG/hypre-2.33.0/src \
  -B /e/MODFLOW-USG/hypre-2.33.0/build-serial -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
  -DHYPRE_ENABLE_MPI=OFF -DHYPRE_ENABLE_OPENMP=OFF \
  -DHYPRE_ENABLE_DSUPERLU=OFF \
  -DHYPRE_ENABLE_HYPRE_BLAS=ON -DHYPRE_ENABLE_HYPRE_LAPACK=ON
cmake --build /e/MODFLOW-USG/hypre-2.33.0/build-serial -j 4
cmake --install /e/MODFLOW-USG/hypre-2.33.0/build-serial \
  --prefix /e/MODFLOW-USG/hypre-2.33.0/install-serial
```

## Clean compilation of MODFLOW-USG

```bash
cd /e/MODFLOW-USG/mfusg_amg_stage3_rc_source/make
make clean
make -j1 HYPRE_PREFIX=/e/MODFLOW-USG/hypre-2.33.0/install-serial
ldd ./mfusg_amg.exe
sha256sum ./mfusg_amg.exe
```

**Do not use `make -j`**: original Fortran module dependencies are not fully encoded.

Verify `ldd` shows only OS libraries. Test on a second, clean Windows installation.

Build flags follow Stage 2e; compiler optimization and library versions may affect numerical differences.
