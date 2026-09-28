# FTFT4A current working-tree performance review

This ZIP contains the current uncommitted implementation, including new source
files. Base HEAD is 3404388f1af48419505c802fc11dbf8a0b577f38. It was NOT produced with
git archive HEAD.

Start with docs/evidence/ftft4/RESULTS.md, README.md and GEOMETRY.md.
Compare baseline/performance/results.json and final/performance/results.json;
each directory includes all seven measured logs. Development profiling source,
commands, source fingerprints and raw textual output are included. The
pre-fix/source directory is explicitly historical baseline source; src/ is the
current implementation. The before-guard profile is historical, not the final
source profile.

The git/ directory records full HEAD, both grouped and expanded short status,
and the exact tracked binary diff against HEAD. Untracked files do not occur
in that diff, so their actual content is included and hashed in MANIFEST.json.
The manifest covers every ZIP member except itself.

This is a focused review export. Existing result JSON and documentation can
refer to deliberately omitted full-suite artifacts. No original source or
evidence file was edited to make those references appear self-contained.
Assets, third-party dependencies, fluid prototypes, previous FTFT runner
dependencies, binaries, caches and build output are excluded. The full
validation runner therefore still requires the normal complete checkout.
Included profiling commands can be reproduced with the exported physics
sources and ordinary C++17/GLM dependencies; no profiling was rerun for export.

EXPORT_VERIFICATION.json records before/after repository content, Git diff,
status and index comparisons. The source and repository contents were not
changed by this export.
