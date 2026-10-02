# M43 runtime scene transitions — candidate for operator review

Starting/current committed HEAD: `ed6e8fc342b3dfd9982ea7a07ad54314f9ff9926`.
Implementation remains uncommitted. No push/tag. Human validation is PENDING.

## Executed evidence

- One genuinely clean Release build, zero compiler warnings.
- One complete production run: **78 suites PASS**; real async integration:
  **246 checks, zero failures** (`final/production/results.json`).
- Initial focused current-engine transitions: **21 checks, zero failures**.
- Editor Play/Stop: authored scene after Stop **IDENTICAL**.
- Standalone startup PASS. Export contains both registered scenes.
- Moved package ran its ordinary Application loop from unrelated `/tmp` cwd:
  real sensor callback A -> B -> A, session key observed true in both scenes.
  Project-owned JS supplied automation; no alternate engine transition path.

### Narrow post-gate correction

Review identified cursor capture could remain released after leaving a modal
scene. The current-engine witness reproduced **23 checks, one failure** in
`followup/cursor-before.log`. `InteractivePlay::Begin` now invalidates capture
initialization; its first frame reconciles cursor ownership after new UI starts.
The demo's existing project input map also now binds Interact to E, matching its
HUD. No physics, serialization/fingerprint policy or algorithms changed.

Affected checks rerun on the final candidate:

- Scene transitions: **23 checks, zero failures**.
- Actual runtime UI application: **16 checks, zero failures**.
- Input: **40 checks, zero failures**.
- Editor Play/Stop: authored scene **IDENTICAL**.
- Refreshed Release package copied/moved outside the repository to
  `/tmp/Judas_M43_Moved_ed6e8fc`; ordinary standalone execution from `/tmp`
  observed A -> B -> A and retained session data, exit 0.

No redundant full-suite rerun after this narrow correction. Exact four source/
content changes since the full gate are in `FOLLOWUP_SOURCE_CHANGES.json`.
`followup/SOURCE_SHA256.json` is the final-source manifest: **30 hashes matched**.
`followup/RESULTS.json` records actual commands, exit codes and elapsed times.
A cross-filesystem package move initially failed with EXDEV in the diagnostic
runner; it now uses `shutil.move`. Engine export itself succeeded. Only the
remaining move/launch was resumed; passing tests were not repeated for that error.

## Package measurement

Final representative package: **6 registered assets, 2 scenes, 6,935,623 bytes**;
export **0.029 seconds** on this machine. This is a file-copy observation, not a
universal guarantee. Generated packages/binaries are excluded from evidence/Git.

## Reproduce / review

- Complete clean gate: `python3 scripts/m43_validation.py` (requires a fresh
  evidence destination; archive prior output first rather than overwriting it).
- Narrow follow-up recipe: `python3 docs/evidence/m43/followup/run.py` (fresh
  fixture/package destinations required). Both recipes reuse normal engine paths.
- Demo: `projects/scene_demo/scene_demo.judasproj`.
- API, ownership, limits: `docs/SCENE_TRANSITIONS.md`.
- Exact source/content list: `CHANGED_FILES.txt`; status: `GIT_STATUS.txt`.

Earlier focused failures are retained: JS string typo, primitive session values
incorrectly using object-root validation, and an incorrect test assumption that
all AdditionalEntities were runtime-spawned. The corrected test checks the actual
spawned identity, without ignoring authored entities or weakening lifecycle rules.
The later cursor failure is also retained. All FTFT/P1/M33–M42 evidence, research
prototypes and historical failures remain unchanged.

## Operator checklist

1. Open Scene A; E sends the prefab courier through the green sensor to Scene B.
2. In B, E or the authored pause-menu Return button returns to A.
3. Confirm session key/visits survive transitions and R reload.
4. P spawns a courier; R removes spawned state and restores authored transforms.
5. Pause -> Return/Reload restores mouse look; previous scene UI/scripts stop.
6. Launch a moved exported package and repeat transitions.

The trigger participant is a normal prefab rigid body; custom sweep-only player
sensors remain outside M42's accepted scope. Scene loading is synchronous; no
streaming, persistent entities or automatic session save is claimed. Reload reads
the saved authored scene file, so save editor edits before testing reload.
