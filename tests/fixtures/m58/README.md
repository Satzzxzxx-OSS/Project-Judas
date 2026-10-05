# Independent M58 shaping references

Logical UTF-8 samples, exact font SHA256 and command arguments are in
`reference.json`. `reference.h` is their compact C++ representation. Generated
with the **unmodified upstream HarfBuzz 10.4.0 `util/hb-shape.cc`** from the pinned
archive, not Judas's shaping wrapper. OT font functions, scale 1536, ppem 24,
cluster level 0, paragraph BOT/EOT flags. Offsets/advances are 1/64 pixel units.

Build the upstream utility after the ordinary Judas build (GLib development headers
are required only for this reference utility, not the game/runtime). Compile
`hb-shape.cc` with includes for `build/_deps/judas_harfbuzz-src/{src,util}` and
`build/_deps/judas_harfbuzz-build/src`, link Judas's pinned `libharfbuzz.a` and GLib
from `pkg-config --cflags --libs glib-2.0 gobject-2.0`, plus pthread. Then run:

```sh
python3 scripts/m58_reference.py /path/to/hb-shape
```

The focused suite separately verifies UAX#9 visual order and wrapped lines against
ICU's direct visual-index API, CJK/grapheme/shape boundaries and actual GL glyph
positions against a CPU FreeType coverage oracle. These samples establish integration
correctness, not universal Unicode/font or linguistic certification.
