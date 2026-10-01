# M35 executed results

- Starting/unchanged HEAD: `615900164a3ccb7412d2d3a8a2dda8608a1b5f07`.
- Completed genuinely clean Release build: PASS, zero warnings.
- Production: **64 suites PASS**. Focused input target: **40 checks, 0 failures**.
- Actual async application: **12 cases / 246 assertions PASS**.
- Editor Play/Stop/save/reopen and standalone startup smoke: PASS.
- Existing classic/terrain near-origin walk: both PASS, 900 fixed steps each,
  unchanged supplied walk script. These are labelled blocking scripted control/
  physical regressions, not human hardware validation.
- Compiled sources unchanged throughout the completed gate. FTFT/P1/M33/M34
  evidence and research prototypes unchanged. No scene/save fingerprint bump.

The first fresh build failed because older standalone test targets omitted the
new dependency required by Window. No stale tests ran. Source lists were fixed,
then the completed gate started from another empty build directory. Raw failure
output remains in `build-failure/`; there was only one completed production run.
The earlier virtual-controller failure was SDL suppressing hidden-window input;
only the focused test opts into background joystick events. Runtime focus policy
was not relaxed.

## Lightweight performance

Actual default map: 47 named entries / 70 bindings, **6.28735 µs** mean core update
plus queries. Stress: 100 one-binding entries, **20.945650 µs**. 10,000 iterations,
Release settings. These measure InputSystem; they do not include OS event pumping,
controller drivers or editor UI, and are not a universal performance guarantee.
The core uses the same compiled InputSystem source; no alternate mapping algorithm.

## Scope and limits

One active SDL-compatible controller; ordinary face/shoulder/stick/D-pad buttons,
sticks and triggers. Axial linear deadzones, summed axes, multiple digital bindings,
non-consuming frame/fixed snapshots, project rebinding. Mouse look remains relative;
stick look uses a frame-scaled rate. Unknown control tokens remain inactive.
Editor input edits apply on next Play; runtime callers apply SetMap explicitly.
Editor-only shortcuts remain outside game bindings. Historical script injection
is retained for existing regressions, not presented as physical-input evidence.

**Human input validation pending.** The virtual pad is an adapter test, not
physical-controller feel/compatibility acceptance. No commits/pushes/tags made.
Read `docs/M35.md` for authoring, API, format, migration and the short checklist.

## Evidence

Raw completed run: `final/production/results.json`, including commands, source
hashes and every suite result. `final/M35_RESULTS.json` uses the inherited M34
wrapper's human-listening field; M35's relevant human-input disposition is the
pending state above. Demos: `demos/results.json`. Final source inventory/hashes:
`CHANGED_FILES.md`, `SOURCE_SHA256.json`. Prior local failures are preserved.

## Operator-discovered Options crash

The original candidate crashed when opening Options due to temporary-owned menu callbacks. Fixed with in-place reset and non-copyable/non-movable ownership. Focused follow-up passes UI tests, real GL application navigation (29 checks), and all 40 input checks. See `menu-crash/README.md`. The full production run above predates this local repair; it was not repeated.
