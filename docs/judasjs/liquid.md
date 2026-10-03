# `liquid` and `Entity.liquid` — conserved liquid (M54)

Judas owns quantities, geometry and transfers. JS owns why a game drains, scoops
or displays water. This API is separate from legacy `world.fluidSample` (PBF).
SI units: m³, kg/m³, metres, seconds. One litre = .001 m³.

## `entity.liquid: LiquidVolume | null`

A component becomes available after its CPU assets load and the next fixed step
registers it. Query again until non-null. `LiquidVolume.handle` is an opaque
world-generation token: do not construct/parse it yourself. `valid` returns false
after removal/world destruction; state/control on stale handles throws
ReferenceError. Entity handle lifetimes follow [entities](entities.md).

## `LiquidVolume.state`

Detached snapshot: `entityId`, `entity`, `enabled`, `equilibriumValid`, `container`, `material`, `density`,
`volume`, `capacity`, `stableCapacity`, `coordinate`.
`coordinate` is basin-local equilibrium distance (plane coordinate or radius),
not necessarily world height. `stableCapacity` changes with container orientation.
Volume is authoritative; fill percentage is merely volume/capacity.

## `LiquidVolume.enabled = boolean`

Disabling removes query/flow participation but retains owned volume. Scene reload
restores authored initial volumes. Destruction retains the volume as a parked
parcel instead of silently deleting it.

## `source.transferTo(destination, cubicMetres): number`

Use in `fixedUpdate`. Returns actual transferred volume, bounded by source and
destination capacity. Same material identity/density required. Invalid negative
or nonfinite requests throw TypeError; stale handles throw ReferenceError.
Disabled/self/incompatible transfers return zero. Debit/credit are one operation.
This is an explicit plumbing/storage transaction, not the physical scoop API.
Scooping/pouring are automatically driven by authored openings and geometry.

## `liquid.sample(point): LiquidSample | null`

World-local coordinates around Judas's fixed double-precision absolute origin.
Returns null when dry/disabled/outside authored geometry. Snapshot fields:
`entityId`, `entity`, `material`, `density`, `depth`, `coordinate`, `surfacePoint`, `normal`,
`velocity`. The normal is local anti-gravity; no universal world-Y. M54 bulk
velocity is zero (no waves/currents). Radial curvature has declared bake error.

## `liquid.submerged(entity)`

Actual enabled box/compound collider integration. Returns `volume`, `center`,
`buoyancy` (newtons). Missing/stale collider throws ReferenceError; unsupported
shape throws TypeError. Read-only; automatic force application requires the
separate LiquidInteraction component. No entity-origin-only approximation.

## `liquid.accounting(material = 'water')`

Snapshot m³ totals: `reservoirs`, `containers`, `detached`, `total`, `expected`,
`error`, `tolerance`. Expected includes explicitly authored/runtime-spawned initial
volume. Different material identities have independent ledgers. Geometric solve
error never changes ledger volume. This is session runtime data, not a save API.

## Diagnostics

`liquid.errors`: `{entityId,message}[]`. `liquid.connections`:
`{entityId,active}[]` (hydraulically open above authored saddle; source/destination
keep their own volumes on opening/closing). Stable deterministic entity ordering.

## Boundaries

Static, baked basins; disjoint authored tetrahedral physical geometry. Uniform and
constant-magnitude radial conservative gravity supported. No dynamic surface/PDE,
particle bulk, live rebasing, sealed-air physics or nonconservative equilibrium.
Small containers use local uniform-gravity clipping and freely vented gas.
Detached ballistic parcels have no inter-parcel solver; unsupported solid landings
retain parked conserved parcels. Dry body mass/inertia are not automatically
rewritten from contained quantity. See [authoring](../LIQUID_RESERVOIRS.md),
[physics](physics.md), [scenes/session](scenes-state.md) and [character](character.md).

A moving container in unsupported/zero gravity retains volume and its authored
enabled state, but sets `state.equilibriumValid=false`. Its queries, surface and
transfers are suspended. Returning to supported gravity resolves the retained
volume at its current pose and clears the diagnostic. `coordinate` and
`stableCapacity` are last valid solve values while suspended.
