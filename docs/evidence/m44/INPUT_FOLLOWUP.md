# Human-input follow-up

Human reported invisible pause menu and P spawning nothing.

Reproduction: real SDL key events through the normal Application loop gave
7 passes / 1 failure: P created an entity, but Escape immediately closed its
panel. Resume's **focus** event was incorrectly accepted as activation by
project JS. Original failure: `input-before/run.log` (exit 1).

Correction is confined to project demo JS: Resume/Quit require click events;
the unused Options button is hidden; additional prefabs spawn three metres
along the active camera ray and the HUD shows the spawn count. Initial source
placement before the first view remains unchanged. No engine query/input/UI
mechanism changed. The source remains ordinary project behaviour.

Affected rerun only: `judas_physics_cast_application_tests --output
 docs/evidence/m44/input-followup` with SDL offscreen / real GL. **8 checks,
0 failures**, including physical P, Escape panel visibility, and Escape resume.
No repeated full regression. Prior full gate remains in `final/` as executed;
its original source manifest is preserved. Latest candidate fingerprints are
`INPUT_FOLLOWUP_SOURCE_SHA256.json`. Human re-review is pending.
