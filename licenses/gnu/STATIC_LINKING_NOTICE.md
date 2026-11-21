# Static linking and LGPL notice

MODFLOW-USG Serial AMG v0.1.1-beta includes GNU
libquadmath, statically linked into mfusg_amg.exe.

libquadmath is licensed under LGPL-2.1-or-later.

Users are expressly permitted to modify libquadmath,
relink the application with a compatible modified version,
and reverse engineer the executable for debugging
modifications to libquadmath.

The distribution includes:

- The applicable LGPL license text.
- The corresponding GCC/MSYS2 source package,
  including libquadmath source code and build patches.
- Application object files and relinking instructions.

These materials are supplied to support the rights
granted by LGPL 2.1, section 6.

The project's MIT license applies only to its own
original additions and does not replace the
licenses of third-party components.
