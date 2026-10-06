# Final safety follow-up

After the single production gate, final review found two narrow M62 integration defects.

- `invalid-derived-unfixed.log`: finite coordinates could overflow derived area/volume;
  finite density/weights could overflow node mass. Five new checks failed. Asset
  preparation now rejects nonfinite derived geometry/render data. Initialization,
  material changes and saved state require representable positive masses; material
  preflight is atomic. Failed lazy initialization returns a diagnostic and removes
  the incomplete record rather than throwing through the JS native boundary.
- `destroy-cleanup-unfixed-corrected.log`: authored reset cleared deformation before
  script destroy callbacks. A callback could recreate a lazy instance during cleanup.
  Both lifecycle checks failed. Scripts now retire before component clearing in
  authored reset and world destruction, so callbacks inspect the existing state and
  fresh instances are constructed only after cleanup.

The first cleanup fixture mistakenly treated `GameSession::End` as authored reset;
that function only detaches the session. `destroy-cleanup-unfixed.log` preserves this
fixture mistake; the corrected reproduction explicitly calls `RestoreAuthoredState`.
`cleanup-regression-build.log` also preserves a test attempting to inspect private
mass storage; it was corrected to the public total-mass query. The load fixture's
already-generated upper/lower groups are cleared before rebuilding them, and its
normal topology validation now passes.

The clean Release gate had one test-only misleading-indentation warning, corrected
with braces without altering the loop. `final-safety-build.log` and
`final-cookbook-build.log` contain no warnings/errors. No solver quality/cadence or
physical constitutive/contact law changed in this follow-up.

`safety-followup/RESULTS.json` records the affected core, cookbook/API, actual editor,
resource/lifecycle/cold-save/streaming and re-export checks against the final source.
The unchanged performance benchmark and broader production results remain in
`focused-final/` and `final/`. The full production suite was not repeated.
