# M65 current candidate — developer integration

Starting HEAD/main/origin/main: `19a53a42a9b363818e67c07d8b63e9fddcea2845`.
Implementation remains uncommitted; human review is pending. Accepted history is untouched.
This report is current M65 evidence, not a rewrite of the frozen consumer reports.

## Input and consumer scope

The three report bytes in `input/` are frozen from the supplied bundle. The Skate
report is M60 evidence, Rooftop is M64 evidence. Their proposals are not automatic
diagnoses. Source and existing regressions establish D-01/D-02/D-03 already fixed,
M64 geometry/query additions and M61 save capability. Wider wishes remain distinct.
The skate author's wrong limit signs, import preparation/orientation mistakes and
spawn placement remain game/content mistakes rather than newly claimed engine bugs.
The original editor evaluation was partial/headless; M65 automation is not human UX acceptance.

The permitted local Rooftop Run project was copied into `projects/m65_integration`;
original hashes/provenance are in `rooftop-copy-input.json`. Its manual checkpoint
box overlap workaround is replaced by normal sensor callbacks. The human-review
follow-up also excludes its own motor from environment casts using ordinary query
filters; arcade steering,
coyote time, checkpoint/finish rules and cameras stay JS. Frozen pre-migration copies
allow repeatable content generation. No original consumer worktree was built/edited.
Mixamo redistribution was unverified, so no skate character/art was copied. The
CC0 10-joint rider is an explicitly labelled original reproduction, not validation
of the original 13-bone character or its quoted performance.

## Closure table

| Source ID | Current finding/status | Change and proof | Remainder |
|---|---|---|---|
| Skate D-01/D-02 | Already covered before M65 | Existing joint correctness regressions retained in final gate | No new rail/grind policy |
| Skate D-03 | Already covered before M65 | Localization unpublished fallback/missing-key regressions retained | No localization redesign |
| Skate E-03/E-04 | Core capability already M64 | Cooked mesh/hull/rotated compound geometry, collider metadata and closest point retained | Coarse art does not promise smooth skating |
| Skate E-05 | Save capability already M61; clarified/partial | Current cold-process slots and qualified ownership proof | Not a new independent profile-store API |
| Skate B-01 / Rooftop E3 | Reproduced; fixed | 11° uphill/downhill tangent support, oblique rotation, real departure, edge/platform and M49 regressions | Existing slope/recovery limits remain |
| Rooftop E4 | Clarified | supportNormal is capsule contact/separation; docs give safe use | No fabricated mesh-face normal or corner smoothing |
| Rooftop E5 | Reproduced gap; implemented | Massless query capsules; normal M39/M42 enter/stay/exit after movement; checkpoint copy | Endpoint sensor overlap, no full crossing CCD or motor blocking |
| Skate E-01 | Implemented | Final jointTransform local/model/world; visual socket, layered/interrupted/ragdoll app proof | No bone interpolation fiction or physical sockets |
| Skate E-02 | Implemented bounded seam | Two-bone M47 contributor; reach, length, degenerate/weight tests; independent board feet | Direct three-joint chain, uniform positive scale; no foot orientation/balance/physical IK |
| Skate F-01 | Current resting cost reproduced; implemented | Contact/joint islands sleep/wake; M56 1/10/20 and pile; liquid contact wake | Active/free-spinning bodies continue solving; residual registration/pose cost remains |
| Skate F-02 | Deferred, measured backlog | Current stream crossing/pin/adopt/revisit measured | No splittable integration scheduler or hard budget guarantee |
| Skate E-06 | Implemented | Normal physical material resource, safe identity/coefficient mutation, query/contact identity; cold restore | Body-level identity, no per-triangle material or new friction law |
| Skate E-07 | Implemented | Public createJoint/configure/destroy, validate-before-replace, stable lifetime; cold/stream proof | Existing M45 shapes/limits; no new solver |
| Skate E-09 | Implemented | physics.gravity uses normal resolver; oblique override/radial return; non-motor JS body query | Fixed M23 origin remains |
| Skate C-02 | Implemented | Stable multi-selection/search, common/mixed batch edits, delta transforms, grouping/reparent, component copy/paste, subtree duplicate, single undo | Existing editor visual language |
| Skate C-03/B-02 | Partial | Stable skeleton picker and rest/local axis overlay for mappings, sockets, IK; joint frame guidance | No autofit/humanoid template/rig limit increase |
| Skate C-09 | Implemented | Typed entity inspector properties, safe wrappers; prefab/duplicate/stream/save remapping | Not arbitrary typed scripting objects |
| Skate C-01/C-04 | Partial; current M64 supersedes collision claim | Existing cooker/selection retained, importer diagnostics identify actual limits, named authoring | No arc/spline DCC, FBX/multi-mesh/bone-reduction/retargeting |
| Skate C-06 / Rooftop E7 | Implemented | Structured JSON authoring invokes normal scene/UI/input serializers/validators; round-trip/invalid-input checks | No second runtime schema or general schema language |
| Skate C-08 / Rooftop E1/E2/E8 capture | Implemented | Normal Input→JS/UI/presentation/render harness, multi-step telemetry, several/paused captures | No proof of human input feel; not a GUI automation framework |
| Rooftop E6 | Implemented | Scene/view backgroundColour contract; actual GL clear check, normal HDR precedence | Not display RGB under tone mapping |
| Rooftop E8 title/hints | Fixed | Configured game title and removal of inapplicable universal save hints | Editor identity retained |
| Skate E-08/B-04 | Partial | Instance UV scale/offset, primitive UV documentation, actual GL sampling check | No world/triplanar mapping |
| Skate C-05/B-05 | Partial | Specific invalid-clip/settings/group/capacity errors and group docs | One AudioEmitter per entity remains |
| Skate C-07 | Implemented | Explicit registered export include/exclude; startup/world dependencies rejected if excluded | Dynamic registered assets retained; no JS reachability guessing |
| Skate B-03/B-06 | Clarified | Joint regression source and actual recent evaluated-motion semantics documented | Not stronger motion continuity/get-up promises |

