# FTFT4A — represented contact geometry

**Scope:** geometry repair on baseline `3404388f1af48419505c802fc11dbf8a0b577f38`.
No commit, tag, event scheduler, gravity, persistence, resource or fluid-model/source change.
Full FTFT4 remains open. FTFT4B's temporal witnesses still fail.

## Provenance and reproduction

The repository initially matched the reviewed commit and was clean. Every source
hash supplied by the handoff matched the repository. `pre-fix/baseline.json`
records the attachment checksums, initial status and all tracked source hashes.
`pre-fix/source/` preserves the replaced source. The original handoff and
extracted reference source are under `handoff/`; their historical claims of
unexecuted engine adapters are not current engine evidence.

From the repository root, with the ordinary C++17/CMake/GLM/SDL2/OpenGL build
dependencies and Python 3:

```sh
python3 scripts/ftft4_validation.py --final --jobs 4
python3 docs/evidence/ftft4/geometry_oracle.py \
  --binary build/judas_contact_geometry_tests \
  --output docs/evidence/ftft4/geometry-reproduction
build/judas_contact_geometry_tests --invalid
bash docs/evidence/ftft4/handoff/code/run_source_probes.sh \
  "$PWD" "$PWD/build/ftft4-reproduction-probes"
python3 docs/evidence/ftft4/handoff/code/check_world_trace.py \
  build/ftft4-reproduction-probes/world_probe.csv --mode geometry
```

The production runner deliberately refuses to overwrite an existing completed
`final/results.json`. Preserve that directory under a different evidence name
before an operator-requested rerun. The independent oracle accepts a new output
directory. The source-probe `--mode full` command is **expected to fail** until
FTFT4B; it is not part of FTFT4A passing acceptance.

The runner reuses FTFT2's original tests and application loops, redirects only
FTFT3 gravity evidence output, and reuses FTFT1's original runtime/editor smoke
logic with its output directory redirected. This avoids overwriting protected
evidence. Scripted classic/terrain walks remain **blocking physical reference**
checks; actual async acceptance is the unchanged 12-case real-GL harness.
M31 stress remains supporting diagnostic evidence. No human visual validation
is claimed for offscreen execution.

The supplied four Python references and scalar C++ gamma probe were separately
executed unchanged in `independent-reference/`. They required the supplied
`numpy==2.3.5` and `mpmath==1.3.0`; a build-only virtual environment supplied
those dependencies. The initial missing-dependency failure is retained.
Their PASS is supporting arithmetic evidence, not engine acceptance.

## Reproduced defects and repair

The supplied adapters were compiled against the complete real engine **before
editing**. Raw results and the deliberately failing geometry check are in
`pre-fix/`. They reproduced positive-gap collapse at local translations 0,
137 and 1024, four-step zero-gravity premature stopping, premature bounce
position, and an impact-chain timing error. The actual raw AABB tree passed
all 400 supplied query expectations before and after the repair.

The selected mechanism is in [GEOMETRY.md](GEOMETRY.md): promote stored inputs
before subtraction and child composition; use proper quaternion rotation;
filter sign predicates with actual-operation outward intervals and exact
expansion fallback; clip in local binary64 frames; retain signed separation
and parent-local anchors; outward-round shape bounds. The old gamma8
zero-collapse rule is removed. Broadphase structure and impact timing remain.

| Non-closing gap witness | Represented gap (m) | Pre-fix velocity | Repaired velocity (m/s) |
|---|---:|---:|---:|
| T=0 | 2.384185791015625e-7 | 0 | -3.5762784591497621e-6 |
| T=137 | 3.0517578125e-5 | 0 | -0.00045776364277116954 |
| T=1024 | 0.000244140625 | 0 | -0.0036621091421693563 |

The repaired velocities equal their initial values; no false support impulse
is applied. Float pose integration can still lose a displacement smaller than
its representable spacing. `post-fix/` records the final rebuilt probes,
checks and source fingerprints. [Trajectory analysis](trajectory/README.md)
preserves actual baseline/current engine explanations for changed scene motion,
and explicitly records the remaining terrain attribution limits.

Review also reproduced a defect in the new convenience-wrapper integration:
rotated spheres received identity-frame offsets as parent-local anchors.
A central sphere/box witness gave impulse 0.571428597 instead of 2 and
artificial spin 1.42857146 rad/s. Explicit anchor-frame flags and conversion
in `Prepare` repair it. Six direct-contact controls now pass, including
quarter-turn and oblique sphere orientations. The failing source and output
remain under `development/convenience-pre-fix*`.

## Independent geometry evidence

`geometry-final/results.json` and `probe.jsonl` report actual stored inputs and
actual production outputs. The unchanged supplied rational geometry functions
supply independent pair truth; new checks independently evaluate surface
witnesses, projected gaps, anchors, normals and bounds. Oracles do not derive
expected results from the engine narrowphase.

