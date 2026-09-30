# Bounded exact-density-gradient candidate

This is one intentionally bounded numerical-model consistency experiment. The
previous solver estimates density with Poly6 but uses a conventional Spiky
surrogate gradient. That convention is not labelled a coding error here.

The candidate instead differentiates the implemented Poly6 density kernel in
both the constraint denominator and its positional correction. No density
iterations, regularization, correction cap, smoothing, cadence, collision law
or acceptance tolerance changes. The independent focused pair oracle obtains
the gradient by a centred numerical derivative of the scalar density kernel
(at three distances), then checks the resulting pair projection, closed
momentum and rotation covariance.

Only one unchanged 0.20 m neutral-player smoke is planned after the shared
source freeze. The existing strict mean-particle-speed <0.05 m/s preparation
gate remains a failure gate. A failed candidate is restored immediately and
its source/logs retained here. This directory is development evidence, never
a passing FTFT9 acceptance claim.

## Executed outcome: rejected and restored

Combined frozen-source build succeeded in 4.81 seconds. Existing smoothing
checks and all three independent derivative/projection/rotation/momentum
cases passed. Maximum pair-position error was 2.13e-8 m, rotation error
4.56e-9 m and closed pair momentum residual exactly zero.

The single unchanged .20 m neutral-player smoke ran 480 ordinary steps and
492 checks, exit 1, four failures, in 17.76 seconds. Pool mean particle speed
was 1.07689071 m/s (previous surrogate diagnostic .08335652 m/s), bulk speed
.11119931 m/s and last-second centroid movement .07624843 m. Neutral drift
failed too. The exact Jacobian is mathematically consistent but this candidate
substantially destabilizes the fixed finite-iteration PBF approximation.

The candidate was rejected immediately. Only the gradient definition, its
two density-correction call sites and the candidate focused test were
restored. Concurrent reviewed box-union geometry repairs were preserved.
Current source requires relinking before further acceptance testing. No
acceptance assertion or physical fixture was changed.

The full-tree hash guard observed a concurrent edit solely to
`tests/FluidBoundaryContactTests.cpp`, which is not an input to either tested
executable. All engine and tested player/smoothing sources were unchanged
during the run. Full hashes and the single changed-file list are retained.
