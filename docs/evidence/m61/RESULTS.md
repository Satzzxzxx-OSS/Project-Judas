# M61 candidate results

Starting/current HEAD: `a9c6cd780b8c93db6355b791c6fdb34afec7cf53`.
Candidate complete; human visual/interactive/listening acceptance is pending. Nothing committed, pushed or tagged. No M62 work.

## Contract and scope

See [participation matrix and lifecycle](../../M61_SAVES.md), [public API](../../judasjs/saves.md), [declarations](../../judas.d.ts), [inventory](../../judasjs/API_INVENTORY.md) and [cookbook](../../judasjs/examples/saves.js).

Eleven required versioned chunks: entities, motors, animation, articulation, navigation, audio, liquid, contacts, scripts, session and streaming. Normal scene/component construction and M59 qualified identity are reused. There is one project service, one active operation, immutable job products and one non-ticking restore candidate. Saves capture a later safe outer boundary; they do not continually serialize idle worlds.

Changed APIs: public `saves` namespace; constructor `context.restored` and `restore(dt)` lifecycle. Generation-aware body ownership prevents old-world handles aliasing new native slots. No game-specific native save manager or new simulation algorithm. The ordinary Range scripts retain gameplay policy and reconstruct their HUD/controls after load.

## Validation actually executed

| Check | Final result / evidence |
|---|---|
| Clean Release | One clean configure/build succeeded. One new test indentation warning recorded, then removed in the affected narrow build; final affected builds have zero warnings. |
| Production | 124 suites executed once: 120 initially passed, four failed. Original `final/production/results.json` remains failed. Camera/prefab regressions and boot-save preparation were corrected; all four affected suites then passed narrowly. No broad rerun or rewritten gate. |
| Actual async | All 12 cases / 246 checks passed in the original gate; source/protected-path checks passed. |
| Storage | 28/28, `review-final/storage.log`. |
| World reconstruction | 27/27, `resting-hinge.log`; includes pending force, safe handles, boot-before-fixed, navigation link, Stay rather than Enter, moving/rotating support and resting/hinged first-three-step comparisons. |
| Single-scene fresh process | Write 13/13, load 20/20. Final refreshed load in `input-edge-corrected/lab-read.log`. |
| Actual streamed game | Write 11/11; final independent load/read-B each 19/19. `gameplay-corrected/` and `input-edge-corrected/` include actual earned score, defeat, spawn and held-confirm proof. |
| API/tooling | 217 cookbook checks; 25 exports / 248 public symbols / 162 native operations / 14 callbacks / 26 examples; TS 5.9.3, live enumeration and three negative drift checks pass. `review-final/api.log`, `cookbook.log`. |
| Narrow shared regressions | Project, prefab, prefab Application, render camera, audio world and audio Application all pass, `narrow-regressions/RESULTS.json`; input 40/40 in final follow-up. Related liquid, animation, motor, streaming and presentation suites passed in the original gate. |
| Editor | Both isolated projects completed normal Play/Stop; authored scene IDENTICAL, save OK, jobs zero failures. Save Lab log is `review-tail/editor-save_lab.log`; Range log `editor-followup/editor-streamed_range.log`. An initial runner misread stderr as stdout; the passing original log was retained and not rerun ceremonially. |
| Final exports | Both packages exported, moved, made read-only and exercised from unrelated `/tmp`. Fresh write/load and second-slot/backtrack proofs pass, then normal unmodified shipping startup and owned-PID window close return 0. `packages-review/COMMANDS.json`. Package hashes unchanged; no user saves or assertion hosts in shipping trees. |

All genuine failures and corrected follow-ups remain in [FOLLOWUPS.md](FOLLOWUPS.md). Large raw profiles/CSV are losslessly gzip archived; [COMPRESSED_EVIDENCE.json](COMPRESSED_EVIDENCE.json) records original hashes and byte sizes. Original command paths remain as recorded; append `.gz` when inspecting archived outputs.

## State demonstrated across complete process exit

