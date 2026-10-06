# M63 candidate — structural fracture and physical fragments

Starting HEAD/main/origin/main: `29845bbdb911d2129a2d2b995fed22a1448594e7`.
No commit, push, merge or tag. Human visual/physical acceptance remains pending.

## Implementation and source authority

[Current architecture, authoring, controls and limits](../../M63_FRACTURE.md).
[Public API](../../judasjs/fracture.md).
[Exact changed paths](CHANGED_FILES.txt).
[Final source/asset SHA-256 records](final-source-fingerprints.json).

The accepted M62 solver, render mapping, resources, attachments and save participant
were extended. A version-2 optional cook and one irreversible part/bond graph support
both cohesive deformable cells and ordinary M45-connected rigid cells. No second
physics solver, health-based failure, mesh Boolean package or dependency was added.
All original games, fluid projects/solvers, protected research and historical results
remain unchanged. The shared joint changes record signed solved reaction impulses;
they do not change joint row solving. No global fingerprint change (schema **5**).

## Human input follow-up (current)

[Windowed capture and static-click correction](input-followup/RESULTS.md) supersedes
previous input behaviour: native application 12 checks, input 40, refocus/pause 11,
and moved-project input 12, all passing. Original reproduced failure is retained.
A focused Release rebuild and exported shipping smoke cover the narrow correction;
no second full production run. The original broad run remains earlier evidence,
not a claim that these late Window.cpp changes were present in that run.

## Final focused proof

[Final focused follow-up](review-complete/RESULTS.json) is the final focused result, not the
earlier development/candidate runs. All 18 commands passed; the later four-case authored-beam proof and native export
probe below supersede the earlier beam fixture settings without changing engine/project content:

| Check | Assertions |
|---|---:|
| Partition, cohesion, graph, mass, rotation, removal and invalid cooks | 33 |
| Rigid publication/motion, attachment, required-save guards and lifetime | 35 |
| Supported load / support release / physical redistribution | 17 |
| Uniform / radial / fixed-origin `1e12,2e12,-3e12` application probes | 23 each |
| Cold-process save / fresh-process load and next physical break | 22 / 22 |
| Travelling family, visible pin, adoption, true unload and revisit | 20 |
| Scene replacement/reload and independent instance cleanup | 17 |
| Real hole, audio obstruction, navigation and CharacterMotor passage | 19 |
| Moved-package probe / lifetime / game | 23 / 17 / 19 |
| Executed JudasJS cookbook | 236 |

TypeScript 5.9.3 and API drift/live enumeration pass: **27 exports, 275 public
symbols, 181 native operations, 15 callbacks**, with three negative drift checks.
Editor create/rebake/import reports rigid/soft/partition PASS; authored scene after
Play/Stop is identical. The editor test uses a copied project, not original assets.

The tiny unit-area loaded specimen's nominal force/area is 5 Pa at 5 N and 400 Pa
at 400 N. The conservative finite-iteration observed peaks are 12.005 Pa at 5 N,
758.478 Pa at 400 N (4 substeps × 4 iterations), and 973.530 Pa at 400 N (8×6).
Thus both settings distinguish survival/failure against 80 Pa, but the high-load
peak differs by **28.4%**. This is sensitivity evidence, not engineering calibration.
Freefall and rotated rigid motion do not create fictitious support damage.

[The authored-beam final proof](authored-beam-final/RESULTS.json) removes the earlier
fixture’s yield override and uses the actual 5 N/120 N demo loads; uniform/radial,
cold write/read and native moved-package probe all pass. The real plastic beam starts intact at about 804.8 Pa under its own supported load.
The low 5 N transverse load retains plastic deformation without fracture; 120 N
breaks it at about 1582.7 Pa (radial 1602.1 Pa) against a 1500 Pa threshold. Remaining soft material
keeps its current shape/rest state. The frame holds a modest load and fails from
actual redistribution after a support is released and sufficient load is applied.

