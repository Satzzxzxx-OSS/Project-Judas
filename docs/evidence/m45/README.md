# M45 candidate evidence

Starting checkpoint: `fa2c281c399a848033768f253e277742625078eb`.

Reproduce focused checks with a Release build of `judas_joint_tests`, `judas_joint_authoring_tests` and `judas_joint_application_tests`. The application target uses normal SDL/GL/Application/QuickJS paths; headless environment: `SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 JUDAS_WORLD_STATE=none`.

`python3 scripts/m45_validation.py` performs the one clean Release build, existing production/async gate and disposable editor/standalone/moved-export smoke. It refuses to overwrite `final/`. Existing evidence/output adapters relocate generated files; the additional WorldState.cpp protection exception is for reviewed joint-reference preflight only. Historical evidence remains unchanged.

`focused/application.log` through `application3.log` preserve malformed demo-generation failures (not physical solver failures). `application4.log` is the corrected 8-check application pass. `solver2.log` is the 19-check solver pass; `authoring.log` is the 14-check authoring pass. Human acceptance is pending.
