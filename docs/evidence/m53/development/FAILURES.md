# M53 development failures and corrections

These are observed development failures, not fabricated negative fixtures.

- The initial `.judasnav` decoder read an eight-byte magic while the writer emitted
  nine bytes (`JudasNav1`). Asset round-trip/tracking failed; the test then dereferenced
  an unbuilt world and crashed. Corrected header length and added a fixture early exit.
- The first authored prefab mapping omitted the existing encoded-property count.
  Normal scene loading reported `invalid prefab property count`. Corrected content
  to use the existing mapping grammar, including the empty override count.
- Obstacle/link fixtures initially used authored IDs for runtime `CreateEntity`.
  The existing identity guard correctly rejected them. Corrected fixtures to allocate
  normal runtime identities and use ordinary registered surface assets.
- The navigation cookbook fixture retained linked prefab provenance; resolving the
  prefab restored its original enemy script instead of the example. Detached that
  copied test fixture's provenance before replacing scripts. No production prefab
  semantics were changed.

- The disconnected-route fixture reused a modifier whose area ID belonged to a
  different fixture registry. Bake correctly rejected the unregistered area; the
  unchecked fixture then queried an empty service. Corrected the fixture to use
  its registered Default area.

- Review of the actual link corridor found that the navigation route crossed
  a continuous physical perimeter wall. A route alone did not prove motor traversal.
  Opened an ordinary authored gap in that wall, preserving CharacterMotor collision
  and script-controlled hover traversal; rebaked the affected source assets.

Corrected focused checks and the ordinary project navigation/cookbook checks pass.
