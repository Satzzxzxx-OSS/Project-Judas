# FTFT3 — uniform-gravity scene construction

Implemented for operator review. Starting `main` was clean at
`0d38ddb2babbb8bc51d9500f3fd7721f7f8a1b84`. No commit, push or tag.
FTFT4 and fluid remediation were not started.

## Actual defect and repair

`RuntimeWorld::Build` selected `FaithfulGravity` when `magnitude == 9.81f`
and `IsWorldDown` found both transverse direction components smaller than
`1e-6`. This discarded genuinely authored small rotations. The same rotation
worked at either adjacent representable magnitude, making the physical vector
discontinuous at constructor selection.

The pre-fix witness loads an ordinary scene with a +`5e-7` radian Z rotation.
Its represented inputs are `g = 9.81000041961669921875` and
`q = (1, 0, 0, 2.499999993688106769e-7)`.

| Witness quantity | Independent expectation from represented inputs | Before fix | After fix |
|---|---:|---:|---:|
| acceleration x (m/s²) | 4.905000197424108e-6 | 0 | 4.905000423605088e-6 |
| acceleration y (m/s²) | -9.810000419615473 | -9.810000419616699 | -9.810000419616699 |
| velocity x after 120 steps (m/s) | 9.810000906479606e-6 | 0 | 9.809994480747264e-6 |

The transverse acceleration tolerance was `1.871116221842318e-11`, fixed
before running the witness. The lost component was not rounding noise.
The narrow witness failed 128 of 344 checks; the complete family failed
1,024 of 12,095 checks. Both failures and the exact old production/test source
are retained under `pre-fix/`. The C++ test source is byte-identical before
and after the production fix; fixtures, oracles and tolerances did not change.

The only production edit is in `src/RuntimeWorld.cpp`: compute the requested
acceleration once; use `FaithfulGravity` only if its full represented vector
is exactly `(0, -9.81f, 0)`; otherwise construct `UniformGravity` with that
vector. Remove the now-unused angular-tolerance helper. `FaithfulGravity`,
`RadicalGravity`, gravity region order and the fixed-step integration are
unchanged. No new gravity model, player/contact correction or blending.

## Authored contract and real implementation path

1. `SceneSerialization.cpp::ObjectParser::Quat` parses `rotation w x y z`
   into four finite binary32 components without normalizing them. The
   `gravity uniform g` magnitude is another finite binary32 value.
2. `RuntimeWorld::Build` normalizes the object quaternion and rotates local
   `(0, -g, 0)`. The region position determines containment, not uniform-field
   direction. The tested fixtures have one sphere region and no other field.
3. `GravityContextMap::AddRegion/Sample` uses authored order: first containing
   region wins; unclaimed space returns zero. The fix does not change this.
4. The ordinary `StepPlayedWorld` calls `PrepareDynamicBodiesForStep` in
   `DynamicBody.cpp`, samples the map at each body's local simulation position
   and calls `PhysicsWorld::ApplyLinearAcceleration`: `v += a * dt`.
   `PhysicsWorld::Step` subsequently integrates motion. The fixture's 37 kg
   unmanaged box has no contacts, competing forces, celestial component or
   vehicle controller. Its angular velocity stays zero.
5. The tests use the actual file loader, `RuntimeWorld`, `GameSession` and
   `StepPlayedWorld`. `Window::SetTestInputMode` only controls input acquisition.
   It does not replace the gravity, physics or simulation code. Classic/terrain
   application regressions separately run the complete executable.

The independent generator uses 80-digit Decimal scalar matrix arithmetic.
For the **represented** quaternion `(w,x,y,z)`, `n²=w²+x²+y²+z²`:

```
a_x =  2*g*(w*z - x*y)/n²
a_y =    g*(x² + z² - w² - y²)/n²
a_z = -2*g*(y*z + w*x)/n²
```

No expected vector calls GLM rotation, the runtime factory, selector or gravity
sampler. The manifest also records an intended vector from the original
binary64 axis-angle and ideal specified magnitude; this separates input
representation error from subsequent construction error. In particular,
9.81 is represented as 9.81000041961669921875, not exact decimal 9.81.

For `u = epsilon_float/2`, `gamma(k)=k*u/(1-k*u)`, acceleration tolerances use
`gamma(64)` times these component operation-magnitude scales:

```
s_x = 2*|g|*(|w*z| + |x*y|)/n²
s_y = |g|*(1 + 2*(x²+z²)/n²)
s_z = 2*|g|*(|y*z| + |w*x|)/n²
```

