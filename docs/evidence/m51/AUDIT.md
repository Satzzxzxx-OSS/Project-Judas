# M51 C++ behaviour inventory and adjudication

Authority: actual source at M50 checkpoint
`74da7b534831d7cb7043ed1c02970283f7078daf`, then the candidate changes.
Historical evidence has not been rewritten.

Systematic searches covered gameplay words, Action consumers, request consumers,
force/control policies, name/scene comparisons and demo IDs. The policy-word
search returned 208 matching lines in 31 C++ files (not 208 independent behaviours).
Manual call-path inspection followed GameSession → InteractivePlay/Simulation →
physics/presentation, project/editor startup and scene replacement, and the public
QuickJS dispatch. Additional asset/input/query/pose services were checked as
consumers of these paths. The following 35 rows are the reviewed behaviour groups.
Names alone were not used as classification evidence.

| # | C++ behaviour and source mechanism | Why it exists | Classification | Action / justification |
|---|---|---|---|---|
| 1 | `PlayerController::FixedUpdate/UpdateFrameInput` speed, air control, launch, swimming | historical playable character | LEGACY COMPATIBILITY | no spawn or locomotion in scripted worlds; M49 JS motor controller reused |
| 2 | `CharacterMotor::Step`, `WorldCharacter.cpp` support/sweep/step | collision-aware motion independent of intent | ENGINE PRIMITIVE | retained, no input polling or gameplay states added |
| 3 | `GameSession::HandleFrameInput` G/F/H/V/X/R/Z/Y/T/C choices | historical game controls | GAME BEHAVIOUR | compatibility gate; current input meaning in ordinary JS |
| 4 | `GameSession::SpawnPersistentEntity` crate definition | demo creation shortcut | LEGACY COMPATIBILITY | never implicitly called in scripted mode; JS prefab spawn instead |
| 5 | `GameSession::DestroyTargetedEntity` held/targeted eligibility | demo delete policy | LEGACY COMPATIBILITY | JS query/tag + safe destroy in current demo |
| 6 | `InteractionSystem::SelectInteractable` and PickupInteractable | historical selection/range/meaning | LEGACY COMPATIBILITY | modern GameSession has no interactables; queries/tags + JS select |
| 7 | `Door::Interact/FixedUpdate` open-state and prescribed panel angle | old door demo | GAME BEHAVIOUR | legacy gate; current physical hinge spring chosen by JS |
| 8 | `LightSwitch::Interact/FixedUpdate/LampOn` | old switch/lamp demo | LEGACY COMPATIBILITY | retained only for old component data; no new switch framework |
| 9 | `ObjectManipulation::PickUp/Drop/Throw` whitelist/range/8 m/s | M18 meaning | GAME BEHAVIOUR | JS eligibility/drop/impulse; old path inactive in scripted worlds |
| 10 | `ObjectManipulation::ApplyCarryForce/OrientationTorque` PD constants | historical holding feel | GAME BEHAVIOUR | JS finite PD uses real mass/inertia; physical force/torque stays engine |
| 11 | `PilotControl::TryEnter/Leave/AdvancePlayerForPiloting` F, support gating, clearance | riding/piloting demo | LEGACY COMPATIBILITY | current control/seat/exit policy JS; old historical dismount retained |
| 12 | `PilotAttachment` frame position/orientation and velocity math | local/world reference conversion | GENERIC ENGINE SERVICE | mathematics retained; it no longer implicitly attaches modern characters |
| 13 | `FlyingPrimitiveControl` thrust/torque/SAS target/gains | M11/M21 flight policy | GAME BEHAVIOUR | ordinary JS body force/torque controller; old vehicle binding absent in scripted sessions |
| 14 | `Simulation` M20 prograde/retrograde/radial Action forces | orbital operator controls | LEGACY COMPATIBILITY | compatibility-gated; celestial mechanics unchanged |
| 15 | `PlayerView/ApplyPlayerViewToggle`, InteractivePlay main view | historical FPS/third-person choice | GAME BEHAVIOUR | current world.setView and V policy JS; identity view if modern project has no view |
| 16 | `WorldPresentation` torch/ship light colours/ranges, player model | old showcase presentation | LEGACY COMPATIBILITY | legacy player/light rig gated; normal authored lights/render cameras retained |
| 17 | `PauseMenu`, UIWidgets resume/options/HUD/quit | pre-M41 game menu | LEGACY COMPATIBILITY | fallback only legacy; modern menu M41 asset + JS |
| 18 | `InteractivePlay` F6/F7 persistence shortcuts | historical operator controls | LEGACY COMPATIBILITY | key meanings gated; serialization/persistence engine service retained |
| 19 | `SceneSession::Request/Apply`, GameSession R handling | scene replacement mechanism + restart policy | GENERIC ENGINE SERVICE | queued mechanism retained; R/menu choices JS, legacy shortcut gated |
| 20 | `Window::Init`, InteractivePlay/Editor StartPlay cursor choices | historical automatic mouse look | GAME BEHAVIOUR | request in JS; SDL capture implementation and editor camera capture remain generic/editor-owned |
| 21 | `Window` SDL collection and Action/request adapters | device normalization/older consumers | GENERIC ENGINE SERVICE | raw collection retained; compatibility adapter no longer drives modern gameplay |
| 22 | `InputSystem` logical map/edges/fixed-step handoff | multiple intent consumers | ENGINE PRIMITIVE | retained; no physical key polling in modern game C++ |
| 23 | `Simulation` B fluid particle insertion | manual old water demo control | LEGACY COMPATIBILITY | gated; accepted production solver/coupling untouched |
| 24 | `Simulation` held 18 kW heater position/C choice | combustion showcase policy | LEGACY COMPATIBILITY | inaccessible through modern GameSession controls; combustion machinery retained |
| 25 | `ProductionFluidCoupling::PrepareRigidStep/AdvanceResolvedLiquid/SampleField` | accepted physical approximation | ENGINE PRIMITIVE | unchanged; JS swimming consumes existing sample, not a new solver |
| 26 | `PhysicsWorld` forces, inertia, broadphase/contact/query/constraint paths | authoritative physical state | ENGINE PRIMITIVE | retained; new readonly mass/inertia bindings expose existing data |
| 27 | `RuntimeWorld` Create/Destroy/SpawnPrefab and generation validation | ordinary entity lifetime | GENERIC ENGINE SERVICE | retained; project tags/IDs are data, no privileged demo entity |
| 28 | `RuntimeWorld/WorldState/SceneFingerprint` baseline/save reconstruction | persistence identity | GENERIC ENGINE SERVICE | legacy hash unchanged; script execution mode domain-separates effective baseline, no scene schema bump |
| 29 | `RuntimeWorld` fidelity focus/pins, coarse/celestial stepping | authored scalable simulation service | GENERIC ENGINE SERVICE | current view focus and all motor support pins; held-object policy remains legacy; no new distance game rules |
| 30 | `RuntimeOptions` environment classic/terrain scene selectors; TestHarness taps | historical regression entry points | LEGACY COMPATIBILITY | already explicit no-argument test selection; ordinary explicit project/export startup unchanged |
| 31 | `TerrainDemo/TerrainLibrary` named procedural terrain registry | authored terrain source/reference fixture | GENERIC ENGINE SERVICE | registered surface geometry, not object-name physics or flat-water substitution; retained |
| 32 | `GameplayHud/HUD/RuntimeDiagnostics/WorldDebugView` labels and measurements | operator/debug information | GENERIC ENGINE SERVICE | debug overlays may remain engine-owned; new project hides old gameplay diagnostic HUD through authored UI |
| 33 | `editor/EditorCamera/EditorPanels/ComponentEditors` editor keys and historical component creation | editing/compatibility | GENERIC ENGINE SERVICE | editor-only shortcuts retained; legacy authoring explicitly labelled, new-project execution defaults scripted |
| 34 | `ScriptSystem` input/physics/UI/scene/animation/ragdoll/character bindings; resource/audio/Renderer services | public reusable engine operations | ENGINE PRIMITIVE | retained; only mass, inertia tensor and capture intent added, no openDoor/throw/enterVehicle APIs |
| 35 | `Scene/serialization/prefab` display names, Skeleton/clip names, classification registry names | stable authored identity, metadata and lookup | GENERIC ENGINE SERVICE | name matches are identity/registry lookup, not scene/object-name physical policy; no such branches introduced |

