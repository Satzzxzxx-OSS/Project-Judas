# M62 combined cloth and volumetric-solid candidate

Starting HEAD/main/origin/main: `9e281d6ed57a7f88bc0a8fa48cd9c3cec6984961`.
Implementation is uncommitted. **Human visual/physical acceptance is pending.**

## Implementation and boundaries

[Architecture](../../M62_DEFORMABLES.md) and [public API](../../judasjs/deformables.md)
are the current contracts. RuntimeWorld owns independent CPU XPBD instances of
immutable `.judasdeform` topology. Cloth uses stretch/shear and opposite-chord bend
constraints; solids use tetrahedral Green strains, determinant-volume resistance
and bounded isochoric plastic rest flow. These are modest game-oriented material
laws, not measured material calibration or the full Neo-Hookean constitutive law.
Four substeps/four iterations at 60 Hz are unchanged across benchmark modes.
No new dependency, GL baseline, second physics world or fluid-solver change.

Existing box/sphere/compound geometry and M39 tree/filtering feed sampled surface
contacts, relative-point-velocity friction and finite rigid reactions. The normal
rigid world steps once. Self vertex/face and edge/edge tests use refitted BVHs,
one-ring exclusions and conservative motion-margin pair caching; overflow stops
diagnostically. Animated attachments read the already-resolved authoritative pose;
dynamic attachments use a bilateral velocity constraint and normal body inertia.
Render interpolation, weights, normals/tangents, materials, masks, culling, shadows
and M33 cameras use normal Renderer resources. No simulation is run per camera.

Editor sheet/block/import helpers and selection-box groups are exercised through
the actual editor. Scene/prefab fields use ordinary stable identities/remapping.
M61 conditionally adds a required deformables participant; old scenes retain their
previous participant set. M59 snapshots once before teardown, restores independent
state privately before publication and pins live cross-region targets. Asset bytes
and the conditional authored extension remain under canonical fingerprint schema 5.

## Focused physical results

[core](safety-followup/core.log) has **66 checks, zero failures**. Representative
independent expectations and measured results include:

- Unit tet cube volume 1 m³; cloth/solid mass derived from area/volume and density.
- Rest state stationary; shared assets have independent motion/plasticity.
- Below-yield rest change 0; above-yield retained rest change 0.07493; plastic rest
  determinants preserve original volume. Compression/bend/twist recover from
  0.00688 / 0.01843 / 0.00825 m to 0.000463 / 0.000214 / 0.000385 m after unloading.
- Dynamic cloth/sphere transfer: total momentum -0.80000017 versus -0.8 kg m/s;
  maximum combined kinetic energy 0.400000006 versus initial 0.4 J.
- Free dynamic attachment: maximum combined kinetic energy 0.01121949 J versus
  initial 0.0125 J; support receives motion at 0.00561583 m/s. No reaction is applied
  twice and no extra rigid step is used.
- Relative moving-support friction: cloth 0.496923 versus body 0.498781 m/s.
- Rotated-equivalence error below 1e-6; actual radial/zero gravity and fixed-origin
  large-coordinate checks pass. Vertex crossing a face interior, folding/self-contact,
  bilateral cloth/solid contact, masks and stale body slots are covered.
- Compliant extension: 4×4 at 1/60 gives 0.04378255 m; 8×6 at 1/120 gives
  0.04403086 m. Finite-iteration response is close, not claimed exactly independent.
- Denser render mapping, independent stretch/shear/bend control, sleeping/waking,
  diagnostic inversions, disabled force handling and invalid geometry/mass pass.

See [development failures/follow-ups](development/): the actual initial dynamic-pin
energy failure, contact performance bottleneck, content/parsing mistakes, private
stream restore error and cleanup/derived-data edge cases are preserved. No
historical failures were rewritten. The first cleanup fixture incorrectly assumed
GameSession::End reset state; its corrected reproduction explicitly calls authored
reset. [Safety follow-up](development/safety-followup.md) distinguishes the fixture
mistake from the real cleanup defect and records the narrow fixes.

## Integration and single broad gate

- [Single clean Release / production gate](final/RESULTS.json): **127 suites pass**;
  async **12 cases / 246 checks**, zero failures; existing editor Play/Stop and
  startup/reference checks pass. Source stayed unchanged during that gate.
