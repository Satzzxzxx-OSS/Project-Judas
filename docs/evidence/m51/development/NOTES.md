# Genuine development findings

- The initial hand-authored new box omitted required scene serialization fields;
  startup rejected `render.radius`. Added the ordinary serialized fields; no
  parser relaxation or assertion widening. The corrected startup is recorded.
- First migration-test compilation used one `auto` declaration for BodyHandle
  and vec3; split the declarations. Its build failure log is preserved.
- Support pin integration initially treated RuntimeCharacter's return as a
  wrapper, rather than CharacterMotor itself. Corrected member access; the
  follow-up compiler failure log is preserved.
- The physical-control cookbook example initially lacked a JSDoc constructor
  type; standard TypeScript strict checking reported TS7031. Added the Entity
  parameter annotation, not a runtime type dependency.
- The temporary M50 TypeScript directory was absent. Downloaded the same pinned
  TypeScript 5.9.3 archive into /tmp; SHA256
  `10e108c9cf7d5f2879053dff18515fb405abf2ccef63eaaf017d9c571687a1d3`.
  This remains developer-only tooling and is not packaged with games.
- The first focused composition run passed 26 checks. The follow-up strengthened
  the hinge check from handle existence to actual joint coordinate movement;
  all 26 checks still passed. These are ordinary focused development iterations,
  not repeated production-gate runs.

- Final review found editor StartPlay still captured the pointer unconditionally,
  before the normal frame reconciliation, and world reset cleared capture intent
  before destroy callbacks. The narrow cleanup respects compatibility mode and
  clears intent after callbacks. Only affected migration/editor/transition/export
  checks were rerun; the production suite was not repeated.
- Final package relocation initially used rename across different filesystem
  mounts and correctly reported EXDEV. Used normal shutil.move; export/runtime
  implementation was unchanged.
