# M53 evidence

Starting checkpoint: `3c52b13765a0721fa6a1fea0038505326359326b`.
Candidate is uncommitted; human visual/gameplay acceptance is pending.

- [Development failures and corrections](development/FAILURES.md).
- `final/RESULTS.json`: one clean Release/production gate, then API, editor bake,
  Play/Stop, real scene transitions and moved-export commands.
- `final/production/`: unchanged production-suite assertions and async integration
  executed with output-only adapters into M53 evidence. This does not rerun P1 research.
- `final/api.log`: source/type inventory, real-VM enumeration and TypeScript check.
- `final/cookbook.log`: executed public API examples, including navigation.
- `final/bake.log`: layer/polygon counts, source hashes and bake times.
- `final/editor-bake-play-stop.log`: same inspector bake command and editor lifecycle.
- `final/transitions.log`, `final/moved-transition-probe.log`: disposable ordinary
  registered project proves queued reload/replacement, safe handles and nav resources.
- `final/moved-package.log`, `final/game.png`: unmodified game export launched from
  `/tmp`, outside repository/package working directory. This screenshot uses the
  fixed-step harness; full world+HUD/pause screenshots are in `final/shooter-application/`.

The transition probe modifies a copied project only. The playable export is the
actual Spring Range project. Headless GL screenshots/state checks are automated
observations, not human visual/controller/audio acceptance.

Final result summary, source fingerprints and exact changed-file inventory are
recorded alongside these logs after candidate verification completes.

## Narrow final follow-up

`lifetime-followup/` preserves the direct-world-destruction crash (exit 139),
its original probe/build output, and the corrected checks. Navigation now survives
until script destroy callbacks finish; disabled source-exclusion modifiers are inert.
The probe initially also omitted its ordinary SceneSession and produced a script
diagnostic; the corrected fixture supplies normal project scene/session context.

The full production/async gate was not repeated. The affected Release targets,
77 navigation checks, 26 actual game/lifetime checks, navigation example, live API
coverage, inspector bake/Play-Stop and refreshed moved export passed. The independent
link probe verifies the sixth motor actually crosses the ordinary collider gap
(`x=20.9815`), not merely that a planner reports a route. Stale export is rejected.

`FINAL_SUMMARY.md` records measured results and limitations; `SOURCE_SHA256.json`
records final candidate content; `CHANGED_FILES.md` lists every changed/new file.

A final geometry check corrected capsule-foot sampling to include the existing
rotated local motor offset, and projects navigation progress for non-motor root
pivots without moving entities. The first offset fixture incorrectly mutated live
settings that RuntimeCharacter reloads from its definition; the corrected fixture
uses SetCharacterSettings. Original and corrected logs are preserved. Final
navigation checks: 77/77; game/lifetime: 26/26.
