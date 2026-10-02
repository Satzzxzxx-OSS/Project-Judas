# M41 candidate verification

Starting HEAD: `66b161fa21475716b361cefcdf87b34e3556bd8c`.
No commit, push or tag. Human visual acceptance remains PENDING.

## Executed final gate

Command: `python3 scripts/m41_validation.py` (one clean Release build and one complete production-suite run).

- Clean Release build: PASS, zero compiler warnings.
- Production suites: 75, PASS; actual asynchronous application integration: 246 checks, zero failures.
- Focused runtime UI: 22 checks, zero failures (real GL framebuffer rendering, layout, input, script events, lifecycle).
- Runtime UI Application/export test: 16 parent checks, zero failures; child moved-package interaction also passes from unrelated working directory with poisoned engine-root environment.
- Demo editor Play/Stop: PASS; authored scene IDENTICAL afterwards.
- Demo standalone startup: PASS. This scripted startup smoke is not itself full UI-interaction proof; actual Application/export tests supply that evidence.
- Complete gate: 613.22 seconds.
- Final source fingerprints: all 52 entries matched after validation.
- Protected FTFT/P1 and M33–M40 evidence/prototypes: unchanged.

## Lightweight performance

Final Release real-GL 100-element document: layout 16.551 microseconds; render 414.498 microseconds; 308 actual UI draw calls (including glyphs). This is a sanity sample, not a universal performance guarantee. Application last-frame metrics describe a mostly hidden HUD and are not the full-menu benchmark.

## Evidence / limitations

`final/RESULTS.json`, `final/production/results.json`, test logs, editor log, and `final/SOURCE_SHA256.json` are authoritative. `candidate/` holds focused pre-gate checks. `focused/` retains initial slider activation failure and its corrected follow-up; do not count the initial failure as a pass. `CHANGED_FILES.txt` lists implementation/content/doc changes.

Demo: `projects/ui_demo/ui_demo.judasproj`. Authoring/API/limits: `docs/RUNTIME_UI.md`.
Legacy no-document pause UI remains a compatibility fallback; the new demo's game-facing menus are authored UI driven by JavaScript. ASCII font atlas, no shaping/scrollbars, basic source editor without undo/preview, restart Play after source edits, unique runtime document names. Runtime UI presentation state is not automatically saved.

Human review still required: mouse menu; keyboard/controller focus; HUD/script updates; pause without gameplay double-action; slider/toggle; resize; moved exported game. No human visual or hardware-controller acceptance is claimed.
