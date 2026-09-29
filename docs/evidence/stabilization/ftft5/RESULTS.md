# FTFT5 — fixed-origin guarantees verified

**CLOSED; no production defect reproduced and no production code changed.**
Live origin rebasing is a future capability, not an implemented M23 feature
or an outstanding repair to its accepted fixed-origin contract.

`WorldCoordinates` stores a fixed `glm::dvec3` origin. Absolute positions
are origin plus local `glm::vec3`; conversion back subtracts in double
before narrowing. Simulation, contacts, frames, camera and renderer share
local float positions. Compact scenes can be placed at very large absolute
coordinates. Travelling arbitrarily far through a continuously rebasing
local region is not supported. Float local-range and double absolute-range
limits still apply; precision is finite.

## Executed evidence

- Existing world-coordinate suite: **52 checks, zero failures**, including
  motion, contact, player/camera, orbit, interaction and reference-frame
  equivalents at the tested billion-metre translation.
- Actual gravity scene construction: **136 scenes / 272 constructions /
  32,640 fixed steps / 12,095 checks**, zero failures, including near/far
  placement and authored round trips.
- Actual runtime classic/terrain near/far: **four 900-step walks**, complete;
  each scene's near/far physical payloads are byte-identical.
- Source remained unchanged during execution. Protected evidence/prototypes
  and FTFT1–4 production code are unchanged. No live-rebase test is claimed.
- Build and all commands exit0; total runner wall time **15.323 s**.
  Automated offscreen GL only; no human visual-validation claim.

Reproduce from the repository root after configuring the ordinary Release
build:

```sh
python3 scripts/ftft5_validation.py --output docs/evidence/stabilization/ftft5/new-validation
```

`validation/` contains commands, raw logs, binary/source fingerprints,
scene outputs and harness CSVs. The existing harness remains a blocking
physical reference; it is not relabelled async evidence. No assertion,
fixture, origin representation or numerical tolerance changed.