## Mechanisms and ownership

Input harness actions use actual authored physical bindings with normal edges; no
private JS intent injection. InteractivePlay performs the same script camera,
presentationUpdate/interpolated root, world renderer and UI drawing. Frame captures
work when fixed simulation is paused. Selected motor telemetry includes its pose/up,
support and actual velocity. Test mode alone isolates real desktop input.

Motor departure includes effective gravity/external acceleration once and uses
relative motion against current support separation normal. Its tolerance is
min(0.1×skin, tangential distance×skin/radius), with the slide-resolution floor
of 0.000001 m; normal-only gentle lift is not repeatedly discarded. Motor capsules
register before fixed physics; endpoint sensors and observed sweep/support contacts
coalesce once after motor movement through the normal filtered lifecycle. They have
no dynamic mass and cannot be constrained as rigid participants. Missing impulse is null.

Resolved skeleton transforms are read-only; sockets are target-first visual attachments
with authored stable references and cycle/physical-owner rejection. IK contributes in
(order,id) order before the physical producer. The board is project JS content, not
an engine skate system. Both feet measure final poses instead of transitionFraction.

Physics sleeping is generic contact/joint island quiescence: bounded velocity,
pose/anchor/contact/limit errors and 0.75 s quiet time. Spring equilibrium can be
away from rest because gravity balances it. Positive speculative support rows join
both settling and waking even without a touch callback. Shared static floors do not
merge unrelated islands. Awake impacts/loads/configuration changes wake before solving.
Unchanged gravity is deferred while asleep and applied once if awakened; changed
gravity wakes. Sensors/queries/spatial identity remain. The active solver/cadence is
unchanged; performance reference disables only sleeping through a test control.

Force producer review: DynamicBody gravity uses ApplyLinearAcceleration; celestial,
aerodynamic, coarse, spacecraft/legacy controls, manipulation, liquid hydrostatic/drag,
JS and fracture use PhysicsWorld force/torque/impulse/mass interfaces. M62 attachments
and contacts use impulse-at-point. Legacy particle coupling wakes two-way contacts
before solving. None may silently leave a changed load on a frozen body. Continuously
applied balanced non-gravity forces conservatively keep bodies awake.

Physical and render materials are independent resources. Runtime joint settings
remain ordinary owner/body references, not raw solver memory. Editor batch operations
use existing snapshot commands; properties untouched by mixed edits stay unchanged.
Named authoring uses real validators, keeps runtime scene/UI/input formats compatible,
and rejects unknown fields or non-string canonical scene values. Conditional
fingerprint extensions retain canonical schema 5 and old content identity.

## State and compatibility

M61 stores normal runtime IK/socket/entity-reference/material/joint authored state.
An optional version-1 sleep chunk adds one participant; old 11/12-participant slots
without it remain supported, new slots have 12/13 depending on deformables. The M61
application test's expected count moves 11→12 specifically for this chunk. No GPU
matrices/layout caches are persisted. Cold-process save/load uses ordinary slots.

M59 typed/socket dependencies pin external targets. Live pose/articulation state
remains unsuspendable and visibly pinned; M65 does not claim lossless pose streaming.
The stream proof asserts that policy, adopts the complete parented socket/reference
assembly to root, then unloads/revisits the residual physical body/joint. Handles
retire, reconstructed bodies are fresh, adopted entities do not duplicate. Runtime
joint assemblies must be adopted together. Save/reload and Play/Stop use normal worlds.

## Performance