- The clean build contained one test-only misleading-indentation warning; the
  narrow final build corrected it with braces and has no warnings/errors.
- Legacy fluid quiescence diagnostics reproduce the M60/M61 records; their acceptance
  checks still pass. No fluid implementation or historical evidence was changed.
- [Final safety/integration run](safety-followup/RESULTS.json): **12 commands pass**,
  without repeating the production suite or unchanged performance benchmark.
- Actual application: probe 11 checks, cold write 8, **separate-process cold read 13**,
  region suspend/revisit 11, scene transition/reload/destruction 12. Saved node state
  matches before resume, plastic dent remains, fresh handles replace old ones and
  resumed steps stay finite. A runtime cross-region attachment pins its target;
  releasing it allows unload and exact fresh-owner restoration.
- Actual editor creates sheet/block and imports an indexed OBJ with UV seam identity;
  Play/Stop restores an identical authored scene with no failed resources.
- Executed cookbook **228 checks**, zero failures; API live enumeration/drift and
  TypeScript 5.9.3 pass: **26 exports / 264 symbols / 174 native operations /
  14 callbacks / 27 examples**, plus three negative drift controls.
- [Existing M61 cold readers](safety-followup/legacy-read-results.json): ordinary
  save 20 checks and streamed save 19 checks pass against the final runtime.
- Export: **16 assets, 3 scenes, 50,790,712 bytes (~48.44 MiB), 0.098 s**.
  Moved package `/tmp/Judas_M62_safety-followup` runs from unrelated `/tmp` cwd;
  moved probe 11 and lifetime 12 checks pass. Assertion executables do not ship.
- [Final native desktop proof](native-safety/RESULTS.json): unmodified shipping
  executable runs from a read-only package, exits normally, leaves package bytes
  unchanged and closes only its own PID/window. No other user's/Claude's processes
  or workspace were touched. A capture-directory collision before this rerun is
  preserved in `native-final/preflight-collision.log`; the tool now uses unique
  output-derived paths instead of overwriting previous raw captures.
- A separate 20-second loaded/airflow/moving-cape endurance run stays finite with
  1,056 nodes and 60 reported contacts. A changed imported source is rejected by
  normal export with a rebake diagnostic and no promoted incomplete package.

## Performance

No other build/stress run overlapped native measurement. The normal M56 scopes
provide the measurements; headless software GL and native desktop results are
separate. These are short sanity samples, not broad performance promises.

| CPU workload, identical quality | Median ms | p95 ms | Maximum ms |
|---|---:|---:|---:|
| Active 1,585 nodes, 2,976 triangles / 288 tets / 14,612 constraints | 7.552 | 8.366 | 8.366 |
| Same workload sleeping | 0.0194 | 0.0228 | 0.0228 |
| Empty rigid step, deformable calls skipped | 0.00076 | 0.00078 | 0.00078 |

[Benchmark log](focused-final/performance.log): zero measured hot allocations/bytes;
active candidate count 22,256, no contacts in this mild uniform-force workload.
Its slowest internal constraint phase is about 6 ms. The bypass baseline retains
fixture assets/instances; it measures CPU call overhead, not whole-process memory.
The mixed lab supplies the actual contact/render workload below.

| Native desktop workload | Frame median / p95 / max ms | Fixed median / p95 / max ms |
|---|---|---|
| Unmodified shipping lab, initial calm state | 9.902 / 10.438 / 11.250 | 6.481 / 7.688 / 8.104 |
| Active lab assertion run | 18.666 / 21.552 / 67.014 | 15.886 / 18.272 / 20.444 |

Initial lab: 7 deformables / 1,056 nodes, 6 rigid bodies and one CharacterMotor;
the active host spawns an eighth deformable for 1,221 nodes. Its last captured
step reports 1,095 contact solves (iteration/substep observations, not unique pairs).
The isolated 1,585-node benchmark has no rigid bodies.
The active maximum frame includes repeated render/read-only and capture assertions,
not just the shipping loop. Native RSS peaks are 148.3 MB shipping / 147.8 MB host,
including engine, graphics and profiler resources. No dropped/truncated profiler
records. The active slow phases are internal constraints 5.56 ms, rigid contact
5.00 ms and surface contact 4.33 ms (nested cache about 3.09 ms). Render mapping
remains a separate ordinary resource/update phase.

