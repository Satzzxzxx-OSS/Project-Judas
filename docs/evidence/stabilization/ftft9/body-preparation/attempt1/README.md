# Fully submerged neutral fixture — targeted attempt 1

This is a **physical neutral-motion PASS / complete fixture FAIL**. The raw
process exit is 1; the unchanged strict quiescence requirement still fails.
No subsequent targeted neutral run is required: the initial-state defect is
resolved, while preparation rest remains a separate implementation concern.

The original 0.25 m fixture authored 5.25 m³ of particle inventory but, after
ordinary PBF preparation, represented exterior free-surface heights around
0.82–0.90 m. Its body centre was 0.7 m with half-height 0.3 m, so its top
was exposed. An initially exposed neutral-density body should sink until
fully submerged; the old initial condition did not establish a neutral
suspension oracle.

The revised reference uses identical geometry for every density and spacing:
2.4 m authored fill and 3 m side walls. Body geometry, pose, density, ordinary
preparation duration and every existing acceptance threshold are unchanged.
A shared independent observer uses raw particle positions, fixed exterior
columns and particle half-spacing to measure the release surface, then checks
that it clears the body top by one particle spacing. The existing final
immersion oracle uses that same observer plus its separate dense 32³ box
quadrature; neither observer calls production hydrostatic reconstruction.

Executed once against restored Spiky kernel plus the reviewed union escape
geometry and exterior bulk-motion sampling. No second targeted attempt.

| Measurement | Observed | Requirement | Result |
|---|---:|---:|---|
| Independent release surface | 1.702324 m | ≥1.25 m | PASS |
| Release body top | 1.0 m | — | Recorded |
| Initial clearance | 0.702324 m | ≥0.25 m | PASS |
| Neutral 4 s displacement | −0.00980879 m | absolute ≤0.20 m | PASS |
| Final vertical velocity | −0.00535728 m/s | Recorded | Finite |
| Sampled / independently observed immersion | 1 / 1 | Fully submerged | PASS |
| Maximum body speed | 0.01860643 m/s | <100 m/s finite-state gate | PASS |
| Preparation mean particle speed | 0.0727231 m/s | <0.05 m/s | **FAIL** |

592 particles, 9,250 kg liquid inventory; 729 checks, 480 ordinary fixed steps
(240 preparation + 240 measured). Build exit 0, test exit 1. Build 5.5177 s,
test 4.62335 s. Sources were unchanged between capture, build and execution.
Commands, binary/source hashes and source copies are in `metadata.json` and
`source/`; physical scenes and raw per-step traces are in `fixtures/`.

The deeper pool establishes valid neutral initial geometry. It does **not**
prove continuum liquid-volume conservation: PBF boundary packing remains an
approximation, and authored inventory divided by area is not treated as its
represented geometric free-surface oracle. The strict rest failure is retained,
not relabelled as acceptance.
