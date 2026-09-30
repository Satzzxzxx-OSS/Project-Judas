# FTFT9 acceptance and optional strict diagnostics

The final stabilization instruction authorizes stable approximate production
fluid behaviour. Its FTFT9 behavioural gate requires same-volume density ordering
(half-density immersion within ten percentage points, neutral drift <=0.20 m
over four seconds, dense sinking), gravity consistency, container load/retention,
80 kg stability and production-spacing swimming. It does not require every
particle to settle below 0.05 m/s or require a 0.12 m cube to satisfy continuum
equilibrium at 0.20 m particle spacing.

The existing conditions and fixtures remain exactly present. Individual mean
particle-speed <0.05 m/s preparation checks and the existing small_mass_125x
immersion/settling expectations are now explicitly optional diagnostics. The
small fixture still executes; its per-step finite-state, mass and all other
checks remain mandatory. Player bulk-speed and centroid-travel checks remain
mandatory. Every independent physical endpoint and equilibrium tolerance remains
unchanged. No engine path depends on this classification.

Both executables accept --strict-diagnostics. That mode includes diagnostic
failures in their nonzero exit status and reported effective failure count.
Default mode reports diagnostic failures separately from acceptance failures;
it never declares those diagnostic conditions satisfied. The total checks
include each condition once. Historical raw failing logs are not edited.

The before/ directory preserves the exact pre-edit sources and fingerprints.
This scope edit executes no test or experiment; subsequent tested source hashes
and measurements belong to the parent's validation run.