Unforced rigid fitting uses 108 kg of partitioned material: COM error
`1.4211e-14 m`, linear momentum error `2.5789e-6 kg m/s`, angular momentum error
`2.3317e-5 kg m²/s`. A subsequent explicit complete cut changes neither positions
nor motion nor total material mass. This does not claim total-energy conservation.

Cold-process restoration compares the complete deformable participant bytes,
including plastic state, tombstones and owned fixed-joint warm history, then
resumes dynamics and physically breaks a surviving connected frame. Capture before
restored JS initialization correctly defers. No break notification is replayed.

## Failures and corrected follow-ups

[Core development records](development/core-followups.md),
[application records](development/application-followups.md), and
[measurement environment/capture correction](development/performance-capture-note.md)
preserve genuine failures. Earlier runs are development evidence, not final gates.
These include the initially under-strength beam, a missing audio listener, harness
scene/save assumptions, an excessive performance impulse, and the final malformed
walkway content record. No historical result was overwritten or weakened.

## Clean Release and production gate

The single clean Release build completed with **zero warnings**. All **130
production suites** passed, plus actual async integration **12 cases / 246 checks**,
editor async Play/Stop and the blocking reference. See
[original gate](final/production/results.json).

Its source-freeze guard is deliberately preserved as **false**: safety assertions,
two narrow removal/disable guards and timing-window fixes were added during the
run, and the final H-control proof was added after compilation. The changed engine
objects were compiled after the corresponding edits; all production binaries
therefore contained those engine guards. A final incremental Release build rebuilt
the keyboard-driven application proof with zero warnings; final focused execution
covers the final source/content. See
[exact source/object records](development/source-freeze-followup.json).
No failed run was converted into a fabricated green original gate, and no second
unrelated production run was performed. The original protected-path guard passed.

## Performance and shipping proof

Intel i7-4790 (4 cores / 8 threads), GTX 970, native NVIDIA GLX 580.178.04;
software capture is Mesa llvmpipe 26.0.8 (separately probed). Runs are sequential,
after the build/production gate. Load averages are recorded in each JSON. No other
Judas process was terminated; the package check closes only its own window.
Application assertion hosts use one 1/60 step/frame, no vsync and a 1 ms resource
wait. These are measured costs, not claims of interactive monitor FPS.

Matched rigid workloads: baseline **7 bodies / 12 draws**; other cases **90 physical
cells, 97 bodies, 720 nodes, 540 tetrahedra, 120 interfaces**. Intact has 0 broken
interfaces/32 draws; failing and debris have 56 broken/52 draws. All retain ordinary
contacts, joints, main/secondary/shadow submission. No debris freezing or distance
skip. “Settled” means the later debris window, not a sleeping rigid island.

| Native case | Frame median / p95 / max ms | Fixed step median / p95 / max ms |
|---|---|---|
| No fracture | 1.780 / 2.100 / 2.175 | 0.138 / 0.229 / 0.263 |
| Intact, 90 cells | 7.174 / 24.170 / 37.436 | 4.633 / 21.472 / 34.873 |
| Failing, 90 cells | 29.427 / 61.102 / 103.866 | 26.670 / 58.630 / 100.776 |
| Later debris, 90 cells | 17.637 / 38.633 / 54.634 | 15.017 / 35.860 / 52.267 |

[Native measurements](performance-native-final/RESULTS.json) and
[software measurements](performance-software-final/RESULTS.json) include exact
windows, scope distributions, resident memory and startup. Software frame medians
for the same four cases: **15.15 / 23.79 / 46.00 / 36.37 ms**; fixed medians:
**0.187 / 4.769 / 27.011 / 17.895 ms**.

The native first-fracture frame reaches **103.866 ms**, with **99.076 ms** in the
existing contact/joint solver; broadphase max is 0.051 ms and contacts max 1.377 ms
in that failure window. The largest summed topology commit is **0.0122 ms**. Initial
90-cell preparation/publication is **8.856 ms total**, largest single-family
operation **1.153 ms** (a 9-cell family). Initial resource/GL startup varies
**156–389 ms** across native cases; the intact retained-frame maximum, including
initial preparation/upload, is **69.626 ms**. These costs are not hidden behind
warm medians. General awake rigid contact/constraint cost limits bigger scenes;
M63 does not implement a sleep rewrite or silently remove material to reduce it.
Native case process RSS maxima are about **132–204 MiB**; they include engine,
assets/driver caches and are not isolated fragment allocations.

