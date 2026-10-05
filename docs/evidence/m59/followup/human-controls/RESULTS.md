# M59 human controls follow-up

Human test exposed an actual project-script defect: Q called `world.viewRay()` although the real API exposes `world.viewRay` as a property. QuickJS threw TypeError in Residency.update; existing ScriptSystem error isolation faulted that player instance, so movement/view/UI/function-key callbacks stopped. The correction changes only that property access in the current project's JS. No engine/runtime binding or physics semantics changed.

A separate manual-control authoring error started the next gallery index at zero despite start() already requesting gallery-0. The first F9 therefore coalesced with the existing request. The index now starts at one.

Extended actual SDL application input coverage now verifies Q on empty space; Q picking/adopting a real streamed crate; finite-force carrying/release and movement afterward; first F9 loading gallery-1; F7 loading conserved liquid; F10 releasing manual demands while retaining safety-pinned water. Source and moved read-only export both passed 48 checks, zero failures, including existing reload/locale/cleanup checks. No production suite repeated.

The first extended fixture passed all carry/function-key checks but its immediate shot at the already-impulsed plate missed (48 checks / 1 failure). This diagnostic is preserved. The fixture now waits 45 ordinary frames for the hinge's initial test impulse before aiming/firing; real logical input, query, impulse and score paths remain unchanged. Corrected source and moved runs passed.

Only residency.js and StreamingApplicationTests.cpp changed in implementation scope. Evidence/results/fingerprints refreshed; original human failure and pre-follow-up fingerprints retained. Existing performance captures and broad suite remain applicable to unchanged runtime code. Previous 37-check export evidence remains historical; current export has 48 checks and a changed authored baseline because project script bytes changed.

Corrected desktop reopened at clean initial state; human re-test pending. Nothing committed, pushed or tagged. Protected historical evidence and operator roadmap unchanged.
