# M59 candidate — additive scenes and asynchronous world streaming

Starting HEAD: `6fb90d56562651d027a8740cc8d17bb655d9b8b0` (M58).
Uncommitted; human acceptance pending. No M60/M61 work.

## Implementation and scope

One played RuntimeWorld / PhysicsWorld / gravity resolver / navigation service /
QuickJS VM. Optional registered `.judasworld` manifest, independent demands,
stable region/local identities, fresh runtime IDs, qualified references and
unit-scale double-origin placements. Preparation uses real JobSystem jobs;
private per-object/native-navigation installation and publication occur at the
safe outer boundary. No frame waits for unfinished work.

Root owns session/view/locale/HUD/environment/input. JSON/physical state,
tombstones, current joint controls and idle motor velocity survive ordinary
suspension; solver warm-start impulses do not. Travelling actors/props adopt root.
Liquid/pose/articulation/native unique state and in-use supports/dependencies pin.
Departing groups are marked unloading before destroy callbacks; adoption from them
fails safely. Resource references and real body/visual slots unregister.

Coverage/authoring/limits: [component matrix and reference](../../M59_WORLD_STREAMING.md).
Public API: [scenes streaming](../../judasjs/streaming.md).
Exact scope: [CHANGED_FILES.txt](CHANGED_FILES.txt).
Fingerprints: [FINAL_FINGERPRINTS.json](FINAL_FINGERPRINTS.json).

## Validation actually executed

- One clean Release: 612.725 s, zero warnings. 115 production suites passed once.
- Actual async integration: 12 cases, 246 checks, zero failures.
- Final focused streaming: 65 checks; final real application: 37 checks.
- Cookbook: 25 executed examples / 209 checks; TypeScript 5.9.3; source/type/live
  enumeration: 23 exports, 228 public symbols, 148 native operations, 13 callbacks.
  Three negative drift controls passed.
- Affected scene replacement: 23 checks; existing ragdoll regression: 36 checks.
  Other navigation/motor/liquid/material/text checks are already in the production
  run, rather than duplicated under new labels.
- Actual navigator CharacterMotor crossed the active seam through a normal JS link;
  logical look/fire scored on a streamed physical target. Real held preparation
  ran concurrently with ordinary gameplay. Reload cancelled an outstanding read.
- Editor additive preview and Play/Stop passed; all 119 authored project files
  remained byte-identical. Native-language/audio/controller/travel acceptance
  remains operator-owned.
- Export: 52 assets / 9 scenes, 53,789,392 bytes (~51.3 MiB), 0.170 s final
  copy. Read-only moved package passed the 37 application checks from `/tmp`.
  Its executable found the adjacent project without an argument, rendered a
  screenshot after 600 fixed steps and stayed running until bounded observation
  timeout (not a claimed normal quit). Application control exercised normal quit.
- Source and moved-package composed baseline match exactly:
  `7c3196fd88d1af05de4c4a73c76e3d169b9e110f458f38ca186a600dda4f288f`.
  Missing source and stale navigation bake rejected before replacing a valid
  package; all package hashes remained unchanged.

The broad gate's exact earlier source hashes remain in `final/production/results.json`.
Narrow subsequent scheduling/readiness/retention/save and demo corrections received
only affected builds/checks. See development/followup logs; no broad rerun was done.

## Performance and honest limits

See [PERFORMANCE.json](PERFORMANCE.json), CSVs and M56 captures. Matched input
travel/backtrack was run on original range, equivalent all-resident extension and
streamed extension. Original geometry/actor population differs; it is a reference,
not an apples-to-apples speedup claim. Captures precede the final small presentation,
retained-control and teardown-order refinements. No unrelated optimisation occurred.

| Mode | Frame median / p95 / max (ms), after startup | Last fixed-step median (ms) | Max rigid bodies |
|---|---|---|---|
| Original | 32.40 / 44.58 / 64.18 | 1.99 | 45 |
| All resident | 27.37 / 74.55 / 339.12 | 2.14 | 55 |
| Streamed | 24.31 / 31.86 / 349.97 | 0.86 | 23 |
| Streamed real-time | 24.47 / 29.36 / 37.11 | 0.86 | 23 |

