# M55 — dynamic conservative liquid surfaces

Starting checkpoint: `d9ebc8c7a987047b1d4175ed5d5de7da8dea472d`.
**Implementation candidate complete; human visual/interactive acceptance pending.**
Nothing staged, committed, pushed or tagged. No later milestone work.

## Current implementation

An optional surface partition belongs to the existing M54 reservoir: its owned
volume is the sum of its cell allocations, not a second ledger. Physical
cavity tetrahedra define local storage, shared wetted apertures and closed
boundaries. Plane columns and true central-radial angular cells share a bounded
semi-implicit local-inertia pressure/continuity solve. Signed face transfers are
paired, donor/receiver bounded; failed solves retry bounded half steps and roll
back attempted dynamic state. No final volume clamp/rescale, artificial depth cap,
second solver, bulk particles or lowered cadence.

Moving box/compound solids subtract their actual union. Hollow containers exclude
walls and their actual owned wet interior. M54 opening exchanges, transfers,
connections, hydrostatic loading, drag and conserved parcels use the local moving
surface. Moderate splashes debit donor volume and a stated disturbance-energy
budget. A parcel cap suppresses emission without destroying water.

Queries and extracted caps agree with physical occupied geometry. Previous/current
surface levels interpolate for presentation; Renderer owns GL and per-camera
water-path attenuation. Authoritative swimming is ordinary JS + CharacterMotor.
Normal assets/baking, editor, prefabs, resource loading, export and JudasJS
reference/types/inventory are integrated. Non-opted M54 basin serialization remains
`JudasBasin1`; opted assets use `JudasBasin2`/`Surface1`. Canonical fingerprint schema
remains **5**; no new runtime liquid save-state format.

## Documents and exact scope

- [Current guide and controls](../../LIQUID_SURFACES.md)
- [JudasJS liquid reference](../../judasjs/liquid.md)
- [Numerical method](development/CORE_METHOD.md)
- [Measured geometry/storage correction](development/GEOMETRY_FOLLOWUP.md)
- [Preserved failures and corrected follow-ups](development/FAILURES.md)
- [Supplied brief/research boundary](research/README.md)
- [Performance, including heavier cup interaction](performance/PERFORMANCE.md)
- [Exact changed files](CHANGED_FILES.txt)
- [Final implementation/document/project SHA-256 fingerprints](FINAL_SOURCE_FINGERPRINTS.json)
- [Repository preservation check](REPOSITORY_CHECK.json)

## Current ordinary project

`projects/liquid_surface_demo/liquid_surface_demo.judasproj` registers:

- `Scenes/lab.judas`: sloping 16×12 m swimming pool, 160 m³, 384 cells/728 faces;
  tapered 1000 L experiment, graph-connected vessels, receivers, ordinary bucket
  prefab and rigid floating/thrown bodies. 41 bodies, eight basins, one bucket.
- `Scenes/radial.judas`: actual radius-20 planet, 8×6 m terrace lake, 65 m³,
  192 cells/356 faces; real radial equilibrium/storage, local-gravity swimming,
  ordinary bucket and floating body. 11 bodies, two basins, one bucket.

They use the same production liquid/character/physics path around a fixed double
absolute origin `(1e12, -2e12, 3e12)` m. Historical scenes/projects are unchanged.
Final normal exported game: `/tmp/Judas_Dynamic_Surface_M55_Final`, 22 asset records,
16,914,698 bytes. It was moved out of the repository and loaded from `/tmp`.

## Candidate validation and narrow final follow-up

**Later human drain/refill blocker:** the otherwise promising candidate failed
full-pool re-wetting. Its report, original reproduction and intermediate failures
are preserved in [the refill follow-up](refill-followup/README.md). A narrow
momentum/Jacobian/paired-budget correction now passes five complete cycles per
scene, M55 core 61, M54 conservation 67 and 20 application checks per scene.
Final fingerprints include that correction; human re-test is pending. No full
production gate repetition, cadence reduction or project-content change.

[Clean gate](final/RESULTS.json), [production details](final/production/results.json):

- One clean Release build, zero warnings; **103 production suites pass**.
- Async integration: **12 cases / 246 checks / zero failures**.
- M54 core 67 and application 41 checks; M55 core 51 and application 38 checks,
  normal-loop check, all pass at the gate.
- Cookbook 177 checks / 21 examples; TypeScript 5.9.3 + source drift + live
  enumeration: 188 public symbols, 20 exports, 122 native operations, 13 callbacks
  and three negative drift controls pass.
- Normal editor bake and Play/Stop restores identical authored state; save passes.
- Registered reload/transitions, stale-generation rejection, 1000→200→1000 L,
  radial query/refill and moved exported normal-loop transitions pass.
- Changing a disposable cavity after baking correctly rejects stale export.

The gate's source fingerprints are preserved. Narrow later changes cache exact
head areas/cap topology, stage only changed apertures and avoid empty interval-tree
recursion. No numerical tolerances, model, quantity, lake dimensions or cadence
were changed. [Affected final checks](followup/RESULTS.json) refresh:

