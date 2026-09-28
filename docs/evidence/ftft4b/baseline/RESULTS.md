# FTFT4B-1 actual-engine baseline results

**Verification complete. Energy and timing defects reproduced. FTFT4B remains OPEN.**
HEAD was and remains `422fd275c5f8b54ad4ff37d851eab24a36e8e089`; the initial tree was clean. No production source or physical constants changed. No passive hooks were necessary. Existing public observations suffice.

## 1. Original energy witness — FAIL

The supplied `production_energy_witness.cpp` was compiled unmodified against the eight current production physics translation units and run **first**. There were no mechanical API adaptations. Compiler flags, duration and exit codes are in `original-energy-run.json`; original output is `original-energy.log`.

| Quantity | Actual original witness |
|---|---:|
| Initial energy, including rotation | 85.5625 J |
| Final energy | 88.382880866527557 J |
| Energy increase | **2.8203808665275574 J** |
| Momentum residual | 0.0000054240226745605469 kg m/s |
| Contacts | 2 |
| Final x velocities, physical bodies 0/1/2 | 0.81150555610656738 / 4.0997481346130371 / 4.0997481346130371 m/s |
| Physical exit code | **1** |

The unchanged failure thresholds are +1e-4 J and 1e-4 kg m/s. The energy criterion fails; momentum meets its criterion. No extracted-scalar result supplied the engine answer.

### 36 actual-engine variants

Six creation orders, axes +X / +Y / -Z, with and without common boost (2,-1,3) m/s. Same masses, radii, touching positions, restitution and friction. Raw pre/post vectors, rotational energy, detection penetrations, final pair surface separations and budgets: `execution/energy.json`.

**36/36 energy failures; 0/36 momentum failures.** Energy increases range 2.820355811932046–2.8524538741516494 J. Maximum momentum residual is 6.854534149169922e-6 kg m/s. All angular velocities/rotational energies remain zero; each step detects two initially touching contacts.

For unboosted +X, the finite-iteration creation-order dependence is:

| Creation orders | Final energy (J) | Final velocities (m/s) | Final pair separations (m) |
|---|---:|---|---|
| 012, 021, 102 | 88.38287609722848 | 0.8115055561, 4.0997481346, 4.0997481346 | +0.0548040029, 0 |
| 120, 201, 210 | 88.41494016508337 | 0.8115055561, 4.8115057945, 4.0908513069 | +0.0625784751, -0.0078717470 |

The supplementary probe promotes velocity components before energy dot products; the original witness performs float dot products before promotion. This explains their small printed energy difference. Nominal authored masses and analytical sphere inertia are used, independently of solver impulse formulas. Every order/axis/boost retains the defect. This does not establish arbitrary-contact behaviour or iteration-limit convergence.

Source mechanism: incoming-speed restitution targets are fixed for all rows before the ten-iteration accumulated impulse solve. The second touching pair is initially separating but can become active after the first row acts. There is no system energy constraint; the initial cache is empty. This is a demonstrated current-engine defect, not merely a concern about scalar extraction.

## 2. Temporal witnesses — requirements FAIL separately

The existing `world_probe.cpp` and `check_world_trace.py` are byte-identical copies of FTFT4A's handoff adapters. Full checker: **13 failed checks / 45** (eight single-impact positions and five chain positions/velocities). Geometry-only checker: **23/23 PASS**. See `execution/temporal.csv` and both checker logs.

| Physical requirement | Current observation | Outcome |
|---|---|---|
| Zero-g, e=0, gap 0.001 m: reach surface and stop | Stored initial gap 0.0009999871253967285 m; step 1 velocity becomes zero while gap stays unchanged | FAIL: stopping short |
| Continue ordinary zero-g frames | At step 120, same gap, zero velocity, zero contacts | FAIL: stranded through the observed 2 seconds |
| e=0.5: rebound only after tau=s/q | First end gap 0.009333312511444092 m versus independent 0.007833340205252171 m; outward velocity 0.5 m/s | FAIL: premature rebound; gap error +0.0014999723061919212 m |
| Impact changes a third body's next collision | At h=0.2 s, actual (x,v): (0,0), (3.7999999523,10), (3.5999999046,0); expected (0.8,0), (2.6,0), (4,10) | FAIL: missed subsequent impact and wrong drift |
| Positive-gap non-impact controls at three coordinate offsets | Positive signed separations retained; approach velocity unchanged | Geometry PASS; not timing acceptance |