Streamed integration median/p95/max: 0.252/0.469/9.580 ms. The real-time probe
used 1–3 fixed steps/frame; captured frames show no step cap or discarded time.
Largest recorded indivisible unit: 7.843 ms. A 2 ms dispatch budget cannot cap a
native registration/callback/upload. Startup peaks reached ~1.1–1.9 seconds;
first water rendering cost ~340–350 ms in **both** extension modes. M56 locates
these in world/water GL submission, not a blocked region read. They are reported,
not hidden in averages or repaired by changing accepted rendering/liquid quality.

Streamed max working set: 3 active of 8 authored regions, ~650 KiB live descriptors,
14 KiB retained records, ~1.15 MiB ResourceManager bytes. Equivalent all-resident
was 7 regions, ~1.78 MiB descriptors, ~1.34 MiB resource bytes. Repeated unit visits
across 96 declarations kept one 2-body working set and bounded retained records.
Single-scene early return: 100,000 calls in 0.208 ms (sanity probe only).

Descriptor/native-overhead estimates are not total heap/RSS. Resource cache target
is soft for live references; process RSS includes engine/driver/allocator/module
state. Sequential probes showed ~267/296/316 MiB high-water RSS; no RSS reduction
is claimed. Other operator workloads were not terminated. Some probes overlapped
our focused editor/shooter checks; timings are indicative, not isolated benchmarks.
SDL offscreen actual GL was used; EGL warned about software forcing versus explicit
device selection, so no particular GPU execution mode is asserted.

## Genuine failures and follow-ups preserved

- Initial unit assumptions failed (35 checks/10 failures, then 37/2): adoption keeps
  in-use gravity and retained-pressure diagnostics must persist. Corrected logs kept.
- Actual app found prefab-flatten NextId remapping incomplete; fresh mapping count
  was repaired before publication. Original failures and follow-ups kept.
- Gate preflight refused the WorldState protection allowlist before any broad run;
  only the current wrapper's reviewed save-refusal exception was added.
- Review caught navigator spawn outside the corridor and F5 conflict with editor
  Stop. Current content uses the authored corridor and F9/F10; editor Stop unchanged.
- Applying the original shooter's fixed central-aim fixture to this different target
  arrangement produced one miss. New streaming application aimed with real logical
  look input and then fired through ordinary project JS; score/impulse checks passed.
- Focused compilation mistakes (missing algorithm include / BodyHandle comparison)
  were corrected locally; original diagnostics kept.
- Screenshot title overlap and quoted catalog line-break authoring corrected in
  project content. Reload keeps composed disk saves disabled. Suspension now retains
  current joint settings/motor velocity. Teardown prevents adoption after unload starts.

Raw large profiler/CSV captures are losslessly gzip-stored. `CAPTURE_STORAGE.json`
maps original filenames/hashes to stored captures. Disposable successful test project
copies were excluded; assertion logs, screenshots and original sources remain.
Protected historical FTFT/P1/M33–M58 evidence and prototypes were never edited.
The operator's unstaged `docs/ROADMAP.md` bytes and staging state remain exact.

## Human controls follow-up

Q incorrectly called the public `world.viewRay` property as a function, faulting
the player script and stopping controls. Corrected project JS; first F9 now requests
the next gallery rather than its already-loaded initial gallery. Extended actual
input checks pass **48/48**, both source and moved read-only export. No engine
behaviour changed and no broad rerun was performed. Original failure, narrow
fixture correction and fingerprint refresh are in
[follow-up results](followup/human-controls/RESULTS.md). Earlier 37-check results
and baseline remain historical; current package is `/tmp/m59-controls-game-moved`
and current composed baseline is `0cd5dbd913522195ab9f6748a0da519cbb3cb42127b41a613bfcf0c32cc6dd79`.

## Human backtracking follow-up

Actual full-range travel exposed repeated snapshot serialization starving the
2 ms dispatch budget, and body-less teardown invalidating unrelated world-anchored
hinges. Corrected M59 accounting/teardown; surviving hinges also exposed a doubled
placement transform, now corrected including snapshot-local conversion. Final
proof: **71 unit / 48 application / 7 repeat-travel checks**, all passing; source
and current moved export passed the application checks. Original human/automated
failures and narrow follow-ups are preserved in
[backtracking results](followup/backtrack/RESULTS.md). No physics solver or public
API change; no production-suite repetition. Current package:
`/tmp/m59-backtrack-game-moved`. Human re-test pending.