- Save Lab: real 3 L paired transfer into tilted cup, real disturbed M55 cells/face flows, 4 L detached parcel, active three-body articulation/impulse, crossfade elapsed 0.4 s and persistent streamed FLAC sought to 4 s. The read process compares exact liquid owner/cell/flow/parcel archive bytes before resume, all physical positions within 1e-5 m and correct native counts (44 bodies, nine owners, one parcel). Ten resumed steps retain the accounting tolerance, healthy waves, finite articulation and advancing mixer. FLAC cursor is within 10 ms; current machine master gain 1 remains current rather than restored 0.19. Audio tests use a deterministic no-device backend, not human listening.
- Streamed Range: actual logical look/fire earns 250 and defeats a navigator; N creates a separate prefab. Qualified `gallery-0:30` prop is adopted into root without identity change. An altered hinge/target is destroyed, another physical prop changed, then gallery-0 actually unloads. Save contains four active regions plus 135220 retained bytes; other regions remain baseline/suspended. Fresh load restores German locale, game score/timer/defeat/spawn and traveller, revisits only the requested gallery, retains tombstones and changed pose, avoids duplicate adopted props, then saves/loads a separate B slot in another process.
- The game-state proof exposed content issues: previously unhandled authored N action, and a defeated visual cue scaling a motor root. These are corrected in ordinary JS (N spawning and dark material feedback), without changing motor/physics rules.
- Corrupt/newer/truncated/missing participants reject before publication; incompatible candidate leaves original world playable. Queued cancellation and busy save/scene conflicts are exercised. Isolated filesystem tests cover two slots, Unicode, previous-good recovery, path/symlink rejection, locks, abandoned temporaries, controlled pre/post-publication errors and child exit at five write boundaries. These are not physical power-cut certification.

## Measured cost and memory

Seven cycles per workload. Values are **median / p95 / max milliseconds**; with seven samples nearest-rank p95 equals max. This is SDL offscreen GL (software requested) and deterministic no-device audio, not an interactive desktop FPS claim. Measurement uses zero-dt capture frames and one normal first-resumed frame. No offline-time catch-up.

| Workload | Capture pause | Background save incl sync/list | Background load | Preparation elapsed | First resumed frame |
|---|---:|---:|---:|---:|---:|
| One-body simple | 0.067 / 0.111 / 0.111 | 98.654 / 107.904 / 107.904 | 7.188 / 8.200 / 8.200 | 0.256 / 0.411 / 0.411 | 1.414 / 1.896 / 1.896 |
| Liquid/pose/audio Lab | 6.117 / 8.366 / 8.366 | 121.227 / 135.803 / 135.803 | 15.308 / 21.173 / 21.173 | 101.789 / 106.021 / 106.021 | 48.459 / 71.879 / 71.879 |
| Resident streamed boot | 7.889 / 8.569 / 8.569 | 108.762 / 373.680 / 373.680 | 13.571 / 15.413 / 15.413 | 106.570 / 113.005 / 113.005 | 21.827 / 24.181 / 24.181 |

The save worker is not hidden in the capture figure: durable synchronization dominates. One streamed directory-sync outlier was 255.932 ms on the worker, contributing to the 373.680 ms save-worker max. Owner preparation is not free either: the Lab private-preparation scope reached 39.029 ms; its first resumed offscreen frame reached 71.879 ms. No extra optimization/research was started to hide these costs.

| M56 scope | Simple median/p95/max | Lab median/p95/max | Stream median/p95/max |
|---|---:|---:|---:|
| Save encode | 2.234 / 2.835 / 2.835 | 2.903 / 6.841 / 7.353 | 2.297 / 3.596 / 3.596 |
| Save read | 2.281 / 2.951 / 2.951 | 2.885 / 4.688 / 10.173 | 2.657 / 3.560 / 5.274 |
| Save file sync | 30.077 / 46.046 / 46.046 | 29.537 / 52.242 / 52.242 | 22.997 / 51.576 / 51.576 |
| Save directory sync | 16.860 / 32.775 / 32.775 | 18.655 / 61.588 / 61.588 | 22.647 / 255.932 / 255.932 |
| Save private preparation | 0.033 / 0.066 / 0.066 | 35.377 / 39.029 / 39.029 | 7.210 / 7.686 / 7.686 |
| Save activation | 0.028 / 0.101 / 0.101 | 2.135 / 3.933 / 3.933 | 0.749 / 0.950 / 0.950 |

These raw M56 samples are from its bounded 120-frame retained history, deduplicated against spikes; counts may exclude early cycles. Encode/read include validation and metadata/recovery work, not only the selected file. Worker and nested scopes overlap; do not sum inclusive durations. [PERFORMANCE.json](PERFORMANCE.json) records counts, all status samples and native observations.

Immutable snapshot estimates / files: simple 2201 / 1316 B; Lab max 89961 / 89075 B; boot stream max 24817 / 23937 B. The changed/nonresident gameplay proof is ~66 KB (final exported 66064 B), four active regions / 135220 retained bytes. Simple has one entity/body and no liquid; Lab has 56 entities / 41 bodies / nine owners during measurement, stream 27 / 10 / zero owners. The active ragdoll proof is separate and has 44 bodies.

Idle service Advance was ~3–7 ns in a 1000-call sanity loop; an unused service stays unallocated and the outer path performs only a null check. It does not run liquid/streamed serializers. This is a sanity measurement, not a nanosecond accuracy claim.