Add only `8*float_denorm_min` for underflow. This conservative engineering
allowance covers float normalization and quaternion/vector arithmetic for
these finite, well-scaled inputs; it is not a universal GLM error proof.
Component conditioning preserves sensitivity to tiny rotations. A blanket
`|g|` tolerance would hide the reproduced defect. Identity-orientation
acceleration is additionally required to equal the represented magnitude
**exactly**, so a one-ULP magnitude quantization cannot pass on this allowance.

Velocity is compared to `a_expected * N * represented_dt` from rest, at
steps 1, 10, 60 and 120. Its allowance adds acceleration error times elapsed
time to a `gamma(N+4)` product/accumulation allowance. No simulation loop is
copied into the oracle. At step 120 the represented elapsed time is
`2.00000010430812835693` seconds. Nearby accelerations can accumulate to the
same float velocity; the test records that rounding rather than interpreting
it as constructor quantization.

## Fixtures and measured results

There are **136 ordinary scene files**: 34 settings × quaternion signs
`q/-q` × fixed origins `(0,0,0)` / `(1e9,-2e9,3e9)`.
Each is loaded, built and stepped; then saved/reloaded, rebuilt and stepped.

- Identity at default, immediately previous/next float magnitude, zero,
  0.25, 3.7 and 17.25.
- Both signs of X/Z rotations at `0.5e-6`, `0.999e-6`, `1.001e-6` and `2e-6`
  radians, straddling the old `1e-6` selection band.
- The small Z witness at both adjacent magnitudes.
- X/Z quarter turns and reversals, Y yaw.
- Arbitrary axis `(1,2,-3)`, angle 0.73 at default/3.7/zero; also a quaternion
  scaled by 3.125 to exercise ordinary normalization.
- Six interior sample positions and an exterior zero-field position per
  construction. Local/global/local coordinate conversion is checked.
- All q/-q, near/far and original/roundtrip local results must agree exactly.

| Representative scene | Expected acceleration (m/s²) | Observed acceleration (m/s²) |
|---|---|---|
| Identity previous | (0, -9.809999465942383, 0) | exactly equal |
| Identity default | (0, -9.810000419616699, 0) | exactly equal |
| Identity next | (0, -9.810001373291016, 0) | exactly equal |
| Arbitrary default | (-5.602382262222013, -8.024400886211435, -0.677061029885128) | (-5.602382183074951, -8.024400711059570, -0.677060961723328) |
| Rotated zero | (0, 0, 0) | exactly equal |

| Free body at step 120 | Expected velocity (m/s) | Observed velocity (m/s) |
|---|---|---|
| Default | (0, -19.62000186249618, 0) | (0, -19.61998558044434, 0) |
| Arbitrary | (-11.20476510881803, -16.04880260943311, -1.354122130393224) | (-11.20476531982422, -16.04882431030273, -1.354122042655945) |
| Zero | (0, 0, 0) | exactly equal |

Each full execution passes **12,095 checks**, **272 runtime constructions**,
**32,640 fixed steps**, with **zero failures**. The final validation invokes
this family twice: once among all production suites, once explicitly with
its own output. The runner requires the full 136/272/32,640 counts.
The earlier fixed narrow witness and full-family development run also passed.

`construction-explicit/samples.tsv` contains all 1,632 vector samples and
`velocities.tsv` all 1,088 time samples, including raw values and allowances.
Maximum absolute component errors are `1.9073486328125e-6 m/s²` and
`2.17008696269862e-5 m/s`; maximum fractions of their declared allowances
are 0.050969 and 0.107408. The former includes float quaternion quarter-turn
roundoff: exact real normalization would give zero residual Y, while float
normalization/rotation produces about `1.907e-6`. This measured numerical
limitation differs from the removed angular snapping. See `observations.json`.

## Reproduce and full regressions

From the repository root, with the stored pre-fix evidence present:

```sh
python3 scripts/ftft3_validation.py --final --jobs 4
```

Dependencies: existing C++17/CMake/SDL2/GLM/OpenGL production toolchain and
Python 3 standard library. The run uses SDL offscreen and Mesa software GL.
`generate_fixtures.py` reproduces the stored scenes and oracle manifest;
`judas_uniform_gravity_scene_tests --case z_positive_half_default --output <dir>`
selects the narrow witness. `--baseline` is only for the original checkpoint
and refuses to overwrite its captured pre-fix provenance.