[PERFORMANCE_FINAL.json](PERFORMANCE_FINAL.json) contains corrected-source M56
median/p95/max and workload counters. Initial measurements in `PERFORMANCE.json`
remain pre-follow-up evidence. The final trials use 60 Hz, the final 120 frames
of 2200, sequential desktop runs, unchanged content and a sleep-disabled reference.
Other user applications remain running; starting load averages were 3.94/3.08/2.56.
These are elapsed CPU times, not achieved real-time FPS or original Mixamo-rig timings.

| 7-body articulations + 13 baseline dynamic props | Fixed median reference→sleeping | Sleeping fixed p95 / max | Whole frame median |
|---|---:|---:|---:|
| 1 | 0.718→0.515 ms | 0.669 / 1.080 ms | 1.557 ms |
| 10 | 1.776→1.049 ms | 1.198 / 1.301 ms | 2.886 ms |
| 20 | 3.289→1.883 ms | 2.219 / 2.486 ms | 4.919 ms |

All 20/83/153 dynamics settle asleep. Residual broadphase, island, script, pose,
registration and rendering work remains. Twenty additional rigs with two limb
contributors/sockets cost 0.326 ms median final-pose resolution (0.413 p95,
0.489 max); whole fixed step 1.018 ms. The independent reachable lab rider's
final warmed foot error remains below 0.000001 m.

Native motor+sensor workloads at 1/10/100 motors: fixed median 0.030/0.297/3.616 ms;
p95 0.038/0.377/5.157 ms; max 0.057/0.499/5.685 ms. They omit graphics and ran during
focused verification; machine activity is not controlled. The large workload remains
measurably expensive. Streaming integration remains indivisible in places; no hard
budget or free-crowd guarantee is claimed. Raw M56 data is retained in lossless gzip
with original-byte hashes, frame counts and paths in the compression manifests.

## Genuine failures and follow-ups

Development logs remain in this directory and `development/`. They include compile
integration errors; first-step sensor registration; original M49 radial/support-follow
assertions; absent display in sandbox; wrong `.mass` use in lab HUD; injection before
InputSystem BeginFrame; sleep criteria missing speculative support/spring equilibrium;
pre-solver gravity mistaken for impact velocity; a legitimately free-spinning limb;
scene-vector pointer invalidation during fixture authoring; focus event mistaken for
Resume click; attempted nonexistent input.consume removed; stream test wrongly expecting
pose suspension; an invalid two-root pose prefab; and the board target exceeding
rig reach at extreme tilt. Additional preserved public test-fixture failures used a
non-unit quaternion axis and mistook `world.entity` for a verified nullable lookup.
The corrected public proof uses unit rotations and documented `.valid`, and also
checks independently targeted prefab instances. A malformed export-list fixture
used a singular key; the corrected plural `exclude-scenes` proves manifest rejection. Corrections are generic engine fixes or explicitly content/
test fixes. No invented ceremonial failures and no historical evidence overwrites.

The rider height correction makes targets physically reachable without shortening
limbs, stretching bones or rotating the rider as one rigid board. The final tests
also correct awake counters in the disabled-sleep reference and wake propagation
through cached speculative contact. The late harness negative check exposed unknown expectations matching zero;
`harness-expectation-failure/` preserves it; the corrected parser rejects unknown/unbound names, wrong input kinds
and non-boolean expectations. A follow-up evidence parser initially counted only
rows matching the initial objN header; runtime ragdoll births append body columns.
The corrected checker reads the stable first 19 telemetry fields. Both results remain.
Earlier measurements are retained as development records and superseded by
`performance-final` for final claims. A tooling invocation without Node on PATH is
preserved; the installed bundled Node and TypeScript 5.9.3 pass the actual check.

## Final candidate validation

[CANDIDATE_RESULTS.json](CANDIDATE_RESULTS.json) records the complete handoff.
One fresh Release build completed; two new misleading-indentation sites produced
31 repeated warnings across targets. Both were corrected, and affected rebuilds
are warning-free. The **single 135-suite production run initially passed 126 and
failed 9**. Actual async integration passed 12 cases / 246 checks.

Two real regressions were found and corrected after the frozen gate:

- A gravity-driven TOI could change a sleeping body's velocity before starting its
  motion ledger. Sleeping dynamics now have an anchored stationary trajectory;
  actual start/TOI contact participants wake before response. Deferred gravity is
  integrated once (remaining interval for TOI), without pre-impact drift. The pile
  crash/backtrace, original run and corrected impact regression remain recorded.
- Support departure ignored external acceleration and used a fixed deadband.
  Gentle swimming lift was repeatedly discarded by supported step following.
  Predicted relative motion now includes net acceleration with a travel-scaled
  contact-normal tolerance. The unchanged current swimming application improves
  from -0.079747 m to +0.138298 m upward motion; 39/39 checks pass. No fluid algorithm
  or current-game script was changed to repair this regression.

