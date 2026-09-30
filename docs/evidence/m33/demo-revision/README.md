# Demo presentation revision

The operator rejected the original sparse/poorly framed demonstration visually.
Changed ordinary authored demo content: larger framed screen, 640×360 target,
separate camera/stage sightlines, coloured backdrop/subjects, ordinary gravity/floor
and moving rigid subject. The clearer demo exposed the pre-existing primitive-box UV defect: differently
ordered triangles shared one fixed UV pattern. Corrected Renderer planar mapping
from vertex positions for all six faces. Added a real GL four-quadrant front-face
check (16 interior pixel samples across both triangles); final focused suite
passes 52/52. White/untextured boxes and collision geometry are unchanged.
No physics, simulation or fluid changes.

`run.log`/`main.png` came from the older scripted harness, which draws geometry but
not offscreen camera passes; the white screen there is NOT M33 acceptance evidence.
`interactive.png` records the visibly torn pre-fix image.
`interactive-fixed.png` is captured through the ordinary InteractivePlay render loop
using the existing runtime screenshot option. Human acceptance remains pending.

The original engine correctness/performance evidence remains unchanged. The full production gate was not repeated for this localized UV correction;
the rebuilt focused renderer checks passed. The original small-fixture timings are not
performance claims for this larger target.

This
revision includes the generic UV correction.
