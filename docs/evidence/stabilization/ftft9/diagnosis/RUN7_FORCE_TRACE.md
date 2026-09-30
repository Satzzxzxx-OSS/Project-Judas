# Read-only run-7 force and failure trace

No simulation or suite was rerun for this note. All numbers below come from
`body-development/run7/*.csv` and the source in that run / current source.

## Dense-body failure first occurs in particle boundary response

For `dense_020`, step 19 finishes at body y=0.4540635943, vy=-1.3022617102,
with liquid KE=344.9126282 J. The box half-height is 0.3 and the liquid
collision radius is 0.072. Thus its bottom-floor gap is 0.1540635943 m,
only 0.0100635943 m more than the two radii (0.144 m). The next liquid
execution predicts further downward body motion. At step 20, liquid KE
becomes 299336064 J BEFORE sampling hydrostatics. The sampled buoyancy
force is (-1411526.875,1802.4467773,366553.46875) N; the rigid speed
subsequently reaches 35.70735931 m/s. The body did not supply comparable
kinetic energy before this operation.

For `dense_025`, the same onset is earlier: step 17 bottom-floor gap is
0.1655497670 m while two radii total 0.18 m. At step 18 liquid KE jumps
from 163.2617645 to 4.1105851617e33 J.

Source-derived mechanism: a coarse particle trapped below the descending
box can retain floor normal +Y and box-bottom normal -Y. The kinematic
constraints then require v_y>=0 and v_y<=negative wall velocity. There
is no admissible velocity. `ApplySolidCollisions` retains historical
planes throughout the substep, and the auxiliary normal solve/optional
CG acceleration assumes a feasible unilateral system. Increasing
iterations cannot satisfy these opposing moving walls. Exact runtime
row capture is not available in run 7, so the individual offending row
is a source-derived diagnosis, not a newly executed witness. The first
recorded bad operation is the liquid execution at steps 20/18.

This must be corrected geometrically or by a stated finite-resolution
contact policy before using its acceleration as a body force. Merely
averaging or damping the resulting large acceleration conceals the first
energy-producing operation.

## Body-driven local acceleration feeds hydrostatics

At `half_020` step 10, mass-weighted whole-liquid acceleration is
(0.012938,0.46878,0.00088156) m/s2. The continuous current kernel at body
centre gives (0.036549,2.3342,-0.0018181) m/s2; local liquid velocity y is
0.32858 versus whole-liquid 0.03097 m/s. Query currently accumulates
these bulk quantities before rejecting columns geometrically occluded
by the queried solid. Only occupancy uses the solid occlusion mask.
Thus the exterior force includes body-induced contact acceleration in
its own displaced volume's hydrostatic response. This positive feedback
can maintain floating oscillation even without the dense contact failure.

A geometrically consistent correction to inspect is extrapolating bulk
velocity/acceleration from the same exterior unoccluded columns used for
occupancy. This is not a temporal filter or a classification based on
mass, velocity, object identity or expected outcome. It is a proposed
mechanism correction, not yet validated by this note.

## Neutral 0.25 is initially exposed after pool preparation

The neutral 0.25 release pose is y=0.7, top=1.0 m. Re-evaluating the
recorded geometric column reconstruction for one corner's quadrature
y=0.475,0.625,0.775,0.925 gives approximately 1,1,0.9584,0.1059 occupancy.
Many surrounding exterior columns have tops only 0.82--0.90 m. Initial
reported total immersion is around 0.77, giving about 1600 N support
against 2118.96 N weight. A final mean immersion near 0.985 does not
prove that the starting neutral fixture was fully submerged. Its
initial sink is physically expected from the represented free surface.
The unchanged pool preparation check already fails (mean speed 0.0830
m/s versus 0.05 limit). Resolve particle preparation first, then ensure
an independent fully-submerged neutral initial condition; do not weaken
the neutral-drift assertion or force occupancy to one.
