# M58 candidate results — human review pending

Based on M57 `ba16c2c52c60194a95ff161d55b93ff874863b77`. No commit/push/tag.

## Validation

One clean Release build, **zero compiler warnings**. One **112-suite production**
run and **12 async cases / 246 checks**. The original broad record remains immutable:
111 suites passed; the render-free shooter harness failed two HUD assertions because
it never demanded/published the new asynchronous catalogs. Corrected only harness
readiness/consumer lifetime, kept all score/completion assertions, **36/36 passed**.
Effective final production status is 112/112; **no broad rerun**.

M58 final focused: **48** text/catalog/layout checks, **48** actual GL checks,
**33** localization/application/resource/lifetime checks. Relevant UI **22**, UI
application **16**, exporter **18**, all pass. Full cookbook **201 checks**; final
localization/surface follow-ups each **9**. TypeScript 5.9.3, drift negatives and live
QuickJS enumeration: **23 exports / 215 public symbols / 135 native operations /
13 callbacks / 24 example files**, pass. No permanent TS/Node runtime dependency.

Independent unmodified upstream hb-shape results cover Arabic, Indic and marks /
ligature / kerning; exact logical sources/font hashes/commands retained. Separate
UAX#9 visual-order/ICU visual-map checks cover wrapped mixed Hebrew/Latin/digits.
GL output matches an independent FreeType bilinear coverage oracle, with clipping,
exposure independence, atlas eviction and retained-layout reset checks. This is
integration evidence, not universal conformance/linguistic certification.

The late guard follow-up rejects malformed UTF-8 anywhere in a UI file, invalid
identity strings and NUL/newline/overlong keys. Embedded-NUL **text**, supplementary
characters and text/key precedence remain lossless in QuickJS. Only affected tests
were rerun. Export requirements were clarified to distinguish static Unicode data
from system platform libraries. The normal destroy callback also observed the last
session locale correctly (`destroy-locale-before.log`); no teardown runtime fix needed.

Editor **normal export + Play/Stop**, authored scene identical. Actual application
locale changes while playing/paused preserve pointer/modal/fixed-time policy; reload
retains locale, fresh session starts configured default, invalid catalog replacement
retains prior text, cancelled font jobs cannot publish, prefab UI reference preserved,
authored fingerprints are path/locale-independent and notice catalog byte changes.
The final numeric-locale gameplay probe exposed older InputMap/navigation file-format
streams inheriting the process's comma decimal separator. The pre-fix parse failure
is preserved. Explicit classic locale on those streams fixes only authored I/O;
no simulation, geometry, schema or baked asset changes. Input **40**, navigation
**77**, text **48**, shooter **36** pass after this narrow correction. Identical
**60-step** inputs yield the same **492** body/motor state values and baseline under
classic/comma process locales; current project/scene bytes also remain invariant.

## Native / performance

Real **GTX 970 / driver 580.178.04**, with operator applications left running and
concurrent clean build at the 100-label capture; see `native/environment.log`.
M56 120-frame CPU submission sample (no swap; includes HDR resolve + UI):

| 100 labels + changing numeric label | Cold ms | Median ms | p95 ms | Max ms | Glyph rasters |
|---|---:|---:|---:|---:|---:|
| ASCII | 20.920 | 1.404 | 4.675 | 20.920 | 23 |
| multilingual | 10.489 | 1.175 | 1.388 | 10.489 | 31 |

ASCII was sampled first; driver warmup/scheduling differ. **No Unicode speedup
claim.** Warm raster median is zero; repeated unchanged labels do not rerasterize.
Cold/warm layout/submission/raster scopes and memory are in M56 captures. Both use
one **4 MiB** atlas; native face bytes **759,720 ASCII / 5,752,416 multilingual**;
layout cache **42,174 / 65,263 bytes**. 100 compatible label batches produce 100 draws.
Resource/font bytes and process memory are separate from face-cache counters.

Normal async final moved/read-only packages (111 captured frames each, after the
numeric-format correction):
- `package-lab-numeric-final`: median **17.041 ms**, p95 **17.849**, max **414.701** (startup).
- `package-range-numeric-final`: median **17.115 ms**, p95 **17.995**, max **248.132** (startup).

Japanese locale published one frame after request in lab, two in Range. Switch
frames were around **17 ms including native swap wait**; no WaitForAll in these
probes. Startup/reload spikes remain visible; the separate forced-ready application
capture includes explicit test WaitForAll and must not be presented as normal switch
latency. Native Range fixed-step sample ~2 ms with 45 bodies. Software GL is additional
correctness evidence, not GTX hardware performance.

## Export / footprint

Both **exact ordinary project exports** moved outside repository/build, made read-only,
ran from `/tmp` with poisoned development root, absent ICU_DATA and normal asynchronous
resources. Package engine font paths, language/pause/reload/quit and invariant baseline
passed: lab **15 checks**, Range **17**. Saves resolve under writable XDG user data,
not installation. Narrow probe was removed from final human packages.

- `/tmp/Judas_M58_Text_Lab`: **52,762,847 bytes**, 12 assets; normal runtime **46,158,256 bytes**.
- `/tmp/Judas_M58_Spring_Range`: **53,070,114 bytes**, 35 assets; normal runtime **46,158,256 bytes**.

Final export time **0.092 s lab / 0.129 s Range**. Compared with recorded M57 Range
9,469,590 bytes, Range grew ~41.6 MiB. Full pinned ICU data archive is ~31 MiB and
linked statically; the complete current runtime is ~44 MiB. Dependency source inputs
are ~48 MiB; these source archives do not ship as development debris. Existing Linux
SDL2/glibc/libstdc++/OpenGL requirements remain; ldd has no external ICU/FT/HB libraries.
Font/license/dependency notices are included. Runtime notices explicitly credit the
FreeType Team. Both final packages were regenerated after the numeric I/O follow-up;
final 15/17-check native smokes used those read-only exports.

## Human handoff

- Open `projects/text_lab/text_lab.judasproj`: language buttons, Reload catalogs,
  resize; inspect mixed bidi numbers, marks/baselines, fallback, wrapping/clipping,
  unsupported glyph and missing-key examples.
- Open `projects/shooter_game/shooter_game.judasproj`: ordinary game controls;
  **L** cycles language; Escape pauses/resumes; Language button works in pause.
  Continue same round and inspect score/plurals/numbers and LTR/RTL menus.
- Edit a source `.judasloc` entry, reload, confirm no C++ rebuild; try Stop/reload
  and the moved standalone. F8 opens existing M56 diagnostics.
- Inspect editor project locale/font selectors, UI keys/direction and actual text preview.

Translations are demonstration content, **not native-speaker reviewed**. Horizontal
outline grayscale fonts only; no color/SVG emoji, vertical/ruby/rich text, text entry /
IME, automatic translation or typography certification. First async startup may show
[key] until resources publish; scripts refresh dynamic text via revision. Full scope /
cache limits and contracts are in `docs/M58.md` and `docs/judasjs/localization.md`.

Historical FTFT/P1/prototypes/M33–M57 evidence and accepted simulation work remain
unchanged. Operator docs/ROADMAP.md bytes and unstaged state are preserved. See
CHANGED_FILES.txt and FINAL_FINGERPRINTS.json for exact candidate scope.
