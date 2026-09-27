#!/usr/bin/env python3
"""Generate ordinary FTFT3 scenes and an independent 80-digit gravity oracle.

Only Python's standard library is required. Run from any directory:
    python3 docs/evidence/ftft3/generate_fixtures.py

The manifest is a specification, generated before observing runtime results.
It uses explicit IEEE binary32 packing for authored scalar/quaternion inputs,
then Decimal(80) scalar rotation-matrix arithmetic divided by the quaternion's
squared norm. It never calls Judas, GLM, a gravity factory, or a sampled field.
The intended column uses the original binary64 axis-angle quaternion and ideal
specified magnitude, separately exposing input representation error.

Reference tolerance recipe (consumed by the C++ fixture runner): gamma_64 for
binary32 unit roundoff times each component's operation-magnitude scale, plus
only a tiny underflow allowance. The scales retain transverse tiny-angle
sensitivity; a whole-vector tolerance proportional to |g| would hide FTFT3.
For N fixed steps starting at rest, multiply acceleration tolerance by elapsed
represented time, and add an accumulation allowance gamma_(N+4)*|a|*time.
These are declared arithmetic allowances, not fitted to runtime observations.
No decimal-to-binary rounding correction is applied to observed results.
"""

from __future__ import annotations

import csv
from decimal import Decimal, localcontext
import math
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parent
FIXTURES = ROOT / "fixtures"


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def adjacent_f32(value: float, steps: int) -> float:
    """Adjacent positive finite binary32 value, without binary64 nextafter."""
    bits = struct.unpack("<I", struct.pack("<f", value))[0]
    return struct.unpack("<f", struct.pack("<I", bits + steps))[0]


def text32(value: float) -> str:
    text = format(value, ".9g")
    assert struct.pack("<f", float(text)) == struct.pack("<f", value)
    return text


def axis_angle(axis: tuple[float, float, float], angle: float) -> tuple[float, ...]:
    length = math.sqrt(sum(x * x for x in axis))
    s = math.sin(angle / 2.0)
    return (math.cos(angle / 2.0), *(s * x / length for x in axis))


def oracle(quaternion: tuple[float, ...], magnitude: Decimal):
    """Explicit second column of a scalar rotation matrix, with norm² divisor."""
    with localcontext() as context:
        context.prec = 80
        w, x, y, z = (Decimal.from_float(x) for x in quaternion)
        norm2 = w*w + x*x + y*y + z*z
        assert norm2 > 0
        vector = (
            magnitude * 2 * (w*z - x*y) / norm2,
            magnitude * (x*x + z*z - w*w - y*y) / norm2,
            -magnitude * 2 * (y*z + w*x) / norm2,
        )
        # y uses the equivalent -1 + 2(x²+z²)/norm² form's operation scale.
        scales = (
            abs(magnitude) * 2 * (abs(w*z) + abs(x*y)) / norm2,
            abs(magnitude) * (1 + 2 * (x*x + z*z) / norm2),
            abs(magnitude) * 2 * (abs(y*z) + abs(w*x)) / norm2,
        )
        return vector, scales


def cases():
    identity = (1.0, 0.0, 0.0, 0.0)
    default = f32(9.81)
    below = adjacent_f32(default, -1)
    above = adjacent_f32(default, 1)
    result = []

    def add(name, quaternion, intended_magnitude):
        result.append((name, quaternion, Decimal(intended_magnitude)))

    add("identity_default", identity, "9.81")
    add("identity_previous", identity, Decimal.from_float(below))
    add("identity_next", identity, Decimal.from_float(above))
    for label, magnitude in (("zero", "0"), ("quarter", "0.25"),
                             ("3_7", "3.7"), ("17_25", "17.25")):
        add("identity_" + label, identity, magnitude)

    # Straddle the old |down.x/z| < 1e-6 selection tolerance on both signs.
    for axis_name, axis in (("z", (0.0, 0.0, 1.0)), ("x", (1.0, 0.0, 0.0))):
        for label, radians in (("half", 0.5e-6), ("below", 0.999e-6),
                               ("above", 1.001e-6), ("double", 2.0e-6)):
            for sign_label, sign in (("positive", 1), ("negative", -1)):
                add(f"{axis_name}_{sign_label}_{label}_default",
                    axis_angle(axis, sign * radians), "9.81")
    witness_q = axis_angle((0.0, 0.0, 1.0), 0.5e-6)
    add("z_positive_half_previous", witness_q, Decimal.from_float(below))
    add("z_positive_half_next", witness_q, Decimal.from_float(above))

    for name, axis, radians in (
        ("axis_x_quarter", (1.0, 0.0, 0.0), math.pi / 2),
        ("axis_z_quarter", (0.0, 0.0, 1.0), math.pi / 2),
        ("axis_x_reverse", (1.0, 0.0, 0.0), math.pi),
        ("axis_z_reverse", (0.0, 0.0, 1.0), math.pi),
        ("axis_y_yaw", (0.0, 1.0, 0.0), math.pi / 2),
    ):
        add(name, axis_angle(axis, radians), "9.81")

    arbitrary_q = axis_angle((1.0, 2.0, -3.0), 0.73)
    add("arbitrary_default", arbitrary_q, "9.81")
    add("arbitrary_3_7", arbitrary_q, "3.7")
    add("arbitrary_zero", arbitrary_q, "0")
    add("arbitrary_nonunit", tuple(3.125 * x for x in arbitrary_q), "9.81")
    assert len(result) == 34
    assert len({name for name, _, _ in result}) == len(result)
    return result


