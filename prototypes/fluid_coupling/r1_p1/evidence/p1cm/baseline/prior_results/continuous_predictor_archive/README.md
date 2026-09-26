# R1-P1 isolated fluid prototype

The active executable is **P1-C liquid/gas transport only**. It implements Judas Symmetric Geometric Transport (JSGT), an intentional nonlinear method change inspired by COSMIC predictors. It is not claimed as a faithful reproduction of van der Eijk–Wellens Eq. 15.

See `METHOD.md` for the exact method, `P1C_METHOD.md` for code mapping and build instructions, and `P1C_RESULTS.md` for measured evidence. Uniform transport/refinement passes; nonuniform divergence-free strain fails at the first y predictor. **P1-C and P1 remain incomplete.** No moving-solid/coupled-pressure work or P2 is authorized by this result.

All source and evidence are kept here, outside the project's disposable build tree. `.build-jsgt` is only the local disposable executable build. Nothing is integrated into Judas runtime/editor/production tests. No PBF, analytic buoyancy, mass cutoff, AMR, surface tension, or limiter is used.
