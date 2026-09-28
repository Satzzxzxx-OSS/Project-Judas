# Sources and evidence scope

Primary sources consulted on 2026-09-27. No claim that a complete present Judas implementation was available.

1. Erin Catto, Continuous Collision, GDC 2013. Read full parsed slides/notes and inspect slides on TOI/substeps and conservative advancement.
   https://box2d.org/files/ErinCatto_ContinuousCollision_GDC2013.pdf
   Relevant: slides 5–8 (speculation versus TOI), 13–18 (conservative advancement and slow convergence), 39 (glancing rotation misses). This is a geometry/algorithm presentation, not a proof of arbitrary multi-contact TOI resolution.
2. Erin Catto, Solver2D, 2024. Read accumulated impulses, contact anchors, NGS/kinetic energy distinction and precision improvements.
   https://box2d.org/posts/2024/02/solver2d/
3. Pharr, Jakob, Humphreys, Physically Based Rendering, 4th ed., Managing Rounding Error. Read §§6.8.1–6.8.4 and transform error discussion.
   https://pbr-book.org/4ed/Shapes/Managing_Rounding_Error
4. Jonathan R. Shewchuk, Adaptive Precision Floating-Point Arithmetic and Fast Robust Predicates for Computational Geometry. Author overview consulted; no claim that its orientation/incircle routines directly implement our box/sphere oracle.
   https://www.cs.cmu.edu/~quake/robust.html
5. Box2D simulation reference, restitution, speculative contacts, contact lifetime, update stages.
   https://box2d.org/documentation/md_simulation.html
6. Box2D dynamic-tree API reference.
   https://box2d.org/documentation/group__tree.html
7. Bronnimann, Burnikel, Pion, Interval arithmetic yields efficient dynamic filters for computational geometry, 2001. Abstract/primary publication information consulted, not a reproduced algorithm.
   https://doi.org/10.1016/S0166-218X(00)00231-6
8. Halm and Posa, Set-Valued Rigid Body Dynamics for Simultaneous, Inelastic, Frictional Impacts, 2021. Abstract consulted for caution about impact ordering; no algorithm imported into the reference.
   https://arxiv.org/abs/2103.15714

User-provided sources:
- Adversarial audit at 48385a7: source-only findings, gamma8 scope concern, dynamic tree existence and known early-stop limitation.
- M32 attempt-2 report: exact reported gamma8 formula, first-contact/stability results, numerical operation summary and suggested velocity formula.
- Subsequent failed-correction report: nonzero restitution regression and rollback.
- FTFT3 checkpoint report: last user-reported main/origin/main 3404388f1af48419505c802fc11dbf8a0b577f38.

All new numeric results in this package are our independent reference computations. No published source is credited with having run these fixtures. No supplied report is credited as a current executable source snapshot.
