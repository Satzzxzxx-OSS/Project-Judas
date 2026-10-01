# M35 focused input evidence

Focused command:

```
cmake --build build --target judas_input_tests judas_project_tests -j4
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_input_tests
./build/judas_project_tests
```

Single completed clean Release/production/editor/standalone gate:
`python3 scripts/m35_validation.py`. Existing production assertions and smoke
fixtures are reused, with evidence outputs redirected rather than modifying
protected FTFT/P1/M33/M34 evidence. The script refuses to overwrite a run.

The first fresh build stopped on missing InputSystem linkage in old standalone
test targets; no production suite ran from that failed build. Its output remains
under `build-failure/`. CMake source lists were corrected and a new genuinely
clean directory was configured for the completed run.

The focused test uses actual SDL key/mouse events and a tiny SDL virtual pad.
SDL suppresses joystick input for hidden windows by default; that test alone sets
SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS. No runtime focus policy was changed.
Raw initial failure and diagnosis logs remain preserved. This proves adapter/state
behaviour, not human controller feel or physical hardware acceptance.

`project_map_writer.cpp` exports the actual engine default map for shipped project
authoring. The executable stays in ignored .cache; no binaries are evidence.
