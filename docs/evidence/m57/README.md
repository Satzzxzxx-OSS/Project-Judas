# M57 candidate evidence

Checkpoint: `a428d98f3e57a7bc087f165f0d135368f3b4ffab`. Candidate completed 2026-10-05; **uncommitted, awaiting human visual/gameplay review**. No M58 work. Screenshots and assertions establish engineering behavior, not human acceptance.

## Implementation and consumers

[Material contracts](../../MATERIALS.md), [milestone handoff](../../M57.md), [public JS API](../../judasjs/materials.md).

- Immutable `.judasmat` resources plus isolated slot overrides serve ordinary primitives, static meshes and skinned meshes. Legacy, metallic/roughness PBR and unlit are separate choices. Core glTF primitive materials/maps/samplers are retained; pinned MikkTSpace supplies missing tangent frames, including mirrored seams and finite degenerate fallback.
- Renderer owns GL3.3 resources and GGX shading. Optional linear RGBA16F accumulation → exposure → luminance-preserving Reinhard → exact sRGB → display-stage UI. Base/emission textures decode before filtering; data maps and alpha remain linear. AO affects ambient/IBL only. Masked shadows discard the same alpha; blended primitives sort by object depth, do not write depth or cast shadows.
- Baked `.judasenv` contains diffuse irradiance, GGX-prefiltered specular levels and the correlated-Smith BRDF lookup. Deterministic offline Radiance import/bake uses source/settings/version identity. Environment orientation and background visibility are independent of gravity and lighting intensity. No runtime convolution/import tools required.
- M33 modern targets are scene-linear without exposure; legacy targets remain display images. M55 optical attenuation remains before display resolve and uses the existing interpolated boundary paths. Physics, liquid solver, animation poses and navigation are unchanged.
- AssetDatabase/async ResourceManager own decode/reload/cancel/reference lifetime; Renderer owns uploads/deletes. Scene/prefab fields use normal serialization/overrides. Canonical schema stays 5. Worlds using appearance or declaring M57 resources hash registered material/environment/mesh/texture content, including later JS assignments, without absolute paths. Projects without new optional content retain old fingerprint behavior.
- Editor creates/edits shared resources, assigns slots and edits/reverts instance overrides using existing transactions and the scene as preview. JS `Entity.material(slot)` supports `state`, `assign`, `set`, `clearOverrides`; `world.appearance` / `setAppearance` provide transient scene settings. Types/reference/inventory/live enumeration agree.

Ordinary projects: `projects/material_lab/material_lab.judasproj` and `projects/shooter_game/shooter_game.judasproj`. The lab shows roughness/metal rows, rotated/nonuniform normal maps, emission, cutout/blend, embedded multi-material GLB, a moving skinned bar, isolated JS override, prefab spawning and a camera screen. Spring Range changes material/tint/light settings only: normalized scene/prefab comparison confirms unchanged geometry, physics, navigation and gameplay fields; its existing scripts and project configuration are unchanged.

## Executed validation and chronology

**Single broad gate:** `python3 scripts/m57_validation.py`. [RESULTS.json](final/RESULTS.json) and [raw production results](final/production/results.json) retain commands, SHA snapshots and outcomes: one clean Release build, zero compiler warnings, **109 production suites**, async **12 cases / 246 checks**, all passing. No broad rerun after narrow corrections. Existing gate assertions/physics paths were reused with output-directory adapters to avoid writing protected historical evidence. Human validation explicitly not run by this gate.

**Final affected checks:**

