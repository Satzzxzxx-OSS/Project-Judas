# R1-P1 isolated fluid prototype

The active executable is **P1-C transport only**: Judas Symmetric Geometric Transport with frozen Weymouth-Yue predictors. This is an explicit Judas method, not an unchanged reproduction of van der Eijk-Wellens Eqs. 14-15.

**P1-C PASS for the measured uniform and nonuniform validation family. Integrated P1 remains incomplete.** No moving solids, triple-line integration, pressure/body coupling, P1-D/E, P2 or production integration was performed by this checkpoint.

- `METHOD.md`: exact algorithm, proof assumptions, lineage and limitations.
- `P1C_METHOD.md`: equation/code mapping and build/run instructions.
- `P1C_RESULTS.md`: measured evidence, convergence, witness arithmetic and timings.
- `results/jsgt_*`: current raw evidence and fingerprints.
- `results/continuous_predictor_archive/`: preserved failed predecessor.

Everything is isolated here, outside the repository's disposable root build tree. `.build-jsgt` contains only the local disposable build. No commits or tags were created.
