# M60 pinned backend/thread boundary review

Pin: miniaudio 0.11.23 unchanged, existing `MA_NO_RESOURCE_MANAGER` build retained.
Judas uses `ma_sound_init_from_data_source`, **not** backend resource-manager
`ma_resource_manager_data_stream` ownership. Therefore normal voice destruction
cannot enter that backend's job-queue/uninit wait trap.

`PreparedAudioStream::Read`, Buffered::Read, Effect::Process, Output::Process and
Reverb::Process run on the mixer thread. Their Judas code reads immutable PCM,
fixed page state and atomics, copies/zeros samples, runs fixed filters/fades and
Verblib. It does not call the job system, decoder, filesystem, logging, physics,
world traversal, QuickJS or OpenGL. No per-block Judas vector/string allocation.

Page metadata/PCM is published with release ownership and acquired before reading;
only one worker owns a stream decoder. Seek generations invalidate ready pages;
read-side ownership prevents reclaiming an in-use page. Error text becomes visible
only after the failed atomic release. Decoder closure begins after graph detach
and prior ordered decode completion. The teardown fixture deliberately holds a
150 ms decode job while measuring main-thread voice removal.

Pinned miniaudio node graph attachment/uninitialization uses graph locks and
acknowledgment of current processing. `ma_sound_uninit`/`ma_node_uninit` on the main
thread can wait for the current mixer read. This is **not** strict lock-freedom,
and maximum observed main-thread detach is reported. That graph acknowledgment
does not wait for worker decoding. Decoder/page retirement runs on Judas jobs;
only process shutdown performs a blocking drain, before destroying the scheduler.

No JS completion callback was added; snapshots/errors/counters are main-thread
observations. Source positions/orientation/velocity, resident geometry queries and
zone selection are published once per outer frame, never by render-target cameras
or fixed-step multiplicity. Audio fades/playback use sample time.

Tests capture the real production miniaudio graph. They establish mechanics and
bounded lifetime; they do not establish subjective sound quality or strict
real-time guarantees on a general-purpose OS.

A final pre-gate source review corrected two narrow one-shot control details:
independent shots participate in obstruction demand even if their persistent
emitter is stopped, and reuse the emitter's sampled velocity rather than taking a
second finite difference after the current position has already been stored.
Both changes are generic audio control; no physics/gameplay semantics changed.

The clean-build gate began while final source review finished; the narrow one-shot
control review was completed before `WorldAudio.cpp` compiled. Final geometry PCM
and rotated-origin tests were added before those focused targets compiled. No
production test results from an earlier source state are relabelled as final.

Final bounded-prefill audit: pinned `ma_decoder_get_length_in_pcm_frames` warns
that MP3 can scan/decode the whole file. Streaming now selects the supported file
codec explicitly, queries only constant/header WAV/FLAC lengths and learns MP3
length at natural decoded EOF. Seek tables remain disabled. The existing worker
handles compressed seeks, whose latency remains codec/content-dependent. A fresh
MP3 fixture without Xing metadata verifies unknown-duration readiness and learned
EOF length. Buffered whole-clip decoding intentionally keeps its old length query.
