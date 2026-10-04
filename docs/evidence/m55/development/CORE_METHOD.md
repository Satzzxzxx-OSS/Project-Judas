# M55 core discretization — before integration

State: physical-volume partition V_i (m3) and oriented face discharge Q_f (m3/s).
Owner volume is sum of this partition, counted once by M54. A modest horizontal
chart partitions physical tetrahedral geometry; radial cells are angular cones,
not rotated planar columns. Capacity curves retain geometry-derived storage.
Disconnected subregions within one cell must reject rather than join silently.
Faces are actual shared cavity cross-sections; closed boundaries have no flux.

Local-inertia momentum: Q_new = exp(-friction*dt)*Q_old
 - dt*g*A_face/distance*(q_b_new-q_a_new).
Continuity: C_i(q_new)-V_old + dt*oriented_sum(Q_new)=0.
Frozen wetted face coefficients and monotone piecewise-cubic storage produce a
sparse symmetric pressure/continuity system; bounded nonlinear iterations update
storage slopes. Dry regularization is only a solve coefficient, never extra water
or an artificial physical floor. One shared outgoing-flux limiter bounds every
accepted donor debit before commit; no negative-volume clipping or total rescale.
The accepted volumes are inverted back into actual occupied surfaces. Numerical
pressure residual and positivity-limiter activity are reported separately.

No nonlinear momentum advection in this first local-inertia model. Explicit
linear friction is integrated exponentially. It is a hydrostatic long-wave model,
not accurate spectral deep-water dispersion or full CFD. Real depth enters face
aperture; no depth cap. Fixed-step conditions: moderate resolved flow; implicit
gravity waves, bounded PCG/nonlinear iteration budgets, finite-state checks.
A failed solve restores the entire attempted partition/flow state; bounded
half-step retries may commit only if both halves succeed. Failure reports a
retained state diagnostic, never decorative wave fallback.

Initializer solves sum C_i(q)=owner volume; it is not run every moving step.
External changes use planned bounded local/component allocations, updating owner
and partition once. Amount-only changes operate within one deterministic wet
connected component. Spatial changes identify a cell; no global dry-barrier debit.
Moving solids remove geometric space, not liquid; union subtraction prevents
multiple solids or compound walls subtracting the same region twice. Loading uses
one existing hydrostatic path. Core proof precedes bodies, radial optics and demo.

## Storage follow-up

Local capacity now uses the exact piecewise-cubic simplex-spline distribution of the affine coordinate in each physical tetrahedron. Coefficients are cached between the union of vertex-coordinate knots; repeated coordinates use a positive recurrence. This removes repeated clipping/refinement at runtime while keeping the same physical geometry and radial-coordinate approximation. Inversion uses bounded bisection within the actual cubic interval. Vertex knots remain exact and uniquely sorted. A balanced interval tree accumulates each tetrahedron's bounded cubic pieces in interval-local coordinates, avoiding global derivative cancellation and quadratic knot scans. Adjacent-knot midpoint classification uses long double. Storage comparison tolerance is separate from ledger and solve tolerances. M54 non-opted assets keep their existing tables.
