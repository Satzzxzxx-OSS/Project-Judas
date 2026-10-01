# M36 candidate results

- Baseline: `feba69f7dedcad21f6c89f2fbf304d7695dd03e7`.
- A clean Release build completed with zero compiler warnings. The first build attempt failed on an editor type alias before tests ran; that evidence is preserved.
- One complete production run executed 66 suites. Nine initially failed; raw failures remain under `final/`. The ordinary runtime-creation preflight serialized an unassigned ID instead of its validated copy. Correcting that repaired the lifecycle, persistence, orbital, fluid and render-camera regressions. Project asset-count expectations now include the shipped prefab. Two fingerprint reference checks explicitly move from schema 4 to declared schema 5; an independent Python byte-layout/SHA-256 oracle supplies the new golden value.
- All nine affected suites passed in focused rebuilt reruns (`followup/`, `followup2/`). This is an aggregate passing result, not a claim that the original full run passed. Final corrections received narrow rebuilds, not another complete clean build/full-suite cycle.
- Prefab checks: **34/34**. Actual Application prefab startup/render/spawn/shutdown: **12/12**. Real async application integration: **12 cases, 246 checks**, passing. Editor async Play/Stop and standalone reference startup passed. World-state followup: **307 checks**, passing.
- No assertion tolerance or physical fixture was weakened. Existing physics/fluid algorithms were not changed. Protected historical evidence and research prototypes remain unchanged.

## Lightweight performance

Measured authored instantiation totals: 1 = **3.086 µs**, 10 = **94.448 µs**, 100 = **8.926 ms**. One runtime hierarchy spawn = **371.165 µs**. These are local CPU observations, not universal guarantees. Complete build/production/smoke invocation took **692.188 s**.

## Scope and human gate

Human prefab validation remains **pending**. See `docs/M36.md` for hierarchy, runtime component and persistence limitations. No nested prefabs, variants, structural overrides, implicit rigid joints or source-file undo are claimed. No commit, push or tag was made.

`SOURCE_SHA256.json` records the final implementation, fixture and independent oracle fingerprints. `RESULTS.json` links the executed reruns. Original failure evidence is retained unchanged.
