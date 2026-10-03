# Human-reported flat-area gravity defect — M51 follow-up

## Cause and correction

Content error: `Scenes/planet.judas` had exactly one gravity region, the broad
radial sphere. The intentionally flat local platforms had no uniform field/zone.
No CharacterMotor/JS/resolver implementation caches a planet-specific field.
`CharacterMotor::Step` calls `gravity.Sample(position)`; rigid-body preparation
samples the same field, and JS reads the motor's resolved gravity state.

The existing GravityContextMap uses the first containing registered region.
Added an ordinary uniform region **before** the planet region: centre `(0,32,0)`,
world AABB half extents `(9,5,9)`, authored identity orientation, magnitude 9.81.
It covers the local flat-platform area; the broad radial field still owns the
remaining planet. No engine/resolver/motor/JS source was changed for this repair.

## Preserved reproduction and proof

- `planet-before.judas`: byte-for-byte pre-repair content.
- `original-failure.log`: normal RuntimeWorld loads that preserved scene with
  the project's registries; sample at `(6,31.5,5)` was
  `(-1.81365454,-9.52168655,-1.51137877)` rather than local uniform gravity.
- `corrected.log`: **19 checks, zero failures**. Same point is now
  `(0,-9.81000042,0)`. Real motor sampling, unaffected remote radial sample,
  rigid-body sampling, continuous region crossing/return, oblique uniform
  orientation, independent support normals and stable motion are covered.
- `character-regression.log`: **32 checks, zero failures**.
- `m51-regression.log`: **26 checks, zero failures**.
- `radial-startup.log`, `export.log`, `moved-package.log`: normal project startup
  and relocated package transitions through the corrected radial scene passed.

Tests use ordinary authored Scene/RuntimeWorld/CharacterMotor and real physics;
no alternative gravity path, global-up fix, scene-name condition or JS force
substitution was introduced.

## Evidence scope

The original 93-suite production/async gate remains valid for the unchanged
engine implementation and was not repeated. Its old radial content smoke did
not prove mixed-zone authoring; that content-specific evidence is superseded by
this focused follow-up. Pre-repair final fingerprint files are retained here;
current fingerprints record the repaired scene and extended focused test.

The initial probe compilation used the wrong ResetBody signature (build.log).
A subsequent setup omitted the project's tag registry, so Build failed; the
probe incorrectly continued into an invalid world and its test process crashed.
Those setup logs are retained and explicitly are **not** successful defect
reproductions. The probe now loads the actual project registry and returns on
Build failure. `original-failure.log` is the successful corrected reproduction.

Human confirmation of this repair is pending. No commit/push/tag was performed.