Seven failed application fixtures injected SDL desktop events despite deterministic
input isolation. They now queue the same normal M35 physical controls after
BeginFrame. Pause, locale, score, carry, prefab, fracture and save assertions remain.
The streamed bootstrap's exact body count now includes its solid and motor proxy.
A later cold-save count assertion also omitted the proxy; its original failed
20-check result and corrected 20/20 follow-up are retained. No target assertions
were dropped to obtain passing output.

Affected follow-up: 31 selected native/application runs, all pass after that recorded
count correction; this includes rigid contact/motion/impact, joints, contact cache,
CharacterMotor, events, fluid coupling, liquids, deformables, locale/audio/collision,
cold save/load and streamed gameplay. The full production gate was **not repeated**.
M65 core 62/62, authoring 20/20, application 18/18, cold write/read 19/19 and 7/7,
stream ownership 12/12. Streamed game 48/48; CharacterMotor native 32/32 and application
39/39. All 244 cookbook checks pass. TypeScript/live enumeration verifies 289 public
symbols / 27 exports / 195 native operations / 15 callbacks / 29 distinct example
files, including 3 negative drift controls.

Harness: 19/19 checks for real movement/look, two fixed steps per frame, paused
simulation, camera/scene captures and useful malformed-command failures. Editor
duplicate undo and Play/Stop preserve the authored scene exactly. Normal export
packages 25 assets / 3 scenes, 51,757,732 bytes, in about 0.16 s; its moved executable
runs from an unrelated working directory, traverses both scenes and captures pause.
Licenses are present. Generated executable packages were moved out of evidence to
`.cache`; their exact digests remain in package manifests. Redundant copied regression-project
fixtures also reside in `.cache`, with input-byte manifests; logs and captures remain. Agent-inspected screenshots
are not operator acceptance of visual quality or input feel.

## Review and scope

See [M65_INTEGRATION.md](../../M65_INTEGRATION.md) for project, controls and eight-item
human checklist. Exact files/fingerprints are in `CHANGED_FILES.txt` / `FINAL_SOURCE_SHA256.json`.
No commit/push/tag, M66, new physics engine, active physical animation, fluid algorithm
rewrite, FBX pipeline or streaming rewrite is part of M65. Protected FTFT/P1/M33–M64
history, accepted fluid algorithms and consumer originals remain unchanged. Shared
PhysicsWorld force handling adds waking; shared authored parsing/persistence extends
new normal state, rather than altering protected fluid implementation.


## Human-review Rooftop wall-run correction

The operator reported that wall-running did not work. A current Release application
reproduction found self-hits: unfiltered rays at the runner returned entity 10,
distance 0, initialOverlap true, normal +X. The +X environment ray with the normal
ignored-entity filter returned the actual tower (111), distance 0.6500001 m, normal
-X; the -X ray returned no environment hit. The original script could enter a
phantom wall-run and accelerate away from the real wall. Its capsule ledge/headroom
probes suffered the same self-overlap. This is a copied-game query integration error,
not a gravity, motor collision solver or new wall-run engine capability.

Only the current copied runner now retains `{ignored:[entity]}` and passes it to
all seven environment casts. The queryable motor remains available to other
entities' queries/sensors. No engine/runtime/API behaviour changed. Existing game
tuning, input policy, course geometry and original consumer remain unchanged.

`rooftop-wall-followup/verified/results.json`: 17 focused checks pass via normal
M35 logical input, with spawn placement/observation only in temporary project copies.
Wall-run follows actual -X tower normal, advances over 5 m, stays against the wall,
wall-jump departs away/up; open side has no phantom run; climb, vault and slide/stand
work. The initial extra vault check pressed jump too early, so it proved an ordinary
jump rather than vault; that fixture failure is preserved in the parent results/logs.
The corrected jump is within the authored vault probe's reach. No game tuning changed.

`rooftop-wall-followup/export-direct-results.json`: normal export, moved unrelated-cwd
Rooftop scene startup, actual selected-motor movement/jump, pause/resume and captures
pass. Exported runner hash matches current source; runtime executable hash is unchanged.
No broad suite or build rerun was needed. Earlier export attempts/captures are preserved:
one malformed REALTIME fixture failed usefully; queued F2/F1 attempts stayed in the lab.
Inspection found RunTestHarness calls Play.Frame but does not apply the outer
SceneSession replacement boundary, so those attempts (and the earlier handoff-smoke
transition captures) do not prove scene replacement. This follow-up uses an explicit
registered Rooftop scene argument; ordinary interactive application replacement has
its separate outer boundary. No unrelated harness/runtime correction was made here.
Original reproduction, query observations, prior source fingerprints and all
failed/follow-up records remain in that directory. Human interactive re-review of
this correction is pending.
