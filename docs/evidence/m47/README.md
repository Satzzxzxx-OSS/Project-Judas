# M47 internal candidate boundary

Starting HEAD: 4c2ee4631ec36f29160c5c5a9a6b5c6c24039956. Uncommitted M47 candidate recorded before any M48 implementation. Scope: PoseComposition, WorldAnimation resolver, authored layers, JS/editor integration, focused tests, pose_demo. See internal/SOURCE_SHA256.json for the exact pre-M48 scope/fingerprints.

Focused commands: Release configure, build judas_pose_composition_tests and judas_pose_composition_application_tests; execute CPU test from repository root and application test with SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy JUDAS_WORLD_STATE=none, --output build/m47-app.

CPU: 16 checks pass after correcting the test expectation to include the deliberately authored parent scale of 3. The original failed output is preserved. Real Application/JS: 6 checks pass including crossfade/layers, prefab independent state, external source validation, stale handles, reload, pause and shutdown. Three-layer/four-node composition: see raw microsecond measurement in pose.log. This is a small CPU sanity measurement, not a general animation performance guarantee.

No production suite yet; combined M47/M48 candidate gate will run once. Human visual review is pending. Limits and architecture: docs/POSE_COMPOSITION.md.

## Combined follow-up

M48 is now a candidate using the external seam. The single combined gate and
final M47 tests/performance are recorded in ../m48/RESULTS.md. The pre-M48
internal fingerprints above intentionally remain historical and unmodified.
