# FTFT4A performance handoff

Start with **REVIEW.md**, then **ASTRA_FTFT4A_PERFORMANCE.md**.

This package contains the operator's verified current source export in `input/`, the reviewer’s isolated reference code in `work/code/`, generated namespace copies of the source arithmetic, and recorded local results in `work/results/`.

Run the component references:

```sh
python3 run_reference.py
```

Requires Python 3 and a C++17 compiler (tested GCC 14.2.0). No Python third-party package or GLM is required for these **scalar component tests**. The storage-only carriers are not an engine/GLM substitute. Do not add them to Judas. The actual application still requires its genuine dependencies and complete checkout.

The runner writes binaries only into `.build-reference/` and new logs into `verification-run/`. The archive contains no binaries. The original research result files remain unchanged.

Input manifest: `input/MANIFEST.json` (all 328 declared export files verified).
Package manifest: `SHA256.json` (excludes itself).

No current production engine was built by the reviewer; its dependencies were unavailable. Local microbenchmarks do not establish a new 1,500-crate runtime. The purpose is a concrete source-specific optimization plan with tested equivalence, followed by Astra's real-engine validation.

Exploratory rejected arithmetic trials and partial hardware-target build logs are preserved and clearly discussed in REVIEW.md. They are not selected implementation requirements.
