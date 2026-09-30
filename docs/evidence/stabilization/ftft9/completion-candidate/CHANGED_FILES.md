# FTFT9 change inventory

All listed implementation remains uncommitted on d2246cd19ad70065d121ffc5d4a4445ebca5d82b.
The final [source fingerprints](source-fingerprints.json), [snapshot](SOURCE_SNAPSHOT),
[tracked diff](working-tree.diff) and [git status](git-status.txt) are preserved.
Binaries are hashed in metadata but remain in ignored build output.

## Modified tracked files

- `CMakeLists.txt`
- `README.md`
- `assets/scenes/classic.judas`
- `assets/scenes/classic_fluid_rotated.judas`
- `assets/scenes/classic_fluid_zero.judas`
- `docs/ARCHITECTURE.md`
- `docs/FTFT.md`
- `src/FluidWorld.cpp`
- `src/FluidWorld.h`
- `src/GameSession.cpp`
- `src/PhysicsWorld.cpp`
- `src/PhysicsWorld.h`
- `src/PlayerController.cpp`
- `src/PlayerController.h`
- `src/RuntimeWorld.cpp`
- `src/RuntimeWorld.h`
- `src/Scene.cpp`
- `src/Scene.h`
- `src/SceneFingerprint.cpp`
- `src/SceneFingerprint.h`
- `src/SceneSerialization.cpp`
- `src/Simulation.cpp`
- `src/Simulation.h`
- `src/WorldPresentation.cpp`
- `src/editor/ComponentEditors.cpp`
- `src/editor/EditorPanels.cpp`
- `tests/ProjectTests.cpp`
- `tests/SceneFingerprintTests.cpp`
- `tests/WorldStateTests.cpp`

## New source, tests and validation script

- `scripts/ftft9_validation.py`
- `src/FluidHydrostatics.cpp`
- `src/FluidHydrostatics.h`
- `src/ProductionFluidCoupling.cpp`
- `src/ProductionFluidCoupling.h`
- `tests/FluidBoundaryContactTests.cpp`
- `tests/FluidCavityImpactTests.cpp`
- `tests/FluidContainerIntegrationTests.cpp`
- `tests/FluidHydrostaticsTests.cpp`
- `tests/FluidMetadataTests.cpp`
- `tests/FluidSmoothingBudgetTests.cpp`
- `tests/ParticleContactBatchTests.cpp`
- `tests/PlayerDryTrace.cpp`
- `tests/PlayerFluidIntegrationTests.cpp`
- `tests/PlayerSwimmingTests.cpp`
- `tests/ProductionFluidPerformance.cpp`
- `tests/ProductionFluidTests.cpp`

## Evidence and status

- `docs/evidence/stabilization/ftft9/`: prior failures, source snapshots, commands,
  fixture CSVs, raw logs, before/after hashes and current acceptance evidence.
- `docs/STABILIZATION_STATUS.md`: review/checkpoint and final-release scope.
- Protected `prototypes/` and FTFT1–3 evidence are unchanged.
- Main contact solver, impact timing, gravity, orbital and thermal algorithms
  were not changed by this boundary/sampling continuation.