- 823 fixtures, 887 primitive pairs, 1,245 returned contacts.
- 87 exact-touching pairs; 310 contacts from independently separated pairs
  preserve positive separation.
- 1,646 exact rational outward-AABB checks; 823 world-pair comparisons and
  1,646 conservative-query checks. These are check counts, not collision counts.
- 16,354 sign predicates: 16,290 interval resolutions and 64 exact fallbacks;
  **zero unresolved supported fixtures**.
- Maximum surface error 5.684e-14 m; midpoint-anchor error 1.271e-13 m;
  witness-projected separation error 5.684e-14 m; unit-normal error 2.220e-16;
  sphere signed-gap error 9.770e-15 m.
- 21 independent analytical player sweeps pass, including compound openings
  and destroyed/reused proxies. Existing 300 all-body comparison sweeps also
  remain relevant supporting broadphase evidence.
- The unchanged rigid-contact suite, additionally wrapped for counters, passes
  with 2,230,499 predicates, 2,082 exact fallbacks and zero unresolved/invalid
  cases (`development/rigid-uncertainty*`).
- Compiled oracle adapter: 0.05226 s; complete Python+engine oracle: 3.35460 s.

[geometry/README.md](geometry/README.md) declares inputs, fixed error allowances
and oracle limitations. No thresholds or original assertions were widened.
Earlier passing expansions of the fixture family are preserved separately.

## Production regressions and performance

Final aggregate results are in `final/results.json`; initial unchanged results
are in `baseline/results.json`. The final source hashes are recorded before
and after execution. Final measurements and disposition are summarized in
[RESULTS.md](RESULTS.md).

The original 1,500-crate fixture is unchanged except for printing actual
predicate counters. The baseline seven-run median was **4.196 ms** (range
4.029–4.745). The first repaired implementation measured 28.460 ms, then
21.792 ms after a semantics-equivalent adjacent-binary64 operation and cached
position-pass anchor rotations. Final seven-run median is **20.979 ms** (20.295–27.958), approximately **5×** baseline. Detailed component timers are in RESULTS.

The rounding operation was checked against `std::nextafter` for 2,000,026
cases, including IEEE boundary values and seeded random bit patterns; zero
mismatches. `rounding_check.cpp` preserves this independent check. A bounded
instrumented copy of the same crate setup identifies interval multiplication,
SAT, AABB construction and interval addition as major costs. Its profiler
harness is diagnostic only; it does not replace the unchanged acceptance
fixture or provide the reported final timings. Sources, raw profile and
commands are under `development/profile*`. Exact fallback is not the crate
cost: the final-step crate counters report zero fallbacks.

## Explicit remaining limits

- **OPEN FTFT4B:** at restitution zero the body remains at its represented
  0.999987 mm pre-contact gap for all four zero-gravity steps. At restitution
  0.5 the first end gap is 9.333313 mm versus the analytical 7.833340 mm.
  Rebound speed alone does not prove impact timing. The chain witness also
  remains wrong. No temporal equation was changed to hide these failures.
- **Performance:** the robust interval geometry costs substantially more
  than the previous float path. Correctness passes do not establish that the
  cost is acceptable for the engine's budget.
- **Terrain:** its specialized float sampled approximation is retained and
  regression-tested, not certified by the primitive exact oracle.
- **Numerical range:** six zero/NaN/Inf quaternion cases explicitly report
  invalid/uncertain. A finite quaternion with all components `FLT_MAX`
  represents overlapping concentric boxes but exhausts homogeneous arithmetic
  range and remains unresolved. The analogous `FLT_MIN` case resolves.
  This is an acknowledged range failure, not accepted overlap evidence.
- **Construction versus predicate:** exact/filtered signs do not make every
  clipped coordinate exact. Local binary64 constructions have independently
  measured residuals; a detected sign contradiction reports uncertainty.
  Finite tests are not a universal floating-point proof.
- **Representation:** inputs are the stored floats. Lost pre-storage gaps
  cannot be recovered. Fixed-origin scope is unchanged; no live rebasing.
- **Raw API orientation:** geometry consistently uses proper quaternion
  rotation. Existing raw rigid inertia still assumes normalized orientation;
  scaled-quaternion query tests do not certify raw non-unit dynamics.
- Player sweeps retain their existing sampled binary32 capsule approximation;
  coverage does not establish exact CCD. Symmetric coincident geometry has no
  unique rotation-covariant normal.

Protected FTFT1–3 implementation/evidence and all fluid prototypes are unchanged.
The prototypes were **not rerun**: this pass changes no prototype source or
build path. No event timing, FTFT5, fluid research, commit, push or tag follows
from this report.
