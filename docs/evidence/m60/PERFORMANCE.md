# M60 lightweight measurements

Linux desktop: Intel i7-4790 3.60 GHz, GTX 970, driver 580.178.04. Firefox/browser
workers, ChatGPT, claude-desktop and KWin remained running. Nobody's applications
or OS volume were changed. These are short sanity samples, not latency guarantees.

## Prepared memory / worker latency

Four stereo-float pages at 48 kHz: **32–512 KiB/voice**, default **128 KiB**.
The fixture uses 2048-frame pages (**64 KiB**), identical for 60-second and actual
180-second media, and remains constant across repeated page consumption. Owned
PCM excludes decoder internal buffers, OS file caches and process RSS. The stream
counter conservatively includes configured capacity awaiting retirement removal.
32 live streams and 64 combined live/retirement admission slots bound outstanding
prepared PCM to 32 MiB at the largest permitted page size. Buffered PCM is shared
by asset, independently of cursor count; no reverb allocation until requested.

Final fixture prefill: WAV **1.09 ms**, MP3 **1.08 ms**, FLAC **1.11 ms**, including
1 ms readiness polling granularity. WAV seek **1.09 ms**, MP3 **21.42 ms**, FLAC
**2.18 ms** for the representative 30-second target. MP3 seek remains codec/content
dependent, not sample-exact. Its duration is explicitly unknown until decoded EOF;
startup performs no potentially full-file MP3 length query. Missing/corrupt input
fails safely. Longer tracks do not proportionally allocate PCM.

With a decode worker delayed **150 ms**, main-thread detach measured **0.0034 ms**
(device-free); existing real-device smoke maximum was **0.0124 ms**. This measures
indivisible main-thread graph detachment, not completion time of async retirement.
Pinned backend graph acknowledgment can wait for a current read and OS scheduling;
there is no strict lock-free/worst-case real-time guarantee. Retirement counts
returned to zero after trips and Stop. Forced starvation was counted/recovered;
normal demo and workload samples recorded **zero underruns**.

## Audio work comparison

`world-review.log`: production control plus offline mixing of 800 stereo frames
(16.67 ms of audio)/iteration. All times below in **ms CPU work**. This is **not**
a complete rendered frame, device latency or FPS. The warm ResourceManager retained
576000 bytes of shared buffered tone data even in the zero-active-voice row.

| Workload | Total median / p95 / max | Control median | DSP median | Stream PCM | Reverbs | Queries |
|---|---:|---:|---:|---:|---:|---:|
| No active voices | .00288 / .00438 / .01660 | .00209 | .00078 | 0 | 0 | 0 |
| 8 buffered | .06772 / .09034 / .44817 | .00708 | .06067 | 0 | 0 | 0 |
| 8 mixed, one stream, obstruction + reverb | .15148 / .29203 / .94202 | .01352 | .13637 | 131072 B | 1 | 420 total |
| 32 mixed, one stream, obstruction + reverb | .38307 / 1.02142 / 1.40334 | .02996 | .35302 | 131072 B | 1 | 480 total |

All rows end with zero pending retirements and underruns. Query limit is eight
eligible rays every 50 ms; muted/paused/zero-gain demand spends none. Raw logs also
include control and DSP p95/max separately. Retained root music is one stream;
three gallery trips retire all local streams while one adopted prop voice survives.
Audio introduces no region pin; ordinary adopted body ownership is retained root.

## Rendered final package samples

`RENDER_PERFORMANCE.json` is derived from the final moved/read-only normal runtime
profiler captures (119 recent steady frames each, roughly 100 FPS engine pacing).
Frame **interval** includes pacing/waits; audio-main is main-thread publication/
queries/retirement and excludes asynchronous decoding/device DSP.

| Package | Frame interval median / p95 / max (ms) | Audio-main median / p95 / max (ms) |
|---|---:|---:|
| Audio lab | 9.98 / 10.34 / 11.62 | .01221 / .02631 / .04119 |
| Streamed Spring Range | 9.99 / 11.01 / 11.24 | .01114 / .01673 / .03110 |

These brief startup-position samples do not claim listening/travel acceptance or
measure output-device latency. The lab/application has five persistent bindings,
two streams/262144 prepared PCM bytes, zero normal underruns; dry starting position
allocates no reverb bank. Actual zone/occlusion DSP was measured in focused PCM.

## Export

Normal exports: lab **9 assets / 1 scene / 56,326,684 B / .035 s**; range **58 assets /
9 scenes / 62,782,475 B / .171 s**. Both moved read-only to `/tmp`, launched from
unrelated `/tmp` cwd, used normal project startup/audio and closed their own windows
normally. No runtime FFmpeg, Node, TypeScript or repository fallback is required.