- **M55 core 52 / zero failures**, including cached-cap agreement with independent
  geometric clipping; **M54 67 / zero failures**.
- Actual normal application/physics cup, swimming, ledger, reload and Stop:
  **20 / zero failures per scene**. Each cup acquires and retains 25.088 L and
  pours through conserved parcels; no fixture writes liquid quantity.
- Cookbook 177, TypeScript/source/live API coverage, editor bake/Play-Stop and both
  scenes through normal and moved-package transitions pass again.
- Native sequential real-loop measurements and heavier native post-cup checks
  are separate; no production-suite repetition.

The fixed-only legacy startup harness omits project JS presentation/UI hooks; its
blank screenshot is retained as invalid *visual* evidence, not a visual pass.
`followup/package-normal-loop.png` and `radial-normal-loop.png` capture Application's
actual lifecycle using moved project resources. The moved transition proof runs
the actual exported executable through that normal loop. Visual captures are not
operator acceptance. Capture timings include readback and are not used in final
performance tables.

The existing legacy player-fluid diagnostic has four recorded non-gating
assertions and returns success under its existing contract. It is preserved,
not silently presented as zero diagnostics. No legacy assertion was edited.
The initial transition timeout and missing-enumeration invocation are preserved
fixture/tooling errors with corrected narrow follow-ups.

## Quantity, geometry and performance results

- Final water ledger errors: flat `-8.53e-14 m³`, radial `1.42e-14 m³`.
  Surface-owner differences: flat `1.42e-13 m³`, radial `1.42e-14 m³`.
- Final maximum cell pressure-equation residuals: flat `1.48e-8 m³`, radial
  `5.20e-9 m³`. The solve target scales with `max(1, ownerVolume)`; these are not
  ledger or geometric errors, and are measured before flux limiting.
- Radial baked coordinate approximation bound: **0.0172028 m**. Do not infer
  millimetre geometric fidelity from the much smaller conservation error.
- A disturbance reaches the far wall and reflects. Half-timestep comparison has
  maximum level difference `0.002229 m`; wet/dry and coarse-grid checks retain
  conservative bounded states. Oblique uniform and true radial checks pass.
- At the same 128 cells/232 faces, 10/100 m depth mean steps are `.1151/.1141 ms`
  with `4.87/7.08` mean pressure iterations. There are no depth-proportional
  dynamic layers. This is not exact spectral deep-water physics.
- Native median frame times: **17.11 ms flat / 18.56 ms radial**; p95
  **25.37 / 27.91**, maximum **33.42 / 31.47**. Catch-up frames **3 / 12**, capped
  frames **0 / 0**. Liquid steps remain 60 Hz.
- The heavier post-cup fixed step is **5.09 ms flat / 13.36 ms radial**, p95
  **5.92 / 15.80**. Radial moving-solid storage remains the main expense.
  These short automated samples do not certify extended interactive performance.

## Honest approximation boundaries

First-order hydrostatic/local-inertia dynamics, no nonlinear momentum advection,
accurate spectral dispersion, breaking sheets or violent container slosh.
Finite-volume surfaces are piecewise constant physical caps, not a smooth
cosmetic wave shader. Radial affine geometry is bounded and face distances are
frozen at the baked rim metric. Disconnected/nonconvex coarse cells reject;
moving subcell topology is not a general flooding model. Unit-scale liquid owners,
box/compound solids, M54 quasi-static vented containers and approximate loading
remain explicit limits. No exact combined body/liquid energy or momentum theorem.

Optics uses a 32×18 nearest-sampled first connected water interval per camera;
no refraction/caustics and no particle or later separated interval attenuation.
Presented level interpolation uses latest authoritative solid geometry. Unsupported
dry-land parcels remain conserved/parked. No liquid dynamic save-state migration,
changing equilibrium fields, zero-gravity blobs or whole-planet streaming.

## Human handoff

WASD/controller + mouse/right stick; Space launch/swim up, C down;
G grasp/drop, T throw, P held tilt, wheel carry height; V wave impulse;
Q/E small 800 L out/back; Z/X main pool drain/refill; F/H flat spill vessels;
1/2 scenes, R reload, Esc pause.

1. Disturb both small vessel and large pool: propagation, reflection, settling.
2. Q/E: 1000→200→1000 L; wave state and totals remain coherent.
3. Swim/exit and cross the waterline; inspect view and responsiveness.
4. Throw/move a body into/out of water: displacement and one buoyancy response.
5. Dip the bucket, carry actual acquired water, tilt and pour into a receiver.
6. Drain/refill the main pool and inspect shoreline/retained motion.
7. Repeat waves, body/cup use and swimming in the true radial lake.
8. Check HUD totals/errors, pause/reload/Stop and moved exported standalone.

Protected FTFT/P1 research/prototypes, all M33–M54 historical evidence, navigation,
legacy PBF and unrelated projects remain unchanged. Root ROADMAP remains absent.
Proposed commit **only after operator acceptance**: `Add M55 dynamic liquid surfaces`.
