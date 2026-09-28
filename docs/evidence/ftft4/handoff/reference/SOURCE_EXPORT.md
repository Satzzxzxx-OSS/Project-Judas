# Source snapshot needed to finish the source-specific FTFT4 brief

This is a read-only export, not permission to edit or begin FTFT4.

From the Judas repository, first confirm the current commit and clean status:

```sh
git rev-parse HEAD
git status --short
```

If the reviewed current checkpoint is still 3404388f1af48419505c802fc11dbf8a0b577f38:

```sh
git archive --format=zip \
  --output=../Judas_FTFT4_Source_3404388.zip \
  3404388f1af48419505c802fc11dbf8a0b577f38 \
  src tests CMakeLists.txt docs/FTFT.md docs/ARCHITECTURE.md
```

Attach the archive here. It needs no binaries, build caches, research history, saves or credentials. If HEAD has changed, report the new hash rather than silently exporting a different baseline. No source changes, reset, commit, push or tag is required.

This permits actual source inspection and, if local dependencies permit, direct C++ probes. It does not guarantee the exported subset is a self-contained build: vendored dependencies and build configuration may require additional files.
