# M37 candidate evidence

From repository root:

- Focused build: `cmake --build build --target judas_visual_particle_tests judas_particle_application_tests -j 4`
- `SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_visual_particle_tests --output build/m37-focused`
- `SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_particle_application_tests --output build/m37-application`
- One clean Release/production/editor/standalone gate: `python3 scripts/m37_validation.py` (refuses to overwrite evidence).
- Human demo: `./build/judas_editor assets/scenes/particle_demo.judas`, then Play; or `./build/judas assets/scenes/particle_demo.judas`.

The focused C++ test's `--demo <path>` reproduces the authored demo using normal
scene serialization. The small soft texture is a procedural radial alpha mask.
No third-party art/dependency was added.

Focused tests use the production Renderer/EngineHost/RuntimeWorld/pool.
Application tests use the ordinary asynchronous Application/InteractivePlay loop.
Offscreen software GL screenshots and counters are machine evidence, not human
visual acceptance. Rendering CPU timings are local observations; they do not
claim a GPU latency guarantee. Raw development build failures are retained.
Protected historical evidence is never overwritten. Binaries/cache stay in
ignored build output. Human visual acceptance: PENDING.
