# Free dynamic attachment development failure

`core-dynamic-followup.log` preserved the actual failed finite-support energy check:
maximum kinetic energy 131492.823 J and support speed 24.4724 m/s after a 0.1 kg m/s
cloth impulse. The positional pin denominator included body inertia, but repeated
iterations applied reactions against an unchanged prescribed anchor pose.

The correction uses a velocity-level bilateral constraint for dynamic targets,
re-reading their actual point velocity after each reaction. A 0.1 positional drift
bias uses the previous sampled anchor; prescribed world/bone targets retain exact
positional pins. PhysicsWorld still advances once, and corrected node positions
reconstruct velocity once. The same unforced fixture is rerun without weakening
its original 0.04 J upper bound. At this development stage the rerun was pending;
the corrected result is recorded below.

The corrected unforced fixture passes: maximum kinetic energy **0.0112194903 J**,
carrier speed **0.00561582856 m/s**. Initial cloth impulse energy is 0.0125 J.
`core-bvh-followup.log` records the unchanged checks and the added physical
compression/bend/twist recovery cases (50 checks, zero failures).
