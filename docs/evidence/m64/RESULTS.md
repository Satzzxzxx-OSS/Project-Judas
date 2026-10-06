# M64 candidate evidence

Starting checkpoint: `b5676438ed12d9cb3d05c634eaf5f578d7216ada`.
Uncommitted candidate; human visual/gameplay acceptance is **pending**.
Current reference and controls: [M64_COLLISION.md](../../M64_COLLISION.md).

## Evidence boundary

The supplied M60 skate audit/journal and Claude's independent workspace are
unchanged. The raw skate project/reproduction is absent from this checkout. The
current original ramp/chassis reconstruction is a bounded comparison, not a claim
that Claude's original game was rerun. `BASELINE.md` records the pre-extension
pair/query/consumer matrix. Genuine development failures and follow-ups are kept
in `development/`, `preflight/`, `preflight-native/`, and `candidate-focused/`.

`CANDIDATE_SHA256.json` records the participating files frozen before the clean
Release gate. All recorded hashes matched until the build timeout. The resumed snapshot
`CANDIDATE_RESUMED_SHA256.json` differs only in the runner build-time allowance,
changed after that gate stopped; implementation/assets remain identical. One Python bytecode
cache was accidentally included by that initial file enumeration; it is not
source or a deliverable and will be excluded from the final source manifest.
The original snapshot remains as evidence, rather than silently rewriting it.

## Resolved audit boundaries

- **E-03:** imported static concave mesh contacts/casts, local BVH, physical
  adjacency, convex hulls, independently rotated compound children and a shared
  editor/CLI cook path. One mesh remains one static body. Internal coplanar edges
  cannot provide a false solver barrier. This does not certify lossless traversal
  of coarse faceted curves or eliminate every game-side skate assistance.
- **E-04:** bounded read-only `Entity.collider` snapshots and
  `physics.closestPoint(point, maximum, filter)`, including actual child poses,
  extents, keys and cooked identity. Closest surface and containment are separate;
  open meshes have no invented interior or unique normal at every boundary.
- **Excluded:** E-06 physical-material assets, E-07 runtime joint APIs, arbitrary
  moving concave meshes, decomposition/remeshing, exact deformable CCD, fluid
  coupling expansion and unrelated skate/gameplay changes.

## Focused verification

`final-focused/RESULTS.json` passes all 16 commands (the earlier `focused/`
pass is preserved), using native X11 desktop OpenGL
assertion windows. `HARDWARE_GL.txt` identifies the actual driver/device.

- Geometry/cook/contact/query suite: **137 checks**; analytic tetra/cube mass,
  COM/pivot motion, import selection/node TRS, topology/transaction/staleness,
  true contacts, internal joins, corners, casts, filters, motor support, radial
  gravity, bounded fast approach, M42 lifecycle and M62 contact coverage.
- Application: probe **23**, streaming **13**, cold-save write **10** and
  independent-process read **15**, far-origin probe **23**. Four skate comparisons
  pass **6** checks each. M63 uses a real authored fracture asset/interface.
- Actual editor shared cook/assign and undo; Play/Stop restores identical authored
  state. No unresolved resource/job failures. Scene reload/transition is exercised.
- Cookbook **236 checks**. TypeScript 5.9.3, live enumeration and drift verification:
  **27 exports / 277 public symbols / 183 native operations / 15 callbacks**,
  28 examples and three intentional negative drift controls.
- Export: **19 registered assets / four scenes**, 51,188,002 bytes, approximately
  **0.083 s**. Moved package probe **23** and stream **13** checks run from `/tmp`.
  Assertion executables are not included in the game package.

