# M48 candidate evidence

Starting HEAD remains 4c2ee4631ec36f29160c5c5a9a6b5c6c24039956. M47's separate
internal candidate is under docs/evidence/m47/internal, recorded before M48 code.

Focused Release targets: judas_ragdoll_tests and judas_ragdoll_application_tests.
Commands/environment: SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1
SDL_AUDIODRIVER=dummy JUDAS_WORLD_STATE=none; run from repo root, --output build/...
Raw logs under focused/. initial.log passed 28 checks; complete-rotation.log adds
one compact rotated-world/gravity equivalence (33 checks total). The real normal
Application/JS test passes 4 checks in 33 frames. application-initial.log preserves
the failed original demo string syntax; application.log is the corrected result.
No physical solver defect or alternative solver was involved in that correction.

The focused test uses StepPlayedWorld for the actual fixed-step path, including
Judas gravity, contacts, M45 hinges, pose readback, normal entities/queries and
cleanup. A three-body articulation and five-articulation group are timed with
all fixed-step work included. Raw numbers are sanity measurements, not universal
performance bounds. Candidate gate/fingerprints are under final/ after completion.

Authoring and accepted scope for review: docs/RAGDOLLS.md. No human validation is
claimed by offscreen automated execution. No commits, pushes or tags.

Final record: RESULTS.md / RESULTS.json. The final 36+4 focused checks include
two reproduced late cleanup defects and their narrow correction. Final source
fingerprints are lifecycle-followup/SOURCE_SHA256.json; the original clean
89-suite/12-case async gate is preserved in final/.
