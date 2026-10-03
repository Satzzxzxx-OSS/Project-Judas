# M54 development failures and follow-ups

These are genuine development observations. They are not fabricated adversarial fixtures.
All earlier milestone/prototype failure evidence remains untouched.

- The first original-project generator wrote the gravity region in the wrong
  scene grammar. `lab-bake.log` preserves rejection. The first correction then
  exposed liquid validation being performed before collider fields were parsed;
  `lab-bake-followup.log` preserves that rejection. Decoding was moved after
  ordinary collider fields.
- `lab-bake-followup-2.log`: prefab resolution lost repeated compound-box entries
  because the shared property map overwrote a repeated key. The generic map now
  indexes repeated compound-box/cavity properties and emits normal repeated scene
  lines. `lab-bake-corrected.log` and application checks confirm all five bucket
  walls survive. This is a shared prefab correction, not liquid-only serialization.
- `geometry-first.log`: otherwise-linear deep capacity tables were needlessly
  subdivided by roundoff. The bake recognises numerically linear intervals; the
  preserved follow-up records two samples for both 10 m and 100 m tanks. All
  actual vertex knots remain; a coarse radial-knot substitution was removed so
  the between-knot cubic bound remains valid.
- `application-first.log`: a bucket test opening was above the actual surface,
  so it correctly stayed empty. The fixture was placed deeper, not the hydraulic
  rule weakened. `application-followup.log` demonstrates conserved scoop/pour;
  it also preserves a radial HUD error: an Entity wrapper for absent lab-only
  IDs throws on component access. The project HUD now checks `valid` before
  querying those optional scene owners. No runtime safe-handle semantics changed.
- The first cookbook type check found its sample-result field absent from the
  initialized state shape. The example initializes the actual field explicitly;
  final TypeScript/runtime checks cover it.
- Early headless world screenshots omitted the UI pass and showed the character
  proxy from inside its first-person view. The application check now renders the
  normal UI pass; the demo motor has no unnecessary visible proxy. Starting
  viewpoints face the basin. Final world+HUD images are under `final/visual`.

Later narrowed application/geometry logs record corrected checks. No historical
FTFT/P1 evidence was modified, no P1 prototype was merged, and no waves were added.

## Narrow final follow-ups

- The first queued-transition probe timed out. `final/transition-timeout.log`
  and `final/transition-diagnosis.log` preserve the actual failure. The disposable
  JS probe read its next session stage in remaining fixed steps before the queued
  reload completed, and omitted the radial scene's declared `radial` property.
  The fixture now waits after requesting a transition and declares that ordinary
  property. No scene-transition runtime semantics changed. The resume helper's
  first attempt also preserved an existing-package precondition failure in
  `final/followup-tool-precondition.log`; it now retains the previous package
  under a separate `/tmp` name before the affected re-export.
- Source review found a genuine liquid-only unsupported-gravity defect: a moving
  container retained its last valid query geometry and an error was not cleared
  on return. `final/unsupported-gravity-first.log` records one failing regression
  assertion. A suspended-equilibrium flag now retains the ledger quantity while
  excluding stale surface/query/flow participation. Return to supported gravity
  resolves the current pose and clears the error. `final/liquid-followup.log`
  (67 checks) and `final/application-followup.log` (41 checks) pass, including
  ordinary world movement outside/inside the authored gravity region. Types and
  reference expose the actual `equilibriumValid` snapshot.

The clean Release/100-suite/async gate completed before this narrow liquid-only
correction. It was not repeated. Affected liquid targets and executables were
rebuilt with zero warnings, then liquid/application, all cookbook/API checks,
editor bake/Play-Stop, moved export and queued lifecycle probes were rerun.
Final fingerprints describe that corrected candidate; the earlier gate's own
source snapshot remains preserved.