| Executed validation | Result | Scope |
|---|---|---|
| 36 production suites, including the new construction suite | PASS | Existing gravity, physics, contacts, broadphase, lifecycle, scenes, projects, combustion and existing fluid regressions; no fluid research |
| FTFT2 actual async application | 12/12 cases; 246 checks; PASS | Real GL, IO/decodes, shared demand, all cancellation windows, stale generation, project switch/close, failure/retry, residency, upload budget, outstanding shutdown and loop progress |
| Default-async editor Play/Stop | PASS | Authored state restored and saved bytes unchanged |
| Blocking runtime reference | PASS | Three scripted fixed steps; not async evidence |
| FTFT1 suites | PASS | Fingerprint 132; world state 302; lifecycle 78; scene 76; project 107 checks |
| FTFT1 actual application/editor smoke | 6/6 process checks PASS | Valid save accepted; mismatch/legacy rejected; save/scene/project bytes unchanged |
| Classic and terrain, near and far | 4/4 × 900 steps PASS | 90 physical CSV rows/run; near/far identical; every post-fix CSV byte-identical to pre-fix |
| M31 stress tool | Ran; exit 0 | Supporting diagnostic only; opportunistic timing/unconditional success is not deterministic acceptance |
| Explicit second construction execution | PASS | Same complete fixture/count/assertion family |

The unchanged FTFT2 runner is imported with redirected output. Its protection
bookkeeping excludes only `RuntimeWorld.cpp` for this authorized repair;
all runtime acceptance logic is unchanged. The unchanged FTFT1 smoke module
is likewise redirected. Accepted FTFT1/FTFT2 evidence is never overwritten.

Incremental full build: **0.616 s**; FTFT2 async suite: **0.967 s**;
new construction suite: **0.114 s**; final validation:
**51.441 s**. Classic near/far: 2.773/2.519 s; terrain near/far:
3.571/3.571 s. No build warnings were reported. Commands, environments,
exit codes, executable hashes, source hashes and timing are in `results.json`
and `regressions/results.json`; frame-by-frame async operation traces remain
in `regressions/integration/`.

## Preservation, changed files and limitations

Exact implementation/build/test/documentation changes:

- `src/RuntimeWorld.cpp`: sole production mechanism repair described above.
- `CMakeLists.txt`: build the new real scene-path test against `judas_engine`.
- `tests/UniformGravitySceneTests.cpp`: loader/construction/fixed-step tests.
- `scripts/ftft3_validation.py`: reuse existing validation with protected outputs,
  exact fixture counts and pre/post classic/terrain comparisons.
- `docs/FTFT.md`: accepted FTFT2 checkpoint and this item's disposition.
- `docs/ARCHITECTURE.md`: correct the current uniform-construction contract.
- `docs/evidence/ftft3/`: independent generator/136 fixtures/manifest, pre-fix
  snapshots and failures, post-fix measurements, full regression logs, smoke
  artifacts, commands, observations and source/integrity fingerprints.

Working source hashes match the final passing run. Protected FTFT1/FTFT2
implementation/evidence and all fluid prototype files match their pre-fix
hashes, apart from the authorized construction edit in shared
`RuntimeWorld.cpp`; its persistence paths are unchanged. The prototype Git tree remains
`b71542792b83ac3badf8a449bf4d6e7ea04f217a`; prototypes were not built or run.
No binaries, caches or machine-specific build output belong to the evidence.
`REVIEW_SHA256.json` lists every review file's final fingerprint.

Limits: finite, well-scaled quaternions and nonnegative finite magnitudes in
this family; no new proof for negative/extreme magnitudes, degenerate
quaternions, overlap precedence or box-region orientation. Near/far tests
verify **fixed-origin** local simulation, not live rebasing. Player locomotion
is not asserted to equal free rigid-body motion. Existing contact/async/save
semantics are preserved through regressions; no new physics claim follows.
No human visual validation, hardware-GPU matrix, sanitizer or race-detector
run. Inherited FTFT1 screenshot-review flags remain unfulfilled and are not
counted as human acceptance. Async limits remain synchronous project draining,
indivisible decoder calls and count-budgeted main-thread uploads.

Proposed commit:
`Fix FTFT3 uniform-gravity scene construction without angular snapping`

Final review state: all task changes unstaged and uncommitted on `main`;
HEAD unchanged at `0d38ddb`. No unrelated changes; no FTFT4 work.
