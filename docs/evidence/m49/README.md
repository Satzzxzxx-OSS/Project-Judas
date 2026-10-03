# M49 candidate evidence

Starting HEAD: `e8cd76159d0aa451c2b94971028415543d99d2d1`.
No commit, push or tag is part of this delivery. Human interactive acceptance is pending.

## Scope

Explicit-volume capsule sweeps, shared legacy slide/step mechanics, generic motor state, scene/editor/prefab integration and safe JavaScript control. The ordinary current project contains flat, real radial and copied accepted pool scenes. Camera intent is separate from the motor. Existing liquid field sampling is exposed read-only; production fluid simulation is unchanged.

## Focused development evidence

- `focused/ready-motor.log`: 32 checks, zero failures.
- `focused/ready-application.log`: 39 real EngineHost/world checks, zero failures.
- `focused/legacy-steps.log`: legacy step regression passes.
- Motor batch timing over 120 steps, including the ordinary physics step with a floor: one motor 0.032078 ms, ten 0.318656 ms, 100 3.174374 ms. This is a simple floor workload, not a universal bound.
- Swimming observation: existing field immersion 0.644134; +0.0973358 m vertical displacement over 30 fixed steps with project JS propulsion. No perceptual claim.

## Preserved failures and corrections

Raw `attempt*`/`diagnosis*` outputs remain. Early focused failures exposed the short-step lip probe, support-height following and radial departure reference; these were corrected in the motor/shared explicit-volume path. An early application fixture had an unregistered script asset; its setup was corrected. The real dry-world crash and debugger trace are retained: the new fluid-query binding previously dereferenced an absent fluid service, and now returns a dry sample. The first swimming assertion started partially immersed and incorrectly demanded immediate upward movement; the follow-up uses an explicitly immersed start to verify the actual buoyancy/propulsion response. No accepted fluid assertions or fixtures were retuned.

## Final candidate gate

`scripts/m49_validation.py` reuses the existing complete production runner from a genuinely new Release build directory. Existing application fixture writers are redirected into this evidence directory to preserve historical evidence. Final results, commands, compiler output and source fingerprints are under `final/`. Editor smoke uses a project copy and checks authored state after Play/Stop. Transition/export smoke adds an ordinary JS test sequence only to a disposable project copy, then uses the actual standalone executable and exporter, including a moved package launched from `/tmp`. Production source has no alternate test simulation path.

## Limitations

Query-only motors do not physically block each other or generate M42 motor collision events. Unit entity scale and one root motion owner are required. Recovery/slide/step probes are bounded approximations; the explicit-capsule lip probe may add up to 0.08 m of reported step travel. Dynamic pushing is a capped linear impulse, not exact motor/body momentum conservation. Teleports are not continuous support motion. Existing coarse-fluid limitations remain. See `docs/M49_CHARACTER_MOTOR.md` for ownership, API, detailed semantics and the human checklist.

## Final measured results

- Clean Release configure/build passed, zero compiler warnings.
- Existing complete production run: 92 suites passed, no failed commands; real async application integration: 246 checks, zero failures across all 12 cases.
- Final M49 core: 32 checks, zero failures. Final M49 real application: 39 checks, zero failures.
- Editor Play/Stop: authored state IDENTICAL; standalone startup passed.
- Normal runtime transition sequence flat → radial → pool → pool reload → flat passed. Runtime prefab spawn/destruction was included.
- M38 export passed; moved package launched from unrelated `/tmp` and repeated the same transition/reload sequence successfully. Package size 7,515,253 bytes; export command 0.030674 s.
- Final 120-step simple floor batch: one motor 0.030989 ms, ten 0.317179 ms, 100 3.195049 ms. Includes one normal PhysicsWorld step per batch; no universal performance guarantee.
- All recorded final-source fingerprints still match. Protected historical evidence/prototypes and original fluid project are unchanged. HEAD/main/origin/main remain the starting checkpoint; no commit/push/tag.

Headless SDL reports that relative mouse capture is unavailable; the automated runs do not verify desktop mouse capture or interactive feel. Human validation is pending. The exported smoke script exists only in its disposable copied project; the ordinary authored demo remains human-controllable.
