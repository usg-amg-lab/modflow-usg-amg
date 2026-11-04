# License verification: release blocker

Do not release binaries on the strength of this draft alone.

1. USGS MODFLOW-USG: determine provenance/rights for this exact 1.5 distribution, particularly any bundled third-party contributions. USGS original government works are public domain in the United States, but this statement may not automatically cover all bundled code.
2. hypre 2.33.0: upstream offers MIT or Apache 2.0. Copy verbatim licensing and NOTICE files from **the exact release** into final release; choose and document license option. https://github.com/hypre-space/hypre
3. Compiler runtime: check GCC Runtime Library Exception for statically linked libgfortran/libgcc. `libquadmath` may have distinct LGPL obligations; check actual binary's static libraries and source-license requirements. https://gcc.gnu.org/onlinedocs/gfortran/Link-Options.html
4. License new authored modifications separately only after rights attribution is settled. Do not claim the project is an official USGS build.

This file is a checklist, NOT a substitute for verified redistribution notices.
