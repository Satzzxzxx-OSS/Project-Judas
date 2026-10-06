# M63 human input follow-up

Human failure: windowed cursor escaped/clicked background windows; later demo
clicks did not register reliably. Original automated failure: [before.log](before.log).

## Causes and correction

1. Clicking static environment called `applyImpulseAtPoint`, which correctly rejects
   static bodies. The uncaught TypeError faulted the lab script, disabling subsequent
   shots and its pause UI. The project now applies ordinary prop impulses only to
   its authored `pickup` targets; static hits/misses remain valid handled clicks.
   The dynamic-body API and fracture simulation semantics are unchanged.
2. Window stored requested capture; reconciliation tested only that request. Backend
   relative-mode/grab loss without another focus-gained event was reproducibly left
   unrepaired. `Window::PollEvents` now reconciles actual SDL relative-mode and window
   grab while the window owns keyboard focus and capture is requested. It does not
   recapture unfocused applications or released modal UI. The native regression
   deliberately drops SDL capture without a focus event. This proves the repaired
   state gap; human confirmation of desktop/compositor behaviour remains required.

## Narrow validation

Release targets rebuilt with zero warnings. No full production rerun.

- [Native windowed application](after.log): 12 checks, zero failures: actual grab,
  recovery without focus event, repeated static clicks, menu input isolation,
  pause/resume, real prop hit-point impulse, clean shutdown.
- [Existing logical input](input-regression.log): 40 checks, zero failures.
- [Existing joint application capture/refocus](focus-regression.log): 11 checks,
  zero failures; native desktop with visible capture.
- [Moved exported project input](moved-input.log): 12 checks, zero failures;
  working directory `/tmp`, ordinary freshly exported content.
- [Export](export.log): 18 assets / 4 scenes, 50,710,339 bytes, 0.093 seconds.
- [Unmodified shipping executable](shipping/RESULTS.json): native startup, physical
  fracture probe, read-only package hashes unchanged, closes only its own window.

Final fingerprints are refreshed for Window.cpp, lab.js, application regression
and current follow-up documentation; prior fingerprints remain here as evidence.
Protected FTFT/P1 and M33–M62 evidence, fluid and other project content unchanged.
No commit/push/tag. Windowed human re-test pending.
