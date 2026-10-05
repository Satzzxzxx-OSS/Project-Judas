# M60 human door failure and content correction

Human report: E did not open the listening-lab door. The door is intended to move physically.

## Measured cause

The normal project/input/JS path enabled the hinge motor at -1.5 rad/s and its
80 N·m torque bound. With the original content the angle remained effectively
zero after three seconds: contacts existed against the floor, both jambs and header.
The leaf exactly filled the opening (half extents 1,1.5,.15; centre -8,1.5,0;
pivot -9,1.5,0), leaving no swing clearance for its finite thickness.

Test-only removal of floor/frame colliders let it reach -0.44729 rad, then contact
with physical Hall marker 42 stopped it. That colour marker was an unintended
0.2 m raised threshold in the swing arc. Removing that collider too in the
isolation fixture let the unchanged motor reach -1.499968 rad and close normally.
See original `baseline.log`, `isolation.log`, `isolation-all.log`; failures retained.

## Exact correction

Ordinary content only: leaf half extents .8,1.44,.15, pivot -8.8,1.5,0 and local
anchor -.8,0,0. Hall/booth colour markers are thin render-only overlays. Authoring
generator updated to agree, without running it over accepted Spring Range content.
No input, JS, physics, constraints or audio runtime semantics were changed.

The application test now has a focused `door` mode: real logical E input, two
open/close cycles, actual angle assertions and closed/open obstruction ray tests.
`door-isolate` remains a test-only diagnostic and is never used in normal play.
Previous callback-health coverage was insufficient to prove physical movement.

## Affected verification only

Focused Release target builds succeeded. Corrected content: **15/15** checks;
angle -1.499974 rad open, -0.000026 rad closed; repeated twice, no contact jam;
real doorway ray blocks/clears accordingly. No script faults or shutdown failure.
Export rebuilt normally: nine assets, one scene. Moved read-only package runs
from unrelated /tmp cwd, exits zero with no script/asset failure. Original package
results remain preserved; current lab export supersedes its previous content.
No broad production/async/audio suite repeated for this authored geometry fix.
Human interactive/listening re-test remains pending; no claim of auditory acceptance.

Files changed for correction: `projects/audio_lab/Scenes/listening.judas`,
`tools/AudioDemo.cpp`, `tests/AudioApplicationTests.cpp`, current M60 evidence.
Original candidate fingerprints preserved here before refreshing the final manifest.
Protected history/research and accepted simulation implementations are untouched.
No commit/push/tag, no M61.
