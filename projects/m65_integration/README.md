# Judas developer integration — M65 candidate

Open `m65_integration.judasproj` in the Judas editor and press Play, or run:

```sh
build/judas projects/m65_integration/m65_integration.judasproj
```

F1 selects the original integration lab; F2 opens the permitted Rooftop Run copy.
WASD/mouse/Space move, look and depart from support. Escape opens the authored pause
menu. G wakes the articulations, P spawns another, B toggles the rider's ragdoll;
J/K toggle/re-anchor a physical connection, M changes its physical material.
T requests/releases the annex, Y adopts its complete socket assembly; F6/F7 save/load.
Backspace reconstructs the current scene.

The lab has a slope/step/edge/platform course, motor sensor checkpoint, a tilting
board with independently targeted feet, a final-pose hand socket, and resting
articulations/debris. Game policy remains in ordinary project scripts.

The simple original rider reproduces an integration problem; it is not the original
Skate/Mixamo character. `Assets/models/LICENSE.txt` covers the original rig/checker;
`Assets/fonts/LICENSE.txt` covers the copied font. `ROOFTOP_ORIGINAL_README.md` is
preserved consumer documentation. Consumer workspaces are not edited or built.

Full workflow, limitations and human checklist: [M65](../../docs/M65_INTEGRATION.md).
Named UI source: `tools/integration_ui.json`. Export uses the normal M38 workflow.
This is an uncommitted review candidate, not recorded human acceptance.
