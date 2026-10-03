# M50 — current JudasJS documentation/tooling candidate

Starting HEAD: `19613298a55a2095cc856a7462f5c0babe4c1b17`.
Human developer review is **pending**. Nothing committed/pushed/tagged.

## Scope and authority

Actual `src/ScriptSystem.cpp` virtual module, native operation dispatch,
callback/state implementation and called runtime systems were traced directly.
The [landing page](../../JUDASJS.md), [declarations](../../judas.d.ts),
[143-symbol inventory](../../judasjs/API_INVENTORY.md) and subsystem pages cover
current public behaviour, including M49. Older milestone docs link to this current
reference; milestone architecture and protected evidence retain their scope.
No runtime source or packaging semantics changed. CMake adds only a focused
example-test target against the normal engine. No post-M50 feature work.

## Executed checks

- Standard TypeScript **5.9.3**: strict declarations + all 15 JS example files,
  ES-only standard library. Developer checker was temporary, not a game dependency.
- AST declaration/source/inventory coverage: **16 exports, 143 public symbols,
  87 native operations, 12 callbacks**. Actual QuickJS VM surface matches.
- Three negative controls reject missing member, phantom method and removed setter.
- Existing Release build: focused target compiled, **zero compiler warnings**.
- All **16 runtime cases** pass (contacts.js runs once as collision and once as
  trigger), with normal RuntimeWorld/QuickJS/physics/UI/resources. Independent
  spawn, lifecycle, input, queries, audio requests, particles, UI click, scene
  request/session, joints, animation layers/crossfade, ragdoll, motor and stale
  Entity/Character outcomes are in [RESULTS.json](RESULTS.json).
- Ordinary current character-project startup passes. Real outer-frame application
  reload retains session value `1 -> 2` and reconstructs scripts, then quits.
- Python runner syntax and local reference links pass.

The first example run had 129 checks / three fixture-outcome failures: extra
colliders polluted contacts/sensor results; launch arrived before motor support.
Only fixture setup changed. The three affected reruns had 27 checks / zero failures.
The original log and follow-ups are preserved; no fictitious all-green first run.

The first application reload smoke used the deterministic step runner, which does
not invoke UI frames/apply queued transitions. It ran without a script fault but
could not exercise reload. The affected follow-up used the ordinary application
loop and passed. This is a tooling setup correction, not a runtime defect.
Offscreen EGL/mouse-capture notices do not establish desktop hardware validation.

No production-suite or export rerun: no engine runtime/export behaviour changed.
The new runner supports repeatable focused checks without calling either.

## Real documentation discrepancies corrected

Earlier typings were incomplete. Current signatures now include nullable query/
component results, structured snapshots and actual methods/accessors. Older current
prose claiming menus pause all scripts and no main-view switching was superseded
by M41 UI phases and M49 `world.setView/clearView`. A query example now safely
checks nullable hit.entity. No bindings were renamed/added or runtime bugs fixed.

Read the subsystem pages for genuine limitations: trusted synchronous JS, bounded
plain state (not VM heap), no JS disk-save API, ASCII UI text, no arbitrary bone
setter/external JS pose source, existing cast/ragdoll/motor/audio/input limits.
Types cannot prove finite numbers, runtime phases, registry membership or budgets;
structured-value semantics remain explicit manual-review exceptions.

## Review and provenance

[HUMAN developer checklist](../../JUDASJS.md#human-developer-review).
[Exact changed files](CHANGED_FILES.txt).
[Final tooling/docs and runtime-authority fingerprints](SOURCE_SHA256.json).
Protected FTFT/P1, M33–M49 history/failures, fluid production code/demo and
third-party dependencies remain unchanged. Only this new evidence directory added.
