# Preserved development failures / corrections

No historical evidence was modified. These are ordinary M58 development results,
not fabricated adversarial cases. Final acceptance results live in `../final/`.

- Initial configuration/build caught a not-yet-written source and ICU header name;
  files/header corrected, focused builds continued. Upstream hb-shape first lacked
  generated include/link inputs; unmodified utility subsequently built and its
  output is retained with exact font hashes/commands.
- API verification first lacked the temporary TypeScript tool; a generated example
  then accidentally contained checker text. Corrected the example, preserved the
  failed logs, and executed the real QuickJS example and type/drift check.
- Actual GL coverage oracle failed: transparent **black RGB** atlas gutters caused
  bilinear sampling to multiply coverage twice and dim edge pixels. White RGB / zero
  alpha gutters fixed the real rendering defect; the original assertion remains.
- Application fixture initially used an old scene header, then a nonexistent timing
  accessor; corrected fixtures/accessor. A controlled delayed-job test originally
  waited on a gate while ReleaseAll correctly drained workers: interrupted only that
  owned test, then made CancelRequested release the worker gate before draining.
- Test input set before PollEvents lost edges; use real queued SDL key events as the
  current Application tests do. Pause/capture/time assertions were retained.
- Native application profiling was initially disabled by the normal environment
  configuration despite the test's pre-Run enable. The initial images/checks remain
  valid; frame measurements come from the explicitly `JUDAS_PROFILE=1` follow-up.
  No runtime workaround or profiling-system change was needed.

Translations/screenshots are demonstrative, not native-speaker or universal
script/typography certification. No protected fluid/navigation/physics code changed.

## Final numeric-locale follow-up

The first optional evidence probe used an unavailable `RuntimeDefinitions()`
accessor (compile error retained), corrected to authored scene object IDs. The
real two-pass probe then found a pre-existing input-map parse failure under an
explicit comma C++ numeric locale. Input/navigation metadata streams inherited
global punctuation, unlike the normal invariant scene format. Explicit classic
locale on these authored I/O streams is the small generic correction; no global
locale setting, simulation algorithm, navmesh data or schema changed. Final
project/scene round-trip, 60-step gameplay (492 identical state values and baseline),
40 input / 77 navigation / 48 text / 36 shooter checks pass. Two final read-only
native packages were regenerated and their 15/17 checks passed. Original broad
failures/captures and the pre-fix numeric failure remain intact; no broad rerun.
