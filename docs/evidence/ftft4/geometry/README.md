# Independent represented-input contact checks

`../geometry_oracle.py --generate` writes this fixture set. The supplied
`handoff/reference/code/geometry_reference.py` supplies exact `Fraction` predicates.
It is unchanged. All new fixture centres, quaternions, radii, dimensions and
compound-child offsets are first rounded to binary32, then recovered as exact
rational inputs. Rotations mean `R(q)/dot(q,q)` in both the declared production
contract and this independent oracle.

The actual engine adapter is `tests/ContactGeometryProbe.cpp`. Build its target,
then run from the repository root:

```
python3 docs/evidence/ftft4/geometry_oracle.py \
  --binary build/judas_contact_geometry_tests \
  --output docs/evidence/ftft4/geometry-final
build/judas_contact_geometry_tests --player
```

The executable consumes generated expected classifications. It calls the actual
`ComputeContacts(PrimitivePose, PrimitivePose)` path, `ShapeAabb`,
`PhysicsWorld::FindCollidingPairs`, and `QueryBodiesInAabb`. It reports actual
stored inputs, not intended pre-rounding geometry. No contact code is reproduced
in the adapter. Its default invocation reads `fixtures.txt` and prints JSONL.
Python additionally verifies:

- exact closed overlap and candidate-margin classification;
- genuine positive gaps stay positive;
- per-point separation state agrees with its actual signed gap;
- returned parent-local surface witnesses lie on the named primitive features;
- both parent-local physical anchors describe the witnesses' common midpoint;
- witness separation projected onto the normal equals reported separation;
- sphere normals follow the independently computed closest feature;
- box normals correspond to an actual SAT axis and point B toward A;
- outward binary32 AABBs contain exact rational geometry, with no tolerance;
- independently expected world pairs and conservative query candidates.

A box pair's SAT classification is not every manifold point's classification:
a penetrating/touching pair can have separated clipped candidate points within
the requested margin. Point geometry is checked through its own witnesses.
SAT projected separation is not asserted to equal exterior Euclidean distance.

The fixed pre-execution surface/anchor error allowance is
`4096 * binary64_epsilon * max(shape_size, pair-centre-difference)`.
Unit-normal/axis checks use `4096 * binary64_epsilon`. These are declared
engineering allowances for this represented-input fixture family, not proofs for
all binary32 shapes. They do not grow with absolute world translation. Positive
sign classification and AABB containment have no error allowance. The ordinary
player sweep still uses its pre-existing binary32 sampled query; its separate
analytical checks report that path's wider coordinate-arithmetic allowance.

Coverage comprises scales 0.001, 1 and 1000 metres; translations 0, 10, 137 and
1000 metres; identity/oblique rotations; all signed axis feature arrangements;
sphere faces/edges/corners; exact touching and either separation sign;
nonparallel and nearly parallel boxes; actual compound openings and offset
children; and equivalent quaternion signs/scales. Large-coordinate diagnostics
are labelled `representation_limit`. `lost_input_gap` deliberately stores an
intended 0.001-metre gap at 1e6 as exact touching: input loss is not attributed to
the collision evaluator. Known-positive represented gamma witnesses at 137 and
1024 remain separate acceptance cases.

`--player` executes 18 independently derived signed-axis face sweeps and three
compound-opening/rail sweeps through the actual player API. Axis cases also
remove a proxy, reuse its slot for a distant object, and insert the intended hit.
The oracle is elementary capsule/plane geometry (1.25-metre travel) or a rail
face (0.75-metre travel), independent of the engine's capsule-distance routine.

The four unmodified Python research programs and scalar C++ gamma check are
separate supporting evidence under `../independent-reference/`. Their original
exact rational transformed inputs are not silently treated as engine binary32
inputs, and their PASS is not engine acceptance.

`--invalid` separately exercises zero/NaN/Inf quaternion inputs in either body.
The required result is an explicitly uncertain empty manifold and invalid-input
telemetry; it does not introduce new public body-construction validation. Two
finite extreme-quaternion diagnostics have analytically overlapping concentric
boxes. They report either a correct resolved hit or explicit arithmetic-range
uncertainty. An uncertain result is a recorded range limitation, **not** a passed
physical overlap claim and not part of the supported 823-case acceptance count.
Dense noninteger same-orientation cases exercise actual exact fallback. Six
rotated single-child compound cases use identical parent poses, with local child
offset exactly 2 or its adjacent binary32 values, at translations 0 and 137.
Those congruent parallel faces have a uniform gap, so every point is required to
retain the exact touching/negative/positive sign, independently of generic tilted
manifold behavior.
