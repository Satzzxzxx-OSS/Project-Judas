# Measurement environment

The 3.3-core SDL driver probe, using the same window/context attributes as Judas,
reported Mesa llvmpipe (LLVM 21.1.8, 256 bits), Mesa 26.0.8 for offscreen rendering.
The EGL hardware-device warning did **not** mean this context used NVIDIA: see
`requested-driver.log` and `mesa-driver.log`. Native GLX is NVIDIA GTX 970,
4.6 core, driver 580.178.04. No other `judas`/`judas_editor` processes were present
when checked; unrelated processes were not terminated. Native performance records
include system load averages. The first native baseline briefly overlapped a small
required-save-state assertion process; the final corrected timings are collected
sequentially after the clean build/production run finishes.

M56 retains 120 frames and keeps startup separately. Initial performance files
without `-final` preserve the missing-window measurement mistake and must not be
used for intact/first-fracture timing. Corrected cases use explicit capture windows.
Assertion hosts force one 1/60-second simulation step per frame, disable vsync,
and sleep 1 ms for resource publication; their frame costs are throughput/sanity
measurements, not measured interactive monitor FPS. The unmodified shipping
executable is checked separately on the native driver with its ordinary cadence.
CPU-only soft measurements explicitly disable self contact, use 4 substeps and
4 iterations, and report known numeric retained storage as a lower bound, plus
measured allocation count/bytes. The rigid application comparison has actual
contacts, joints, main/M33/shadow render submission, and 90 material bodies.