Clean Release build: **PASS, zero warnings/errors** in `final-resumed/`.
One production run attempted 132 suites: 130 passed, the identity-transform
impact regression failed, and storage refused an incorrectly named runner scratch
directory before checks. Both affected follow-ups pass; all original failures
remain. Async passes **12 cases / 246 checks**. No full-suite repeat.
`late-focused/` records the corrected 89 impact cases / 3,197 checks, 28 storage
checks and seven affected legacy contact/cast/motor/joint/physics regressions.
`final-focused/` passes all 16 final commands including the new two-sided winding
fixture, native editor, cold processes and moved export. See `ACCEPTANCE.json`.
`final/` preserves the initial clean build timeout at 1,200 seconds / 73%, with
zero compiler warnings/errors and no tests executed. The same clean build tree
continues with a build-only 3,600-second allowance; no source/build tree reset.
Unmodified read-only shipping executable: **PASS**, native desktop X11/GL,
launched from `/tmp`, closed only its own window, zero exit status and every
package file unchanged. `shipping/RESULTS.json` records 120 actual real-time
frames: interval median / p95 / max **10.002 / 10.893 / 11.317 ms** (~100 FPS);
CPU frame **9.965 / 10.955 / 11.761 ms**; fixed step **4.861 / 6.331 / 8.321 ms**
(71 samples), no fixed-step cap hits. This is a short stationary startup proof,
not certification of every gameplay workload. Human acceptance is pending.

## Physics-only ramp evidence

No game-side anti-snag, teleport, transport, alignment or energy restoration is
used in `final-focused/geometry.log`.

Box, sphere and hull cross a flat triangulated diagonal at an initial 4 m/s,
remaining above 3.8 m/s (approximately 4). Largest impulses are 0.1635–0.16372 N·s;
penetration is at most 1.31e-5 m. An alternative diagonal preserves the same slide.
M42 reports one enter, 89 stays and one exit on removal despite feature changes.

R6 curves are tested at 16/32/64 facets. All three shapes progress onto the real
curve, but tumbling boxes/hulls lose substantial energy. At 32 facets, final versus
initial mechanical energy is box 5.77/33.48 J, sphere 31.81/33.97 J and hull
15.40/41.82 J; maximum penetration is about 0.0736/0.0111/0.0636 m respectively.
These are existing finite-step/inelastic response on piecewise planar geometry,
not a claim of smooth or lossless skating. Full raw resolution rows are retained.

## Reconstructed ordinary skate comparison

Four-ray spring chassis baseline retains velocity transport; the comparison disables
only that compensation. Other forces and game rules remain the same. The spring
controller can inject energy. Minimum speed is over the whole trial, not a localized
first-seam measurement; it must not be compared directly with the audit's 6.86→1.21.

| Geometry / compensation | Maximum X (m) | Whole-trial minimum speed (m/s) | End / initial energy (J) |
|---|---:|---:|---:|
| Actual mesh / enabled baseline | 5.720 | 0.553 | 178.04 / 143.64 |
| Actual mesh / disabled | 5.532 | 0.102 | 156.12 / 143.64 |
| Independent box chain / enabled | 1.654 | 0.689 | 20.19 / 143.64 |
| Independent box chain / disabled | 1.655 | 0.132 | 21.35 / 143.64 |

Application contact maxima cover **all lab bodies**, not only the chassis. In
particular the recorded 0.5905 m maximum penetration is not a chassis seam oracle.
Physics-only fixtures above isolate the geometry/contact behaviour independently.

## Costs and measured performance correction

Hardware: Intel i7-4790 3.60 GHz (4 cores / 8 threads), GTX 970, NVIDIA 580.178.04.
Focused measurements were taken without another owned build/profiler job running.

A demonstrated accidental full-face containment scan affected open mesh sphere/
capsule distance work. Removing it **only for non-convex surfaces** changes the
same 200-query / 12,800-triangle workload from about 220/218 ms to about 1.37/1.63 ms.
No solver/cadence/geometry-quality reduction was made. Primitive fast paths remain.

Representative 12,800 triangles / 4,095 BVH nodes / 856,301-byte asset: approximately
24.79 ms cook, 25.05 ms cold decode. Each workload below has 200 successful queries.

| Query | Primitive reference (ms) | Mesh (ms) | Mesh triangle evaluations |
|---|---:|---:|---:|
| Ray | 0.070 | 0.289 | 4,000 |
| Sphere | 0.076 | 1.372 | 6,420 |
| Capsule | 0.082 | 1.635 | 6,420 |
| Fixed box | 1.581 | 31.511 | 8,000 |
| Closest surface, radius 4 m | 0.031 | 3.005 | 50,400 |

The corresponding full scan would visit 2,560,000 triangles. These are CPU batch
costs; SAT box casts remain substantially more expensive than rounded casts.

