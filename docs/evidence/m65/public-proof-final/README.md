# Public JS follow-up

Six checks execute through ordinary `judas` imports and the application harness:
two independently targeted feet on an obliquely rotated rider, motor sensor callback,
destruction during that callback, stale wrapper validity, and exactly one exit.
No native dispatch or private script state injection is used.

The first preserved run (`../public-proof/`) used a non-unit axis with a project
quaternion helper that requires a unit axis. It scaled expected world targets while
the runtime correctly normalized the entity rotation. The second preserved run
(`../public-proof-follow/`) incorrectly expected `world.entity(id)` to be a verified
nullable lookup. The current documented API constructs a wrapper; `.valid` is the
correct check. Both fixture corrections are in `probe.js`; no engine correction
was made for these failures. `run.log` contains six PASS lines and no script faults.
