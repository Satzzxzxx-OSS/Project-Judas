# Actual contained-fluid impulse and ownership witnesses

`judas_fluid_cavity_impact_tests` builds an ordinary scene with a declared shape-local cavity and advances `StepPlayedWorld`. The public fluid API supplies the initial particle velocity because authored fluid-volume components have no velocity field. No alternative collision/integration path supplies answers.

`before.log/json` preserves the original reduced-mass defect: a 100 kg particle at 1 m/s striking a 1 kg resting wall leaves energy 50 -> 5000 J while momentum remains 100 kg m/s. Physical exit 1, with exact source snapshots.

`six-case-after` and `after-symmetric` retain six passing independent inelastic-collision checks: wall masses 1/80/1000 kg, each identity and rotated/translated. Expected common velocity is initial total momentum / total mass. Expected final energy follows from that velocity. No expectation calls the contact implementation.

`boost-before` additionally records the same 80 kg-wall collision with a common +3 m/s velocity. It exposes mismatched cavity pose timestamps: momentum 640 -> 598.741455 kg m/s and wrong wall velocity, despite the six unboosted cases still passing. The particle receives contained impulse and then contributes an exterior hydrostatic force because reaction and exclusion use different poses. Physical exit 1 is preserved; it is not acceptance.

Reproduce: `cmake --build build --target judas_fluid_cavity_impact_tests -j2 && build/judas_fluid_cavity_impact_tests`.

`boost-after.log/json` records the pose-timestamp repair: all seven cases pass; the boosted case has P=640 -> 640 kg m/s, common velocity 3.555555582 m/s, and final KE 1137.777771 J (independent oracle 1137.777778 J). The repair uses the particle field's latest collider endpoint for the cavity exclusion mask and its start pose for initial membership. It does not alter the physical fixture or tolerance.
