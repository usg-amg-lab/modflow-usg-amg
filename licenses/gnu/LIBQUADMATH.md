# GNU libquadmath - redistribution information

## Component

GNU libquadmath is statically linked into the Windows x64
executable of MODFLOW-USG Serial AMG v0.1.1-beta.

License: GNU Lesser General Public License,
version 2.1 or later (LGPL-2.1-or-later).

The complete LGPL 2.1 license text is included as
licenses/gnu/LGPL-2.1.txt.

## Original build environment

- MSYS2 UCRT64
- GCC / GNU Fortran 16.2.0
- MSYS2 package revision: 16.2.0-6
- Build date: 13 October 2026

Static library: /ucrt64/lib/libquadmath.a

SHA-256:
ada5db755694b97c9ff345265824a3affc07f1b32582efd37104c323cc71ec41

The static archive belongs to the MSYS2 gcc package.
The corresponding runtime package is libquadmath 16.2.0-6.

## Corresponding source code

MSYS2 source-only package:

https://mirror.msys2.org/mingw/sources/mingw-w64-gcc-16.2.0-6.src.tar.zst

The exact source package and any associated build instructions
and patches should be retained with the release materials.

## Relinking

The separate relinking package contains the application
object files and relink.sh.

It allows a compatible replacement libquadmath.a to be
supplied when rebuilding the Windows executable.

## Release requirements

Before public distribution, verify that the release provides
the applicable LGPL notices, corresponding library source,
and sufficient materials for relinking with a modified
compatible version of libquadmath.

The existence of this document does not itself establish
complete LGPL compliance.