| Check | Result / evidence | Establishes |
|---|---|---|
| Actual material GL | 40/0 [software](final/colour-slot-gl.log), 40/0 [GTX 970](development/native-colour-final.log) | Known linear/sRGB readbacks including black/white filtering, map channels, metallic/roughness, finite emission, AO isolation, UI exposure, normals/static/skin, cutout shadow, blend depth, IBL orientation/mips, secondary linear output/feedback/resize, missing/pending fallback |
| Ordinary application | 45/0 [software](final/colour-slot-application.log), 45/0 [native](development/native-application-final.log) | Async material/environment installation, two imported slots, cancellation/reload, prefab/instance isolation/revert, Stop cleanup, unchanged authored scene, path-independent/source-sensitive identity, M55 cameras above/below water, actual Spring Range/profile passes |
| Game regression | 14/0 [Spring Range](final/shooter-colour.log) | Captured playable startup, physical scoring hit, camera toggle, pause/fire consumption/resume, queued reload and normal shutdown |
| JS examples / type surface | 193/0 [cookbook](final/cookbook.log); [API check](final/api-drift.log) passes | TypeScript 5.9.3, 202 public symbols, 22 exports, 130 native operations, 13 callbacks, 23 examples, three negative drift checks, live VM enumeration; material example also exercises destroyed-handle ReferenceError |
| Fingerprints / persistence | 140/0 [fingerprint](final/fingerprint-followup.log), 307/0 [persistence](final/persistence-followup.log) | Existing canonical/save behavior survives conditional material dependencies |
| Particles | 28/0 [component/GL](final/particles-colour.log), 11/0 [application](final/particle-application-colour.log) | Existing update/culling/lifecycle and application pass remain intact after modern colour-path correction |
| Editor | [lab](final/editor-material_lab.log), [game](final/editor-shooter_game.log) | Real editor Play/Stop, screenshots, no authored scene changes; dummy/headless input/audio is not hardware acceptance |
| Export | [lab](final/export-human-lab-complete.log), [game](final/export-game-complete.log), [missing map](final/missing-map-export.log) | Complete registered assets/derived resources/licenses; actionable missing dependency rejection |
| Moved runtime | [latest read-only lab](final/readonly-package-complete.log), earlier command/exit [TAIL.json](final/TAIL.json), [game startup](final/moved-game-startup-followup.log) | Package outside repo, cwd `/tmp`, read-only installation, writable external save root, public reload/quit sentinel, standalone scene/resource startup |

Known-input GPU tolerances use analytic transfer/filter/channel expectations, not a CPU duplicate of the entire shader. GL checks are mechanisms, not proof of every imported asset or driver pixel.

Final reproducible focused entry points (output directories below were used):

```sh
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_material_tests docs/evidence/m57/final/colour-slot-gl projects/material_lab/Assets/environment/studio.judasenv
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/judas_material_application_tests docs/evidence/m57/final/colour-slot-application
./build/judas_material_tests docs/evidence/m57/native-final/gl projects/material_lab/Assets/environment/studio.judasenv
./build/judas_material_application_tests docs/evidence/m57/native-final desktop
python3 scripts/m57_profile_summary.py docs/evidence/m57/native-final
```

Native commands use the operator desktop display, not the offscreen environment. Do not replay them or the gate merely for checkpointing. Original gate and earlier command records are retained in `final/TAIL.json`; these test entry points create disposable copies and outputs.

### Genuine failures and narrow corrected follow-ups

Development logs retain the initial compile/fixture/API errors, the malformed generated Radiance fixture and correction, and these independent post-gate probes:

1. [Writer failure](development/writer-failure.log): invalid environment payload could overwrite a good asset. Writer now validates before opening destination; bounded hash validation and initialized material-parser temporaries. Final GL writer-preservation assertion passes.
2. [Declared dependency failure](development/declaration-failure.log): a legacy-mode world with a registered material later assignable from JS omitted those source bytes. Generic declaration-based fingerprint domain now covers them; affected application, 140 fingerprint and 307 persistence checks pass. Schema unchanged.
3. [Colour-slot failure](development/colour-slot-failure.log): ordinary Render texture fallback decoded after filtering, unlike material maps. Generic modern consumers now cache a role-correct sRGB GPU copy once from existing decoded texels; parent deletion releases it. Modern particle image and display-authored tint decode separately before multiplication. Final actual GL passes on both drivers; particles/game/application rerun; packages refreshed.

No historical expected hashes were rewritten. The broad gate's source snapshots describe that earlier candidate; [CANDIDATE.json](CANDIDATE.json) explicitly lists later source changes and affected coverage. `RendererMaterials.inl` contains corresponding final renderer helper changes; the broad scanner does not enumerate `.inl`. Final hashes cover it. Profile summaries were corrected to use measured fixed-step intervals rather than repeated frame-level observations. [Temporary-copy cleanup](development/GENERATED_FIXTURES.txt) records removed new fixture copies; original logs/captures/failures remain.

## M56 cost evidence

Final [native profiles/summary](native-final/SUMMARY.json), [environment/load notes](native-final/ENVIRONMENT.txt), [software summary](final/colour-slot-application/SUMMARY.json). Real NVIDIA GTX 970 4 GiB, 1024×768, swap interval 0, identical 45 bodies / 12 hinges / six agents / seven motors, controlled 1/60-s simulation input. Java and offscreen validation were concurrent; no operator workload terminated and no speedup claimed. Mode 0 is a compatibility reference retaining newly authored factors and loaded resources; original pre-edit views/profile remain under `development/baseline*`.

