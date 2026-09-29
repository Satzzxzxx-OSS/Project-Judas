# FTFT6 asset integrity: reproduced defects and narrow repair

The final focused test uses the real `AssetDatabase`, OBJ loader and texture loader. Temporary fixture files are removed after each run. No filesystem calls or import/identity operations are mocked.

| Run | Cases | Checks | Failures | Exit |
|---|---:|---:|---:|---:|
| Original source, initial tests (`before-*`) | 14 | 55 | 42 | 1 |
| Initial repair (`after-*`, without `after-final`) | 14 | 55 | 0 | 0 |
| Original source, final tests (`before-extended-*`) | 17 | 67 | 54 | 1 |
| Final repair and final tests (`after-final-*`) | 17 | 67 | 0 | 0 |

## Reproduced mechanisms

- `Import` and `Move` used a string prefix for containment: a normalized sibling `Assets_backup` passed as inside `Assets`. Relative and absolute destination variants reproduced it.
- `Track` overwrote an existing valid identity or corrupt sidecar when called with a fresh ID.
- `Import` and `Move` overwrote valid/corrupt sidecars whose asset file was absent, destroying the existing identity/evidence.
- A dangling metadata symlink also needs to reserve its name. The final three additional cases reproduce and prevent overwriting or following it.
- `Track` wrongly rejected a legitimate inside filename beginning with two dots.

## Repair and unchanged behaviour

`src/AssetDatabase.cpp` now checks normalized relative path **components**, and refuses existing metadata directory entries using error-aware `symlink_status` before any copying, renaming or writing. The test snapshots every fixture file/sidecar byte, symlink target and database identity/diagnostic record to verify refused operations leave them unchanged.

Successful import and normalized in-project rename still decode the asset, keep metadata byte-for-byte, retain stable ID across rescan and resolve the moved path. Duplicate metadata keeps the existing deterministic first-holder policy and is reported without modifying files.

This is lexical destination containment and identity preservation. It does not introduce a concurrent filesystem transaction or claim protection against hostile ancestor-symlink replacement races.

## Reproduction

The project target is `judas_asset_integrity_tests`:

```sh
cmake --build build --target judas_asset_integrity_tests -j2
build/judas_asset_integrity_tests
```

Each `*-metadata.json` contains the exact directly executed build/run commands, flags, exit status, elapsed time and source/binary hashes. Binaries remain in ignored `build/ftft6-assets/`, not this evidence directory. `AssetDatabase.before.cpp` is the original source; `AssetDatabase.initial-fix.cpp` and `AssetIntegrity.initial.cpp` reproduce the earlier development run. Final test/source are in `tests/AssetIntegrityTests.cpp` and `src/AssetDatabase.cpp`.

The final direct build emitted no warnings; the 17-case test took 0.00572826 seconds. Parent FTFT6 validation separately runs this target linked against `judas_engine` and relevant application/regression coverage.