def scene_text(identifier, origin, quaternion, magnitude):
    return f'''# Reproducible ordinary authored scene; see ../generate_fixtures.py.
JudasScene 3
settings
  name "FTFT3 {identifier}"
  world-origin {" ".join(str(x) for x in origin)}
  sun-direction 0.4 0.7 0.35
  sun-color 1 0.98 0.92
  ambient 0.16 0.17 0.19
  fluid-scale 1
  fidelity-policy none
  next-id 4
end

object 1 "Uniform field"
  position 0 0 0
  rotation {" ".join(text32(x) for x in quaternion)}
  scale 1 1 1
  gravity uniform {text32(magnitude)}
  gravity.region sphere 512
end

object 2 "Free body"
  position 3 4 5
  rotation 1 0 0 0
  scale 1 1 1
  render box
  render.half-extents 0.1 0.1 0.1
  render.radius 0.1
  render.color 0.85 0.35 0.2
  render.alpha 1
  render.secondary-color 0.8 0.8 0.8
  render.secondary-alpha 1
  render.mesh-asset ""
  render.texture-asset ""
  body dynamic box
  body.half-extents 0.1 0.1 0.1
  body.radius 0.1
  body.terrain ""
  body.mass 37
  body.friction 0.6
  body.restitution 0.1
  body.initial-velocity 0 0 0
  body.pickable false
  body.managed false
  body.compound-count 0
end

object 3 "Player start"
  position 2000 2000 2000
  rotation 1 0 0 0
  scale 1 1 1
  player-start third-person
  player-start.yaw 0
end
'''


def main():
    FIXTURES.mkdir(parents=True, exist_ok=True)
    columns = ["id", "scene", "origin_group", "sign", "placement", "magnitude",
               "w", "x", "y", "z", "expected_x", "expected_y", "expected_z",
               "scale_x", "scale_y", "scale_z", "intended_x", "intended_y", "intended_z"]
    rows = []
    for base, raw_q, ideal_g in cases():
        represented_g = f32(float(ideal_g))
        intended, _ = oracle(raw_q, ideal_g)
        for sign_label, sign in (("q", 1), ("minus_q", -1)):
            represented_q = tuple(f32(sign * x) for x in raw_q)
            expected, scales = oracle(represented_q, Decimal.from_float(represented_g))
            for placement, origin in (("near", (0, 0, 0)),
                                      ("far", (1000000000, -2000000000, 3000000000))):
                identifier = f"{base}_{sign_label}_{placement}"
                filename = identifier + ".judas"
                (FIXTURES / filename).write_text(
                    scene_text(identifier, origin, represented_q, represented_g), encoding="utf-8")
                rows.append([identifier, filename, base, str(sign), placement,
                             text32(represented_g), *(text32(x) for x in represented_q),
                             *(str(x) for x in expected), *(str(x) for x in scales),
                             *(str(x) for x in intended)])
    assert len(rows) == 136
    with (FIXTURES / "manifest.tsv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(columns)
        writer.writerows(rows)
    print(f"Generated {len(rows)} scenes: 34 base settings x 2 quaternion signs x 2 fixed origins.")
    print("Independent reference: Decimal precision 80; no runtime code or sampled vectors.")
    print(f"Manifest: {FIXTURES / 'manifest.tsv'}")


if __name__ == "__main__":
    main()
