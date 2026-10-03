# M52 evidence

Current game is ordinary `projects/shooter_game`, not an evidence-path scene.
Original/follow-up logs here are preserved; final state is documented in
`../../M52_SHOOTER_GAME.md` and `jitter-followup/`.
`final/` retains the pre-human-test automated candidate (including its original
fingerprints); the corrected source fingerprints are in `jitter-followup/SOURCE_SHA256.json`.

## Development records

- `build-development.log`: new test fixture compilation mistakes (wrong C++ accessor names and mixed `auto` types); corrected in build follow-up.
- `game-development.log` / `game-followup-1.log`: authored scene omitted required settings/component headers; corrected as content.
- `game-followup-2.log` / `game-followup-3.log`: test continued to call PhysicsWorld::AliveBodies after Shutdown, causing the test-process crash. `teardown-followup-trace.log` identifies that call. The test no longer accesses a shut-down physics world. No production engine crash was inferred or patched.
- Same early game checks exposed a reversed target hinge limit, too-weak shot impulse, and incorrect test boolean/pause assumptions. The authoring now permits a visible swing; the test mirrors outer-loop pause ownership. `game-followup-5.log` passes 33 checks.
- `application-development.log`: synthetic key transitions injected before Window::PollEvents lost their frame edges. Application tests now push real SDL events through that existing boundary. `application-followup.log` passes 14 checks.
- `game-camera-followup.log`: shoulder camera avoids cosmetic head/reticle obstruction. Final test additionally checks player-side cover and a whole twelve-target round.
- `range-first.png` is an early scripted-harness world-only screenshot; authoritative full world+UI screenshots are in `final/application/`.
- `final/performance.log`: initial observer flags didn't request timings; its fixed-step zero is NOT accepted measurement. Correct setup and actual nonzero timing are recorded in `final/performance-followup.log` and CSV.

## Original automated candidate evidence (before human jitter report)

- Fresh Release: `configure-release.log`, `build-release.log`; zero compiler warnings.
- `final/game-round-followup.log`: 36 checks / 0 failures, supersedes earlier 34-check final game proof with the round-completion check.
- `final/application.log`: 14 checks / 0 failures, actual normal asynchronous app.
- `final/script-regression.log`: 34; `final/joint-regression.log`: 19; surface/impulse-point/stale-handle cookbook: 9 each, all passing.
- `final/api.log`: type checker, live VM surface and negative drift checks.
- `final/editor.log`: Play/Stop IDENTICAL; engine-only diagnostic/editor screenshots remain separate from runtime game UI.
- `final/export.log`, `final/moved-package.log`: exact project export and moved executable startup, no source edits to the exported game. This is startup/resource smoke, not human gameplay acceptance.
- `final/application/{first,third,pause}.png`: screenshots, not human acceptance.
- `final/performance-followup.log`: normal desktop renderer/frame clock. Excludes 30 asset warmup frames; frame CSV retains them. Audio in focused checks uses the backend's non-device test mode; audible quality remains human work.

No full production-suite rerun, no protected historical evidence modified, no fluid/AI/navigation work.
Human gameplay/visual/listening/controller acceptance is pending operator review.

## Human-discovered presentation mismatch — corrected follow-up

The operator reported moving-player model and earlier scripted spacecraft jitter.
`jitter-followup/original-*.js` preserve those exact project scripts;
`pre-repair-SOURCE_SHA256.json` preserves the original candidate identity. C++
already interpolates motor and rigid-body world poses, but JS cameras used raw
fixed-step poses and the avatar was copied before the motor moved. This was a
generic presentation/API gap plus incorrect phase use in project scripts.

The new readonly `Entity.presentedTransform` and post-simulation
`presentationUpdate(dt,alpha)` reuse that existing interpolation. The shooter
camera/cosmetic model and current boundary-demo spacecraft camera use it.
Authoritative transform/query/motor/force semantics remain unchanged; no fluid,
solver or game-policy implementation was added.

- `baseline.log`: 36 checks / 0 failures; preserved original scripts reproduce
  0.020833/0.062500 m moving shooter mismatch at alpha .25/.75, plus spacecraft
  position/orientation mismatch. This deliberately uses the new engine with the
  original scripts to isolate their presentation timing.
- `presentation.log`: 44 checks / 0 failures; position matches within 0.000001 m
  in those cases, orientation matches within float precision, authoritative pose
  and velocity are unchanged, frame look works without a fixed step, fixed motor
  intent is rejected in presentation, and Stop clears callback/view state.
- `game.log`: 36; `application.log`: 14; `boundary.log`: 26; `scripts.log`: 34;
  `surface.log` / `cookbook-presentation.log`: 9 each. All pass. Script regression
  intentionally tests faults and interrupt isolation; its error diagnostics are expected.
- `api-live.log`: 16 exports / 148 symbols / 93 native operations / 13 callbacks,
  TypeScript 5.9.3 and three negative controls pass against actual VM enumeration.
- `editor.log`: Play/Stop IDENTICAL; `export-*` / `moved-*`: exact current shooter
  and boundary projects export and launch from unrelated `/tmp` (flight explicit
  registered scene). These startup checks do not establish human game feel.
- `performance.log`: desktop Release, 210 warm measured frames, 58.392 FPS,
  17.126 ms mean, 19.01 ms p95, 21.74 ms maximum, 0.458 ms fixed step, 30 bodies.
  Old measurements remain in `final/`; no performance improvement is inferred
  from small differences between these short runs.
- `build.log`: new test target required CMake reconfiguration; corrected build in
  `build-followup.log` (zero warnings). `*-development.log`: first probe fixture
  omitted a normal SceneSession and faulted session reads; corrected fixture/run
  supplies it. Neither is claimed as a new production defect.

Reproduce narrowly with `judas_script_presentation_tests [--baseline]` from the
repository. Offscreen automation can use `SDL_VIDEODRIVER=offscreen` and dummy audio.
Current packages: `/tmp/Judas_M52_Smooth_shooter/judas` and
`/tmp/Judas_M52_Smooth_boundary/judas`. Human movement/visual acceptance is pending.
No full production rerun and no protected history was changed.
