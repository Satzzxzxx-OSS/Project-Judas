# FTFT8 accounting and numerical scope

The existing finite coating chemistry remains: pre-step temperature activates
reaction, sampled oxidizer limits it, and the actual double-precision fuel
decrement owns chemical heat. The rigid carrier retains its authored mass and
inertia. The atmosphere is an external, prescribed reservoir; neither local
oxygen nor product gas is evolved. Smoke/flame geometry is presentation only.

## Passive-exchange stability repair

The old final `max(1 K, T)` could create unreported heat, including with no
exchange at all. The repair removes that post-temperature overwrite.

For pre-step nonnegative temperatures, a radiative edge has conductance
`Gij = emissivity * sigma * area * (Ti² + Tj²) * (Ti + Tj)`.
This factors `Ti^4 - Tj^4` without dividing by temperature difference.
The environmental edge adds convection conductance. For each body:

`si = min(1, Ci / (2 dt sum_j Gij))`, or 1 if the sum is zero.

Each body-body edge uses **one** `min(si,sj)` for both ends; the prescribed
reservoir edge uses `si`. The original flux expression is retained when the
scale is 1. Thus `dt sum(G_actual)/Ci <= 1/2`: the passive temperature update
is a convex combination of the pre-step temperatures, with self-weight at
least 1/2. This prevents negative temperature and passive overshoot; equal
and opposite body-body exchange remains equal and opposite. It is an explicit
conductance-limited approximation for stiff coefficients, not exact heat
transfer at arbitrary timestep. External heater and chemical power are added
separately and may legitimately raise temperature above neighboring values.

Diagnostics expose `heatExchangeScale` and incident `limitedHeatExchangeJ`.
The latter is withheld exchange, **not** an energy source or part of the
storage budget; a pair is recorded at both incident bodies. Pair/environment
power diagnostics report the actual applied transfers. No temperature floor
or repair supplies unexplained heat. Unrepresentable source-driven temperature
raises an explicit numeric-range error; arbitrary finite input combinations
are not a guarantee of representable output.

## Ordinary vacuum scene path

Absent atmosphere means zero-density, zero-oxidizer, zero-background-temperature
vacuum. The same thermal step handles radiation and the externally powered
heater. A small non-planetary scene does not need an atmosphere component to
use thermal materials. No separate vacuum combustion law is introduced.

## Independent checks

Tests compare `sum(C deltaT)` with consumed-fuel heat minus product-gas heat
plus heater and environmental exchange; pair exchange must cancel. Moderate
settings also compare actual power with independent double-precision mechanics.
Stiff settings verify conservation and the passive temperature maximum
principle, rather than claiming the uncapped explicit rate was accurate.
Physical source flags, fuel and temperature—not a burn timer—control reaction.
