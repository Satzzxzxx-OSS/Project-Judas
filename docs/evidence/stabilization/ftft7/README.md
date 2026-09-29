# FTFT7 physical integration validation

Run from the repository root with a fresh evidence directory:

```sh
python3 scripts/ftft7_validation.py --output docs/evidence/stabilization/ftft7/new-validation
```

The new fixtures construct ordinary serialized scenes, build `RuntimeWorld`
and exercise `GameSession` / `StepPlayedWorld`. They compare Newtonian forces,
momentum, barycentres, orbit evolution and frame-point velocities with mechanics
calculated independently from represented authored inputs. Existing direct
physics, spacecraft/SAS, frame and attachment suites remain unchanged.

`development-*` preserves pre-fix witnesses and intermediate execution; final
acceptance is separately identified in `RESULTS.md`. Failures are never relabelled
as successful physical validation. No research-fluid prototype is modified.

The historical FTFT2 runner is reused without changing assertions or application
paths. This invocation permits the reviewed celestial-participant change in
`RuntimeWorld.cpp` through its dirty-source guard; unchanged persistence tests
and the FTFT1 runtime/editor smoke still execute. Its gravity output directory
is redirected to preserve historical FTFT3 evidence. Source hashes before/after
bind all results to the tested code. No human visual review is claimed.
