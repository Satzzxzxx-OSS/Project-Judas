# Production FluidWorld boundary witnesses

The six deterministic direct-`FluidWorld::Step` cases isolate ordinary collision processing: one unit-mass particle, no forces, density iterations=0, velocity smoothing=0, one substep. No alternative integration or response code is supplied. Oracle: an inelastic particle contacting two perpendicular static walls has zero normal velocity at both; a particle initially touching a wall cannot cross inward through it; departure and tangent motion remain free.

Pre-fix compile:

```
c++ -ffp-contract=off -O3 -DNDEBUG -std=gnu++17 -Wall -Wextra -Isrc -Ithird_party/glad/include -isystem /usr/include/SDL2 tests/FluidBoundaryContactTests.cpp build/libjudas_engine.a build/libjudas_glad.a -lSDL2 -ldl -pthread -o build/judas_fluid_boundary_contact_tests
build/judas_fluid_boundary_contact_tests
```

The pre-fix diagnostic deliberately linked the existing archive before rebuilding the parent repair. Its source snapshot is `../development-smoothing/before-FluidWorld.cpp`, SHA256 `7fa327612f9767479670415aa1e861e78873d4150b504e6dd18f25dc9e198978` (matches the independently recorded player smoke2 source fingerprint). The archive/binary/probe fingerprints, raw stdout/stderr and physical exit 1 are preserved here.

Observed: both corner wall orders project the particle to the correct corner but retain the velocity toward the earlier wall. Exact-touch inward crossing passes entirely through the thin wall. The other three cases pass. This gives four assertion failures across six cases/18 checks. These physical failures are never labelled passing acceptance.