Twelve-body fixed-step comparisons (median / p95 / max): primitive boxes
0.094 / 0.179 / 0.228 ms; mesh boxes 0.679 / 0.946 / 1.100 ms;
mesh rotated compounds 0.581 / 2.626 / **49.515 ms**. The last maximum is an actual
falling/tumbling impact stall and is reported rather than hidden in the median.
The earlier initially overlapping stress setup is retained separately.

M56 assertion-host profiles use controlled 1/60-s test time and retain 120 frames.
They measure CPU work, **not display FPS**. Lab frame median / p95 / max is
7.969 / 17.017 / 58.967 ms; fixed step 6.500 / 13.651 / 18.052 ms. Reload/startup
stalls remain included. Streaming frame 7.232 / 11.659 / 14.181 ms;
mesh skate baseline 1.045 / 9.219 / 11.251 ms. Shipping measurements above are separate from the assertion-host timings.

## Current limits / human review

See the current reference for static-only mesh placement, unit rigid-instance
scale, cook limits/topology rejection, bounded query convergence, sampled radial
terrain/deformable approximations, summed compound mass with overlapping children,
unsupported hull/mixed liquid loading and additive-world disk-save limits.
No live origin rebasing. Protected liquid algorithms and historical projects are
unchanged; rotated primitive geometry is propagated and unsupported loading fails
explicitly. Human checks must include the actual editor, ramp/open boundaries,
thrown bodies, metadata marker, small deformation/fracture proof, streaming,
root-project cold save and moved standalone.


## Final review and human checklist

`FINAL_SHA256.json` records **123** final participating implementation/tooling/doc/
project/dependency files and matches the completed final checks. It excludes the
accidentally enumerated bytecode cache (removed), generated builds/packages and
this mutable evidence directory. The original two snapshots remain, with narrow
post-gate changes identified in `development/FOLLOWUPS.md`. `CHANGED_FILES.txt`
is the exact tracked/untracked delivery inventory; `DELIVERY_SHA256.json` covers
its contents apart from the checksum manifest itself. No files are staged.

1. In the editor, cook/assign the original quarter-pipe model and compare physical
   wireframe/normals/bounds with rendering; inspect static/hull restrictions.
2. Walk on the green mesh and switch 2/3 to compare mesh versus separate box-chain
   skate scenes; V toggles only seam transport. Test a join and an actual ledge.
3. Pass through the gold doorway's real opening. Throw the red convex body and
   multi-child compound; inspect child keys/rotations and closest-point marker.
4. Inspect navigation/audio opening behaviour, the small cloth/soft proof and C
   releasing an ordinary rigid fracture interface onto the curve.
5. F3 requests/releases the rotated annex; return/revisit it, and H adopts its prop.
6. Open `collision_lab_local.judasproj` for F5/F8 disk saves: save, quit the process
   completely, reopen and load. Streamed additive compositions intentionally cannot
   save to disk; they are a separate proof, not an unadvertised save exception.
7. F9 reload and editor Play/Stop must leave no stale geometry/handles.
8. Repeat interaction in `/tmp/Judas_M64_final-focused` with ordinary audio output;
   the automated shipping run used dummy audio, not human listening validation.

Controls are in the current reference. Starting HEAD/main/origin/main remain
`b5676438ed12d9cb3d05c634eaf5f578d7216ada`. No commit/push/merge/tag, M65 work,
ROADMAP creation or changes to Claude's independently owned work. Protected FTFT/P1
research, historical M33–M63 evidence and accepted liquid algorithms/projects are
unchanged. Only the required rotated-primitive plumbing/unsupported-fluid guards
participate in M64, with legacy identity arithmetic preserved.

Proposed commit after human acceptance:
`Add M64 mesh and convex collision with geometric queries`.

Generated broad-regression projects, copied assets and raw profiles are retained
locally under `.cache/m64-regression-outputs/`, excluded from delivery.
`RELOCATED_TEST_OUTPUTS.json` maps original output paths to their exact retained
bytes/SHA-256; original gate result records and logs are unmodified. Native M64
raw captures already live under `.cache/m64-final-focused/` and the shipping
capture under `.cache/m64-shipping-shipping.json`. API surface/results, screenshots,
genuine failures and all final check summaries remain in the deliverable evidence.
The generated root `imgui.ini` and accidental Python bytecode cache are excluded.
