# Final static-wall velocity invalidated by smoothing

Actual production `FluidWorld::Step`, two particles initially stationary, mass 1 each, y=0.018 m and y=0.06 m, particle radius 0.018 m, uniform gravity (0,-9.81,0), dt=1/60 s, one substep, zero density iterations. The static floor surface is y=0. This isolates the order of ordinary solid velocity response and the existing XSPH-style smoothing.

The independent oracle is the final impenetrable-wall condition: a particle ending on the static floor cannot retain negative outward-normal velocity. It does not duplicate the smoothing formula.

Before repair:

| Smoothing coefficient | Lower final y | Lower final normal velocity | Upper final normal velocity |
|---:|---:|---:|---:|
| 0 | 0.0180000067 | 0 | -0.163500011 |
| 0.15 (production default) | 0.0180000067 | -0.009536481 | -0.153963536 |

The position projection and collision impulse initially satisfy the boundary, then internal velocity smoothing restores an inward component. Energy remains dissipative, which does not make the boundary violation valid.

The full boundary target executes eight cases/24 checks and returns physical exit **1**, with exactly the new smoothing-order assertion failing. The six earlier corner/exact-touch cases and no-smoothing control pass. All coefficients, thresholds, original cases and physical fixtures are preserved.

Reproduce:

```
flock build/ftft9-build.lock cmake --build build --target judas_fluid_boundary_contact_tests -j2
build/judas_fluid_boundary_contact_tests
```

`before-FluidWorld.cpp/.h` preserve the tested production source. `probe.cpp` is the exact new test source; `before-FluidBoundaryContactTests.cpp` preserves the previous six-case target. `before.log`, `before.exit`, `before-build.log`, `before-fingerprints.json` and `executed-sha256.txt` preserve commands/results/provenance without binaries. No production edit was made by the witness author.

## Same witness after the production ordering repair

The parent moved the existing unchanged smoothing operation before the existing solid-contact batch. No coefficient, density iteration count, contact oracle or tolerance changed. The already rebuilt target was rerun: **eight cases, 24 checks, zero failures, exit 0**. With coefficient 0.15, lower final normal velocity is now exactly zero; upper remains -0.163500011 m/s. The no-smoothing control and all six earlier geometry cases still pass. `after.log`, `after.exit`, `after-sha256.txt` and `after-FluidWorld.cpp` preserve the observed result. This establishes the ordering repair; pool settling and broader regressions require their own runs.