Independent single-impact oracle is tau=s/q, then e*q*(elapsed-tau), with no force. Ideal decimal s=0.001 and h=1/60 gives 0.007833333333... m at e=0.5. The table accounts separately for actual float inputs. No production target-velocity expression is used as the oracle.

Supplementary 120-frame traces per restitution value preserve the original physical configuration and the existing 2e-6 m allowance: **240 position failures, 0 velocity failures**. At e=0.5, frame 120 gap is 1.000999093055725 m versus expected 0.9995000585913658 m. This is an observation over 120 frames, not an infinite-duration claim.

No existing actual-engine rotational TOI witness was found in the supplied temporal adapter or current rigid suite. The research archive's angular event reference uses exponential motion, unlike current production. Its 48 reference roots are **not counted as actual-engine timing evidence**. Existing eight rotated-wall player endpoint sweep checks were rerun as part of cache tests; those are not general rotational impact timing tests.

## 3. Angular partition prerequisite — reproduced

244 supplied rotation cases (240 seeded random + four named witnesses), **2,504 actual `IntegrateRigidBodyPosition` calls** on copied states. No impulse, torque, or alternative quaternion integrator. Formula checks all pass against normalized-Euler angle 2 atan(|omega|h/2); 141 cases show endpoint differences beyond the declared float allowance. Max whole/partition formula quaternion errors: 1.5161891589495896e-7 / 2.5979034334117767e-7. Angular velocity is unchanged in every case.

For omega=(0,0,100) rad/s and represented h=1/60: measured whole-step angle **1.3894765829197777 rad**; two half-steps **1.5791646078836599 rad**. Difference **0.18968802496388215 rad**. These halves sum exactly to stored h. The general generated partition sums are also recorded to expose float timestep representation differences.

The allowance is 64*float-epsilon*(1+segment count), declared before execution; it is an engineering allowance, not a proven rounding bound. Raw quaternions, inputs, analytical angles and errors are preserved. This is a prerequisite for future event subdivision; it is not a current unrelated-event-loop bug or proof of torque-free asymmetric-body dynamics.

## 4. Regressions and executed counts

All eight existing executables pass: rigid contacts; contact geometry; contact lifecycle (30 fixtures, 90 steps, 1,050 checks); broadphase; contact cache (13,327 checks, 2,894 bounds, 546 world inspections, eight player sweeps); contact storage (921 checks); physics primitives; lifecycle.

Independent geometry oracle: 823 cases, 887 primitive pairs, 1,245 contacts, 1,646 bounds, 823 world pairs and 1,646 queries PASS at unchanged tolerances. 16,354 predicates / 64 exact fallbacks / zero unresolved. Maximum surface error 5.684341886080802e-14 m; anchor error 1.2710574864626038e-13 m. Separate player geometry 21 cases PASS; six invalid-input and two extreme-range diagnostics PASS under the existing documented range policy. Unsupported FLT_MAX cases remain unsupported.

New/unchanged diagnostic world steps: 1 original + 36 variants + 12 original temporal + 240 extended = **289**, separate from regression steps and angular integrator calls. Command timing records are measurements of these runs, not a performance acceptance campaign: original witness compile 21.835 s; witness 0.00223 s; variants 0.00316 s; angular probe 0.00757 s; existing broadphase fixture 20.917 s including its instrumentation; oracle invocation 3.189 s. The post-witness runner's commands total approximately 43.09 s. These whole-program durations must not be described as production step cost.

Not run: full application/editor/async and FTFT1/3 validation suites, fluid prototypes, research experimental multi-contact solvers, new rotational TOI implementation, performance campaign, human visual validation. Protected source/evidence is checked byte-for-byte instead. FTFT4A performance debt remains OPEN.

An initial build requested nonexistent `judas_physics_primitive_tests`; corrected to existing `judas_physics_tests`. A probe formatting warning was fixed before its measured execution. Original build failure logs are retained. System Python lacked NumPy; existing reference venv NumPy 2.3.5 was used. No physical fixture/assertion changes were made for these wiring issues.

## 5. Change boundary and handback

Changed tracked files: `CMakeLists.txt` (one diagnostic executable linked to ordinary judas_engine), `docs/FTFT.md` (bounded verification status). New code: `tests/TimingDefectProbe.cpp`, `scripts/ftft4b1_validation.py`. New evidence: this baseline tree. No production hooks or production source edits. See `final-verification.json` for protected/source checks and hashes; `SOURCE_MAP.md` for exact integration points. No commit, push or tag. Further timing/impact implementation requires separate review.