Seven cycles keep entity/body/owner/voice counts identical (Lab one voice, stream five), running jobs 0–1. RSS includes graphics/fonts/audio/cache and M56 history: measured peaks ~214 MiB simple, ~271 MiB Lab/stream. Lab/stream warmed by cycle 4; last-three RSS growth was ~0.11 / 1.06 MiB. This short native-count/cache plateau is not an unlimited leak soak. Snapshot state is bounded to 8 MiB combined; one private world shares normal immutable resources. RSS is not a claim of exact isolated restore allocation. Files bound 64 MiB, archive vector allocation budget 64 MiB per archive, explicit participant/entity/count bounds; no process-memory dump.

Final read-only shipping package sizes: Save Lab 65896085 B; Range 63257477 B. Both contain only normal runtime package content, not these assertion executables or real user slots.

## Current limitations / policies

- Strict all-registered-content hash: even an unused changed asset can reject an old slot. Container 1 seconds->milliseconds migration is tested; automatic game-data/content migration is not provided. Game-data version 1 is explicit.
- Legacy `.judasstate` is a separate preserved delta path. Modern slots require legacy-gameplay false and reject unsupported required legacy PBF/atmosphere/combustion/vehicle/door/switch state by entity/component. No implicit import, silent omission or double overlay.
- One active request; 32 statuses, 128 slots, 8192 reconstructed entities, 4096 parcels, 30-second preparation. Cancellation only queued/preparing. A running detached write may finish after caller/project shutdown: wait for completed before intentional exit.
- New projects mint stable random save identity. Equal legacy name/filename identities share a namespace unless an explicit identity is authored. Copies of an identity intentionally represent one game.
- Particles, one-shots, DSP tails, UI focus/cursor and GPU/corridor/cache objects are transient/derived. Meaningful liquid/pose/audio/control state is preserved. No waveform phase, bit-identical indefinite physics, arbitrary JS-heap migration or post-callback rollback promise.
- Saved script state installs before restore hooks. Hooks/modules must reacquire resources and avoid unconditional new-game effects; arbitrary project side effects cannot be rolled back after publication. Save does not silently add editor compatibility for unsafe scripts.
- Pinning remains M59 runtime policy; disk persistence does not remove liquid/articulation pins. Nonresident unchanged regions are not instantiated for capture/load.
- Storage is supported local Linux, descriptor/no-follow paths with fsync/rename/directory sync and validated previous generation. Checksums detect corruption, not authentication. Late errors distinguish prepublication failure from published-but-durability-uncertain.

## Operator handoff

Local projects: `projects/save_lab/save_lab.judasproj` and `projects/streamed_range/streamed_range.judasproj`.
Final moved shipping packages: `/tmp/Judas_M61_save_lab_packages-review` and `/tmp/Judas_M61_streamed_range_packages-review`. Launch their `judas` from any directory; these directories are read-only. Separate assertion copies are test-only.

Runtime slots: `$XDG_DATA_HOME/judas/games/<identity>/Saves/Slots` (fallback `$HOME/.local/share`); editor uses `EditorSlots`. Automated tests used `.cache/m61-final-data`, isolated from real player slots. Human listening/gameplay acceptance remains pending.

### HUMAN SAVE CHECKLIST

1. Open Range; earn score/defeat an enemy, N spawn, L choose locale. Escape: create A/B, inspect name/status, overwrite one (confirm).
2. Q carry/adopt a prop; change a target/hinge, walk away until its gallery is genuinely unloaded. Save and wait for completed.
3. Save Lab: V disturb water, G/P scoop/carry/tilt bucket, T leave water in flight; B ragdoll/M crossfade/K seek music. Save without resetting.
4. Quit completely; relaunch; Load. Check score/locale/defeat/spawn/traveller, revisit gallery, inspect water/pose/music and continue playing.
5. Load via mouse/controller; confirm no unintended shot/jump, pause/pointer are usable; save/load B and delete only the chosen slot (confirm).
6. Repeat using the moved package from unrelated CWD. Human visual/interaction/listening acceptance is yours.

## Review records

Exact changed paths: [CHANGED_FILES.txt](CHANGED_FILES.txt). Final implementation/docs/assets fingerprints: [FINAL_SOURCE_SHA256.txt](FINAL_SOURCE_SHA256.txt). Preservation checks and gate/follow-up map: [RESULTS.json](RESULTS.json). Compressed artifact integrity: [COMPRESSED_EVIDENCE.json](COMPRESSED_EVIDENCE.json).

Proposed future commit: `Add M61 game save slots and world persistence`.

No commit/push/tag. HEAD/main/origin/main remain at the starting checkpoint. Protected FTFT/P1 prototypes/evidence, historical M33–M60 evidence, accepted fluid algorithms/projects and unrelated files are unchanged. No ROADMAP created; no M62. Only runner-owned child/window PIDs were closed; Claude/operator processes were not touched.
