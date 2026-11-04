# Release checklist

- [ ] Independent review of matrix ownership, error paths and numerical criteria
- [ ] Stage 3 Windows clean rebuild; no retained `obj_temp`
- [ ] Stage 3 public model tests (including xMD and PCGU and negative tests)
- [ ] Stage 3 Tizert private validation and comparison to frozen Stage 2e
- [ ] Reproducible CI/build script and frozen hypre version/source checksums
- [ ] Verify actual license text/NOTICE for exact hypre 2.33.0 and embedded GCC runtime
- [ ] Resolve GNU libquadmath LGPL obligations if statically included
- [ ] Source provenance/rights for every non-USGS component
- [ ] Clean Windows runtime test
- [ ] SHA-256 release hash, versioning, independent-project disclaimer
- [ ] Remove client model artifacts from public repository
- [ ] GitHub tag v0.1.0-beta only after all blocking items are done
