# FTFT1 real application persistence smoke

From the repository root, after building `judas` and `judas_editor`:

```sh
python3 docs/evidence/ftft1/runtime_smoke/run.py
```

This launches the real standalone application and editor for three saved-state
cases: matching baseline, wrong fingerprint and legacy version 1. Each project
contains an exact copy of `projects/tiny_game/Scenes/main.judas`; its save lives
under this evidence directory. The source project is not edited.

`results.json` records commands, timings, binary fingerprints and before/after
SHA-256 hashes of source scene, case scenes, saves and projects. Standalone
acceptance requires the expected process result and actual load diagnostic.
The valid standalone case runs the existing harness for three fixed steps.

The editor's existing autotest attempts Play at frame 21, captures frame 140,
Stops and checks authored-state restoration before exiting. Its exit code alone
does **not** establish that Play succeeded. Automated checks compare actual
frame-140 physics counts (live bodies for the valid case, none for rejected
cases); `visual_review.json` separately records inspection of the menu/status
and profiler in `editor.play.png`. A fresh reproduction requires inspecting its
new screenshots again; the script does not fabricate visual-review results.

Rendering uses SDL's offscreen driver and Mesa software GL. The legitimate
blocking-resource setting is explicit. These runs validate persistence callers,
not asynchronous resource loading, arbitrary editor workflows or full FTFT6.