CPU-only soft fixture (4×4, self-contact explicitly off): 30 cells / 240 nodes /
180 tets / 29 interfaces has intact/failing/later medians **0.869 / 0.939 / 0.912 ms**;
90 cells / 720 nodes / 540 tets / 89 interfaces: **2.664 / 2.921 / 3.019 ms**.
See [exact CPU log](final/production/judas_fracture_performance_tests.log).
Known retained numeric instance storage is **41,555 / 125,435 bytes**, excluding
shared cooks/maps/allocator overhead. Steady samples allocate zero times; fracture
allocates four times / 60 bytes in this fixture. This is not a total-world memory
claim or soft-contact benchmark.

[Final moved export](review-complete/RESULTS.json): **18 registered assets,
4 scenes, 50,710,140 bytes**, exported in about **0.09 s**. Package contents are
checked against source (payload bytes and stable ID/type, with normal package-relative
sidecar canonicalization); raw partition authoring input and assertion hosts do not
ship. The unmodified shipping executable runs from unrelated `/tmp` in a read-only
package and closes normally, with package file hashes unchanged:
[shipping/native proof](package-final/RESULTS.json) and
[content/identity comparison](PACKAGE_CONTENT_VERIFICATION.json). Last 120 shipping frames:
**9.944 / 10.116 / 10.174 ms** median/p95/max (about 101 frames/s); fixed step
**0.943 / 2.036 / 4.096 ms**. Startup **191.178 ms**. The native physical-impact
assertion capture has frame median 5.967 ms/p95 15.431 ms/max 55.580 ms and fixed
median 3.418 ms/p95 11.128 ms/max 19.539 ms. Human interaction remains authoritative.

## Review controls and limitations

Open `projects/fracture_lab/fracture_lab.judasproj`. **1/2/3** choose uniform,
radial and the barrier game; WASD/mouse and Space move; click applies one actual
hit-point impulse. **B** cycles low/high/off beam load; **V/R** load/release the
supported frame; **G/T** carry/drop/throw; **P** spawns an independent prefab;
**C** explicitly removes the aimed rigid cell; **F3/H** request/release the annex
and adopt its family to root; **F5/F8** save/load; **F9** reload; Esc pauses.

Bounded prepartitioned cracks only (256 cells, 1024 interfaces). Box-cell proxies;
no general concave mesh collision or arbitrary remeshing. Rigid cells stay joint
assemblies and remain awake under current physics; this is not an island-sleeping
rewrite. Soft pieces retain M62's limited contact/query matrix: no general M44,
CharacterMotor blocking or M42 soft events. Rigid skeletal supports and runtime
support additions are unsupported. Strict content saves and existing quotas apply.
Live region families pin until removed/adopted; the retained state is never called
unloaded. Navigation uses an independent baked floor and ordinary obstacle, with
0.5 s repath and approximate debris clearance; fractured-floor rebaking is absent.
Liquid-bearing fracture combinations reject rather than delete conserved liquid.

## Human checklist

1. Hit different panel locations; inspect local separation, brown interiors/shadows.
2. Use B: observe yielding before high-load failure and continued soft deformation.
3. Load the supported frame, release one support, inspect its physical response.
4. Carry/throw rubble; in scene 3 open the barrier and observe passage, courier and sound.
5. Repeat in radial/oblique gravity and spawn another independent family.
6. Request the annex, break material, adopt with H, release/revisit with F3.
7. Save, wait for completion, quit, load in a fresh process, then continue breaking.
8. Confirm reload/Stop cleanup and responsiveness; repeat in the moved standalone.

Proposed future commit: `Add M63 structural fracture and physical fragments`.