| Final mode | Frame median / p95 / max ms | CPU active median ms | Wait median ms | GPU main median ms | Fixed median ms |
|---|---:|---:|---:|---:|---:|
| Compatibility reference | 3.066 / 4.364 / 7.495 | 2.855 | .147 | .382 | 1.366 |
| PBR direct + HDR | 2.956 / 3.874 / 4.410 | 2.800 | .097 | .465 | 1.341 |
| PBR + environment/HDR | 3.158 / 4.139 / 4.681 | 3.040 | .101 | .541 | 1.491 |

Main includes display resolve (~.090 ms); do not add twice. Modern shadow ~.106 ms, UI ~.068 ms. Draws 87–90 plus 214 separately counted UI draws. No GPU drops or exhausted profiler capacity. Warm sample is 91 intervals per mode; excludes startup/pending GPU records and screenshot-adjacent frames. Capture stalls are 57–63 ms and retained separately; startup/import/shader scope ~187–229 ms. Unattributed median .003 ms. Inclusive script callback median .64–.70 ms. These controlled uncapped runs are not a hardware-independent FPS promise.

llvmpipe **software** median frame times are 19.85 / 24.63 / 29.01 ms, p95 21.47 / 27.61 / 32.10, max 22.16 / 30.74 / 36.56. They establish fallback cost, not native GPU performance. Older pre-correction native/software measurements remain separately labelled by directory; final claims use `native-final` and `final/colour-slot-application`.

Derived environment: 128×64 specular / eight levels, 32×16 diffuse, 64×64 BRDF, 128 samples, 170,140 disk bytes / 84,994 GPU payload bytes. Bake **.095 s**, byte-identical repeat. Main HDR 1024×768 plus depth: 8,650,752 estimated bytes. Modern appearance total **8,801,282** includes the environment and one 65,536-byte cached colour view; legacy reference retains the loaded environment but no HDR target. Resource resident estimate 595,579 excludes auxiliary HDR/generated mip storage; these are scoped byte estimates, not total VRAM. HDR cache bounded to eight sizes, lab secondary 384×216. Twelve fragment units required; both tested drivers report 32.

## Export, limits and human handoff

Fresh packages `/tmp/Judas_M57_Material_Lab` (32 assets, 9,147,967 bytes, .023 s) and `/tmp/Judas_M57_Spring_Range` (29 assets, 9,469,590 bytes, .051 s). All three final package executables match `build/judas` SHA256. The automatic reload/quit script exists only in a disposable smoke copy; human projects retain normal interaction. Final read-only asynchronous smoke completed with `M57_RELOAD_PASS`; its last process exit code was not retained, while earlier identical-path smoke has exit 0 in TAIL. Game watchdog expiry is an intentional startup bound, not normal quit evidence; application regression supplies normal shutdown coverage. Source HDR/baker and docs are not needed at runtime; licenses accompany exports.

Limits: self-contained glTF, one mesh node / at most one skin, TEXCOORD_0, existing 48 skin joints / 128 nodes, no material-extension effects beyond unlit, no external images or texture transforms. Transparent sorting is per object, not per triangle; no refraction/GI/dynamic probes/bloom/auto-exposure. Light units remain authored engine multipliers. One-time Render-slot colour-view creation performs context-thread readback/reupload because GL3.3 lacks texture views; steady frames reuse it, with extra texture bytes. Legacy display targets remain live compatibility images; modern targets avoid duplicate exposure. Editor preview is in-scene, not a separate preview renderer. Existing animation interpolation is unchanged. Human input/audio/visual quality remains to be reviewed.

1. Compare Spring Range appearance and play both camera modes.
2. Confirm targets, enemies, score, pause and restart still work.
3. Inspect material differences, normal maps, cutout shadows and reflections.
4. Edit a shared material, override one instance, then revert.
5. Check the moving skinned example, camera screen and exposure/UI (lab V/B/N).
6. Check accepted liquid demo waterline/underwater view without physics changes.
7. Reload/Stop, test moved exported game and inspect M56 performance.

[Exact changed files](CHANGED_FILES.txt), [final source/asset fingerprints](FINAL_FINGERPRINTS.json), [candidate/protection record](CANDIDATE.json). Operator roadmap SHA remains `d07d031090852fe9a138e287338837db29d93e1a79e21bc32e713b3ac649ad12`, unchanged/unstaged, excluded from M57. Protected historical tracked evidence/research and accepted liquid/navigation implementation remain unchanged. HEAD/main/origin-main still match the starting checkpoint; index empty. Nothing committed/pushed/tagged.

Proposed commit after approval: `Add M57 PBR materials and environment lighting`.
