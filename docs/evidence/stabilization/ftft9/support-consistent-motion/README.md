# Support-consistent motion interpolation

The existing occupancy admits a finite vertical column envelope. The former
spherical bulk-motion kernel was smaller: a fully wet column could therefore
return zero velocity and acceleration for a uniform moving/free-falling field.
This was a construction defect in sampling support, not a new fluid model.

The minimal two-particle witness has spacing 0.2 m, centre heights ±0.6 m,
query half-height 0.3 m and radius 0.1 m. Its envelope covers [-0.3,0.3], but both
centres lie outside the old 0.5 m motion reach. The executed before-fix archive
fails both constant-motion assertions. No expectation calls the sampler to
calculate its answer: all donors carry the same independently supplied vector.

The current repair retains the original spherical motion interpolation whenever
it has positive donor weight. Only a wet query with **zero spherical donors**
uses actual velocities/accelerations from its admitted, unoccluded columns.
Extension weights are particle volume × admitted vertical interval fraction ×
positive lateral kernel. A positive weighted mean preserves a uniform vector;
it does not substitute gravity or a filtered history. For nonuniform data,
continuity across the transition between the two supports is not guaranteed.

The broader candidate that replaced all spherical interpolation with column
weights was rejected. Its sampler checks passed, but the unchanged `half_025`
body fixture reached independently observed immersion 0.6086176554, exceeding
the existing 0.60 limit. Its source is retained in [rejected-cylinder](rejected-cylinder/),
and the [raw full-body failure](../completion-candidate/focused/judas_production_fluid_tests.log)
is preserved. No tolerance or reconstruction parameter was changed to accept it.

The existing reconstruction, query reach, occupancy and assertions are unchanged.
The sampler's existing row-entry continuity tests remain mandatory. Before source
and raw failures are preserved here. This does not claim exact exterior momentum
conservation or repair PBF continuum fidelity.

The narrower repair passed [93 sampler checks and the unchanged 731-check
`half_025` fixture](../completion-candidate/narrow-extension/results.json).
That run also recorded the remaining rotated-player free-fall failure; its
overall result is deliberately false. The later caller-geometry repair has
[separate focused evidence](../non-displacing-player/README.md). Neither
component record establishes complete FTFT9 acceptance.
