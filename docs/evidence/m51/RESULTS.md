# M51 candidate results

Starting HEAD remains `74da7b534831d7cb7043ed1c02970283f7078daf`.

- Clean Release build: passed, zero compiler warnings.
- Production: **93 suites passed once**; async integration **12 cases / 246 checks** passed.
- M51 composition: **26 checks**, zero failures.
- Public API: **16 exports / 146 symbols / 91 native operations / 12 callbacks**;
  TypeScript 5.9.3 and live VM enumeration passed, including three drift-negative controls.
- Cookbook: **137 checks**, zero failures, including the new physical-control example.
- Editor Play/Stop: authored scene identical; standalone, all four scene transitions,
  reload and relocated export from an unrelated working directory passed.
- Narrow final cursor cleanup: affected migration/editor/transitions/export checks
  passed; no second production run.
- Representative workshop fixed-step average: **0.380 ms**, including physics,
  two motors and project policy; not an isolated JS/native microbenchmark.
- Export package approximately **7.54 MB**. Runtime packages contain normal project
  assets/configuration, not this documentation/evidence tree.
- Human review remains pending. No commit, push or tag was made.

See [audit](AUDIT.md), [full results](final/RESULTS.json),
[source hashes](final/SOURCE_SHA256.json), [reviewed hashes](final/REVIEWED_SHA256.json),
[exact changed files](CHANGED_FILES.txt), and [workflow/limitations](../../M51_ENGINE_BOUNDARY.md).

Protected historical evidence/prototypes and accepted production fluid source/demo
remain unchanged. ROADMAP.md remains absent. No post-M51 feature was started.
