# Deformable Lab — M62

Open `deformable_lab.judasproj`, select `Scenes/flat.judas`, then Play, or launch
it using the normal standalone executable. `Scenes/radial.judas` uses the real
radial field centred below the laboratory, with the same assets and solver.
The purple oblique fixture has an ordinary higher-precedence uniform region.

## Controls

- WASD / mouse: move/look; Space: script-controlled jump.
- Aim at fabric and hold left mouse: apply a picked physical impulse.
- G: pick up/drop orange brush or grey press. T: throw it.
- V: curtain airflow on/off. R: release curtain top pins.
- C: animated figure root motion/turn on/off; skeletal clip remains active.
- B: physical loads on elastic and yielding specimens, and press, on/off.
- H: cycle the green specimen load between bending, compression and twisting.
  B removes the forces so you can inspect elastic recovery and the orange retained dent.
- P: spawn an independent cloth prefab in front of the view.
- F3: request/release the annex region. Walk to the far right end of the deck to
  inspect its independent cloth; releasing suspends it, requesting restores it.
- 1 / 2: replace scene with uniform / radial lab; F9: authored reload.
- F5 / F8: save / load `deformable-lab`. Wait for **completed** before quitting.
  The pause menu also offers save/load/delete. Quit and launch again to test a
  real cold-process restore; F8 loads the slot without resetting node/plastic state.
- Escape: pause/resume and release/recapture mouse.

## Stations, left to right

A blue curtain: top pins, cloth stretch/shear/bend/self-contact, optional air drag.
B red cape: top group follows the actual `Root/Elbow` resolved skeleton joint;
  a root-driven ordinary box is an intentionally simple authored body proxy.
C green elastic cantilever and orange yielding specimen: B applies ordinary
  external physical forces; unloading retains only material plastic rest flow.
D purple cushion, yellow drape and finite-mass grey support: mixed contact.
E small purple cloth: actual oblique gravity, independent of support orientation.

Models and scripts are ordinary project assets. No scene-name dispatch or native
lab mode exists. Cloth uses denser stored render bindings than simulation meshes.
The simple original animated bar is reused under its included license. It is
an articulated fixture, not a full-body skin-collision or clothing-fitting system.

Use G to physically bring the brush through the curtain, and fold/release it.
The supported discrete/substepped speed envelope excludes fast small objects
crossing an entire coarse triangle in a substep. M44 casts still query rigid
geometry; deformable ray picking explicitly queries its current triangle surface.
CharacterMotor does not acquire new deformable collision/event semantics.
