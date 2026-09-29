# FTFT7 — orbital, spacecraft and reference-frame integrity

**CLOSED with the existing finite-step approximations.** Registration and
cross-fidelity defects were repaired; the orbital integrator, contact model,
SAS law and attachment/release mechanism were not replaced.

## Actual defects, preserved witnesses and fixes

1. A Local vehicle also authored as celestial entered the mutual-force set
   twice. First-step delta-v was `0.0095553007` instead of independent
   `0.0047776503 m/s` for one body: attraction doubled even though total momentum
   still closed. Automatic registration now checks existing body identity.
2. Cross-fidelity reaction sampled the Full body's already advanced pose;
   Local vehicles without explicit celestial metadata were omitted from the
   Coarse body's reciprocal source list. One shared pre-step pair-force pass
   now gives Full and Coarse receivers opposite copies of the same force.
3. A celestial demoted at rest was classified Settled and never drifted under
   mutual force. Nonzero mutual force now wakes that celestial into Inertial
   Coarse motion. Ordinary settled-prop behavior is unchanged.
4. Ordinary Coarse bodies gained static point-source gravity on demotion even
   though their Full counterparts did not select it. The extra pull is removed;
   static sources remain selected by Celestial-mode vehicles, which require Full.
   Example before: Full `(1,1.95000005,3)` versus Coarse
   `(0.900749266,1.79119885,3) m/s`; both now match the authored local field.
5. Orbital operator forces retained obsolete handles after reconstruction and
   omitted runtime-created celestial entities. Rebuilding the live celestial
   inventories now also rebinds operator-force handles. The pre-fix expected
   external impulse was `-8.3333336e10 kg m/s`; observed impulse was zero.

`coarse-pre-fix/` preserves **14 failures / 125 checks / 86 steps**;
`development-orbit/` preserves duplicate-registration and thrust failures,
source snapshots, commands and nonzero exit codes. No failure history was
rewritten. The new force pass allocates no scratch for scenes without an Active
Coarse celestial; it does not impose planetary simulation on ordinary games.

## Final executed gate

`validation/results.json`: **PASS, 78.722 seconds**, 247 source fingerprints
unchanged during execution. All commands exit 0; no warning lines in logs.

| Coverage | Result |
|---|---|
| New actual-runtime orbit/thrust | 23 scenarios; 30,785 steps; 357 checks; 22 PASS plus one explicitly measured 60 Hz accuracy observation below |
| New spacecraft/frame integration | 21 serialized scenes; 1,164 steps; 228 checks; PASS |
| New cross-fidelity gravity | 125 checks; 86 steps; PASS |
| Current production suites | 49 suites; all pass |
| Actual async Application | 12 cases / 246 assertions; PASS |
| FTFT1 runtime/editor save validation | PASS |
| FTFT3 scene gravity via production discovery | PASS |
| Classic/terrain near/far | Four 900-step walks; physical payloads identical near/far |
| Existing orbital/control/SAS/frame/attachment/dismount/lifecycle/contact tests | Unchanged assertions; PASS |

All new physical fixtures use authored scenes and ordinary runtime/session
construction and stepping. Force and orbit references use independent double
mechanics/Rodrigues calculations, not the production force helper. Frame
release compares against `v + omega cross r` from authoritative pre-release
state. The player egress clearance remains an explicit positional correction;
it does not invent a new inherited velocity at the corrected position.

### Measured physical residuals

- Worst normalized closed orbital momentum residual: **9.9721e-6** (resolved
  close encounter, 15,360 steps); declared engineering limit `1e-4`.
- Worst orbital barycentre error: **0.00012795 m**; angular-momentum relative
  error **8.0988e-6**.
- Cross-fidelity normalized momentum residual: **9.5250e-8**, below `2e-6`.
- Reconstructed operator-thrust impulse residual: **7.1545e-8** normalized;
  runtime-created operator: **5.2384e-7**.
- Spacecraft pair momentum after subtracting local external impulse:
  **2.95e-7 kg m/s**; pilot-release velocity error **1.02e-7 m/s**.
- Rotated orbital endpoint error **4.45e-5 m**; local translation error
  **0.00039972 m**. A fixed far absolute origin gives identical local state.

### Numerical accuracy remains finite

| Same physical scenario | Timestep | Measured error |
|---|---:|---:|
| Circular endpoint | 1/30 s | 0.282208 m |
| Circular endpoint | 1/60 s | 0.141837 m |
| Circular endpoint | 1/120 s | 0.0710814 m |
| Eccentric energy maximum | 1/60 s | **1.238434%** |
| Eccentric energy maximum | 1/120 s | 0.616444% |
| Eccentric energy maximum | 1/240 s | 0.307900% |

The initial **new** test's blanket 1% assumption for the eccentric 60 Hz run
was not met. Its failing output/source remain intact. That run still executes
and is labelled **OBSERVED, `energy_budget_met=0`**, not a 1% pass. The same
fixture's first-order convergence is asserted, and the unchanged 1% bound is
required at the resolved 120/240 Hz timesteps. No existing production assertion,
physical fixture, tolerance, force or integrator was weakened to hide this.
This closes the qualitative finite-step engine requirement; it does not promise
1% orbital energy accuracy for every 60 Hz scenario.

The near-surface encounter is resolved at **1/1920 s**, reaches **0.2857165 m**
centre separation for two radius-0.14 m spheres without contact, and has
**0.220766%** peak energy error. It does not establish that encounter's accuracy
at the default 60 Hz. Coincident point masses have no Newtonian direction and
return zero. Static authored sources are external reservoirs; no closed-system
reaction is claimed for them. Coarse bodies have no contacts. Quaternion drift
is still first-order and does not model exact asymmetric torque-free gyroscopy.

## Cost and scope

Final focused executables, including setup: orbital **0.064 s**, spacecraft
**0.016 s**, coarse **0.008 s**. These tiny physical tests are not renderer or
large-world performance benchmarks. No contact performance algorithm changed.
No human visual review is claimed. Fixed-origin scope remains unchanged.

Production changes: `RuntimeWorld.cpp` registration/handle inventories,
`CoarseSimulation.cpp/.h` reciprocal preparation and drift-source selection,
`Simulation.cpp` calling that preparation before pose advancement. Tests,
CMake targets, `scripts/ftft7_validation.py`, concise ledger/architecture notes
and this evidence complete the checkpoint. RuntimeWorld persistence logic and
FTFT1–3 historical evidence, contact geometry and fluid prototypes are unchanged;
persistence/async/gravity guarantees were re-executed through current code.