Final software-GL probe: frame 43.54 / 57.88 / 110.16 ms; fixed
15.78 / 18.00 / 18.59 ms. Startup-inclusive software maximum is 1.60 s,
including resource/shader initialization. Software rendering is not presented as
native gameplay performance. Earlier slow contact profiles are preserved losslessly
in development archives, with original SHA256/index; the final optimization changed
candidate construction, not cadence, iterations or collision narrowphase quality.

## Supported envelope and limitations

- Cloth bending is an opposite-chord surrogate; Green-strain solid/plastic response
  is approximate, with bounded rest flow. Large element inversion halts with a
  diagnostic. No tearing, remeshing, fracture, material calibration or liquid coupling.
- Primitive contacts sample nodes/edges/faces; small fast objects can tunnel through
  coarse triangles. Swept vertex/face plus discrete edge/edge is not full CCD.
- Terrain, CharacterMotor blocking, exact M44 deformable shape casts and M42
  deformable contact events are not added. Current-surface ray picking is explicit.
- Unit-scale owners cannot simultaneously own a rigid root, character motor,
  animated mesh or ragdoll. A separate skeleton/body may supply attachments.
- The animated bar/cape uses an intentionally simple prescribed box proxy; no
  full-body skin collider, garment fitting, IK or active physical animation.
- Explicit target teleports require a deliberate dependent reset. World/bone pins
  prescribe motion and can inject work; only dynamic targets have finite reaction.
- Import supports indexed cloth and versioned pre-baked tet topology plus simple
  blocks; there is no universal external tet importer or arbitrary tetrahedralizer.
- Asset, node/group/contact/quality bounds are explicit and overflow is diagnostic.
  Existing pose/articulation streaming limitations remain; supported deformation
  state uses M61/M59 normal ownership. Continuation is bounded, not promised bit-exact.

## Combined human checklist / controls

Open [Deformable Lab](../../../projects/deformable_lab/deformable_lab.judasproj), Play.
WASD/mouse move/look, Space script jump, Escape pause. Station details and all
controls are in the [project README](../../../projects/deformable_lab/README.md).

1. Aim/hold left mouse for a picked fabric impulse. G carries/drops the brush;
   T throws it. Push/fold the curtain and use R to release its top group.
2. V toggles actual airflow; inspect settling and self-contact, including offscreen
   return. Camera visibility must not freeze physics.
3. C moves/turns the animated fixture; inspect the red cape on its resolved joint
   and ordinary moving body proxy.
4. B loads/unloads specimens; H cycles green bending/compression/twist. Inspect
   elastic recovery versus the orange retained plastic dent after unloading.
5. Inspect yellow cloth/purple cushion and the finite grey support's reaction.
6. Try the oblique station, then 2 for radial gravity; 1 returns to uniform.
7. P spawns an independent cloth. F3 requests/releases the annex; visit the far
   right, leave and revisit, confirming retained independent state.
8. F5 saves; wait for completed, quit the process, relaunch and F8 load. Inspect
   moving fabric and the retained dent, then continue applying forces.
9. F9 reload / editor Stop: no stale cloth, anchors or GPU resources. Inspect
   shadows, two-sided material and the M33 screen while moving the view.
10. Copy/move the exported package and run its executable from another cwd;
    confirm the same interactions and responsiveness.

Visual/physical-feel acceptance is the operator's responsibility and is not claimed.

## Raw detail archive

[Regression detail index](final/REGRESSION_DETAILS_INDEX.json) retains the original
paths, sizes and SHA256 of every generated measurement/temporary fixture copied
into the [lossless archive](final/regression-details.tar.gz). Extraction into an
empty scratch directory restores those original repository-relative paths. Every
archived member was verified against its original bytes before removing duplicate
raw files. Direct production logs/results and genuine development failures remain
plain files. No executable, build tree, exported package or historical evidence is
included in this archive.

## Exact scope / handoff

[Changed files](changed-files.txt) includes every tracked modification and new
source, asset, test, tool, document and evidence file. [Final source/asset/doc/tool
fingerprints](final-source-fingerprints.json) excludes self-referential generated
evidence. [Repository protection review](repository-review.json) records the
unchanged baseline, absent roadmap and no unrelated/historical/project modifications.
Build/cache directories and generated exported packages are not repository content.

Suggested future commit: `Add M62 cloth and volumetric deformables`.
No commit, push or tag was made. No M63/fracture implementation began.
