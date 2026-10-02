# M44 — candidate results

Starting checkpoint: `6258e7a00a8783b912ee98fa706eabb7c729fe4a`.
No commit/push/tag. Human visual acceptance remains pending.

| Check | Actual result |
| --- | --- |
| Fresh Release build | PASS; no compiler warnings/errors |
| Production suites (includes M44 targets) | 80/80 PASS |
| Real async application integration | 12/12 cases; 246 checks, 0 failures |
| Focused geometry/state/filter/tree | 26 checks, 0 failures |
| Real Application + JS | 4 named checks, 0 failures; JS assertions exercise all cast APIs, safe entity data, filtering, prefab hits and read-only behaviour |
| Editor Play/Stop | PASS; authored scene IDENTICAL afterwards |
| Standalone startup | PASS; ordinary project/script/runtime path |
| Export and moved package | PASS; launched from /tmp; JS ray hits entity 4 at 8.199999809265137 m |
| Protected evidence/prototypes | No tracked changes |

## Lightweight performance

1,024 bodies; the real tree returns 32 ray candidates. 10,000 raycasts take
49.690549 ms (4.969 microseconds/query); 10,000 radius-0.25 sphere casts take
55.028397 ms (5.503 microseconds/query). This is one machine/fixture measurement,
not a universal bound. Export: 5 registered assets, 1 scene, 6,966,296 bytes,
0.012 s exporter-reported time. Entire single build/regression/smoke gate:
647.280 s. No repeated full-suite run.

## Limits

Fixed-orientation translation against current resolved body poses, not target
trajectory prediction. Nearest hits only. Local float simulation coordinates.
Initial overlap normals are conventional/nonunique. Terrain ray/sphere/capsule
crossings use finite-resolution real-surface sampling; grazing/thin features can
be missed. Box-versus-terrain explicitly errors. The custom player capsule is
not a registered rigid-body target. Camera snapshot used by demo is one completed
view behind; explicit API origins/directions have no such dependency.

Headless GL smoke emitted the existing libEGL software-device warning; commands
and assertions passed. This is not human visual validation.

Full API/demo operation: `docs/PHYSICS_QUERIES.md`. Raw outputs and tested-source
fingerprints: `final/`. Original local wiring failures and corrected follow-ups:
`focused/`. Separate pre-existing `docs/ROADMAP.md` edit was left alone.

## Later human-input correction

See `INPUT_FOLLOWUP.md`: demo Resume focus/click handling and visible spawn
placement corrected. Affected Application check now passes 8/8 with actual SDL
P/Escape events. Original final-gate evidence/fingerprints remain historical;
latest source fingerprints: `INPUT_FOLLOWUP_SOURCE_SHA256.json`.
