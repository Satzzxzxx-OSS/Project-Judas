# PR #1 integration regression follow-up

The added real-worker JudasJS localization test passed the unpublished-catalog
placeholder, published-key, missing-key and invalid-argument assertions on its
first run. One later, existing machine-path fingerprint assertion failed because
the startup-only script fixture remained registered in the copied project.

The fixture now removes its own script asset after the probe, before the
unchanged-baseline assertions. The affected test was rebuilt and all 41 checks
passed. No runtime correction beyond the reviewed PR was made. Original and
corrected logs are retained under `focused/` and `focused-followup/`.

The main production/async gate is run once with fresh output roots, reusing the
existing assertions and output adapters. Protected historical evidence is untouched.

The full gate's storage invocation used `pr1-storage-gate`, which the existing
M61 test refuses before doing any work: its destructive fixture cleanup requires
a basename beginning `m61-storage`. The sole gate failure (exit 2) is retained.
Only storage was rerun with fresh `.cache/m61-storage-pr1-integration`; all 28
checks passed. The production suite was not rerun, and the safeguard/runtime
were not changed. The generated editor `imgui.ini` was removed; operator files
and other processes were untouched.
