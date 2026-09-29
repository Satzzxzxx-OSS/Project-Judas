# FTFT6 integration evidence

Reproduce the current editor/project/lifecycle gate from the repository root:

```sh
python3 scripts/ftft6_validation.py --output docs/evidence/stabilization/ftft6/new-validation
```

The output must be new. The runner uses offscreen SDL/OpenGL and software GL;
this is automated behavioral validation, not human visual review.

- `JUDAS_EDITOR_STABILIZATION` drives ordinary editor requests and document
  transactions inside the existing editor frame loop. Play uses the existing
  `InteractivePlay::Frame`; it waits on real resource states and fixed steps.
- `judas_project_application_tests` opens that generated project through
  `Application::Run`, with normal asynchronous resources, real GPU readback,
  and the existing controllable frame-clock/observation interface. It explicitly
  disables saved deltas for this authored-startup comparison; the editor and
  lifecycle fixture separately exercise saved-state application.
- `judas_lifecycle_integration_tests` composes fidelity changes, real handle
  slot reuse, runtime creation/destruction, door/switch state and baseline
  rejection in one serialized ordinary scene. Fidelity policy is deliberately
  not persisted; retained entity state is reconstructed under current policy.
- `judas_asset_integrity_tests` exercises the actual asset database and files,
  including refusal without metadata/file mutation.
- The unchanged FTFT2 runner covers real async races and all current production
  suites. The FTFT1 runtime smoke is reused with a new output directory.

Development failures and pre-fix witnesses are historical observations, not
acceptance. Final `validation/` results and matching source fingerprints are
identified in `RESULTS.md`. No fluid prototype is executed or modified.
