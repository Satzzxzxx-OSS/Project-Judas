# FTFT9 current-schema FTFT1 application smoke

Executed only `scripts/ftft9_validation.py::persistence()` after rebuilding `judas` and `judas_editor` under `flock build/ftft9-build.lock`. No production-suite, asynchronous, harness, prototype or unrelated validation entry point was run by this smoke.

Result: **PASS**, six actual application invocations (three runtime plus three editor), 5.212 seconds excluding the recorded build. The valid save uses canonical compatibility schema 2, freshly fingerprinted from the same authored scene with current production fingerprint code. Runtime loads it and advances the existing three-step script. Invalid fingerprints and legacy world-state version 1 both exit runtime with status 1 and the existing useful error messages. Editor Play constructs six bodies for the valid case and zero for each rejected case. Authored state restores after Stop in all cases. Scene/save/project SHA-256 values are identical before/after all six invocations; rejected files are neither replaced nor deleted.

The fast headless editor frame-140 observation reports zero fixed steps in all three cases. Its inherited assertion proves Play acceptance/rejection and authored restoration, not elapsed-time simulation progress. Standalone valid runtime records all three actual fixed steps. No human visual validation was performed; inherited screenshot-review labels are not claimed as reviewed.

## Adapter review

- Historical `docs/evidence/ftft1/runtime_smoke/run.py` is unchanged (SHA-256 `3663b7698978aeee9a729421cf52b4a386e879ee6733c8470be4c4b037ae1aa0`).
- The adapter requires exactly one `compatibility 1 sha256` occurrence and replaces only that generated fixture token with the current declared schema (2). All old runtime/editor return-code, error-message, body-count, authored-restore and file-hash predicates execute unchanged. Legacy version 1 input is unchanged.
- Output relocation and copied fingerprint-tool source do not replace any production simulation or validation path. Fresh output prevents accidental overwriting of old evidence.
- The full FTFT9 runner's four named historical source-protection exceptions are `RuntimeWorld.cpp/.h` and `SceneFingerprint.cpp/.h`. Current diffs are the new fluid ownership/settings/player metadata and explicit canonical field additions/schema increment. Gravity construction and saved-delta application code are unchanged.
- Whole-file exceptions are permission to inspect those reviewed modifications, not proof that arbitrary future edits are safe. The runner records them explicitly and still checks all source fingerprints and protected-path snapshots before/after execution. It retains strict protections for `WorldState.cpp/.h`, original FTFT1 evidence and fluid prototypes. The other old FTFT2/3 evidence and helper sources also remain unchanged.
- Nested subprocess wrappers restore the original function in `finally`; they append output paths only to the listed new fixture executables (and the inherited gravity fixture). They do not alter test assertions, fixtures or return values.

`summary.json` stores exact HEAD/source/binary fingerprints and successful before/after equality. `ftft1-current-schema/results.json` records each executable invocation, renderer environment, raw log paths, file hashes and acceptance predicates. `ftft1_reuse.log` provides the original smoke summary. Build output is recorded without binaries.

Reproduce the bounded smoke using `importlib` to load `scripts/ftft9_validation.py` under a non-main module name, create a fresh persistent directory, and call `persistence(directory)`. Do not execute `main()` when only this smoke is intended.