## Boundary findings

The primary demonstrated leak was that a `world.view` suppressed only legacy
locomotion. GameSession still ran pickup/throw/vehicle/reset/spawn policy and
Simulation still ran old demo controls. A script main camera was therefore not
a sufficient execution boundary. The project compatibility setting isolates
all those policy dispatches even when no script view exists.

An API gap was demonstrated by the old finite carry and SAS mechanisms:
mass-scaled forces and world inertia torque require mass/inertia data. Exposing
those snapshots preserves their mechanism without exposing a HeldObjectManager.
Cursor capture also encoded an unconditional game assumption in window startup;
a boolean intent request moves that choice to the project while keeping device
and modal UI handling in C++.

Engine support mechanics, gravity, frame conversions, contacts, constraints,
query geometry, fluids, pose resolution, skinning, resources, parser, audio
mixing and UI layout stay C++. Project code composes them; it does not reimplement
collision or the renderer. Legacy showcase controls are explicitly retained for
old projects/evidence, not claimed to be universally removed from the tree.

## Source ownership evidence

Before: GameSession input dispatch; ObjectManipulation physical controller;
FlyingPrimitiveControl SAS/thrust; Door and PlayerView policy; Window auto-capture.
After: their dispatch is compatible-only, while
`projects/boundary_demo/Assets/scripts/controller.js` and `rules.js` execute via
normal registered assets/QuickJS relative modules. Bodies are ordinary body
components, the door is an ordinary M45 hinge, and UI is ordinary `.judasui`.
No internal dispatcher calls, hidden demo registration or additional RuntimeWorld
is used. Current project stable IDs/tags remain project data.

Final validation/fingerprints and preserved genuine development failures are in
this directory. Human visual/interaction review remains separate.
