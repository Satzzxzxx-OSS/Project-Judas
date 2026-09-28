# Primary sources consulted, with scope

1. Erin Catto, Continuous Collision, GDC 2013.
   https://box2d.org/files/ErinCatto_ContinuousCollision_GDC2013.pdf
   Read full parsed slides/notes; rendered slides 8 and 13. Supports the
   speculative slowdown/restitution distinction, TOI plus remainder, angular
   trajectory consistency, and conservative-advancement convergence caveats.
   Its particular later fast algorithm openly misses angular glancing collisions;
   no universal CCD guarantee is inherited.

2. Erin Catto, Solver2D (2024), official Box2D article.
   https://box2d.org/posts/2024/02/solver2d/
   Read warm starting, accumulated impulse, position correction, precision and
   substep discussion. Supports caching/iteration distinctions, not our event model.

3. Evan Drumwright, True Rigidity: Interpenetration-free Multi-Body Simulation
   with Polytopic Contact (2016).
   https://arxiv.org/pdf/1608.02171
   Read sections on contact-event times, sustained contact and Algorithm 1.
   Highlights zero-distance CA failure and feature-change events. The paper
   excludes some nonunique-separating-plane contacts and does not cover all
   smooth shapes. Its integration order is NOT silently substituted for Judas.

4. Simbody SemiExplicitEulerTimeStepper documentation (3.6), official project.
   https://simbody.github.io/simbody-3.6-doxygen/api/classSimTK_1_1SemiExplicitEulerTimeStepper.html
   Documents Poisson/Newton distinctions, induced-impact choices, capture speed
   and event-limit compromises. In particular warns of energy gain with Newton
   restitution in multibody simultaneous contacts. Not a recommendation to import
   its unresolved-time or zero-restitution fallback without a new specification.

5. Yan-bin Jia, Matthew Mason, Michael Erdmann, Multiple Impacts: A State
   Transition Diagram Approach (2013), author-institution publication record.
   https://publications.ri.cmu.edu/multiple-impacts-a-state-transition-diagram-approach
   Read abstract/metadata: coupled impacts, contact transitions and relative
   stiffness dependence. Not the full algorithm specification.

6. Wang and Mason, Two Dimensional Rigid Body Collisions with Friction (1992),
   author-institution publication record.
   https://publications.ri.cmu.edu/two-dimensional-rigid-body-collisions-with-friction
   Abstract warns of energy anomalies in restitution/friction combinations.
   Not imported as our full friction solver.

7. Uchida, Sherman and Delp, Making a meaningful impact: modelling simultaneous
   frictional collisions in spatial multibody systems (2015/2016 record).
   https://pubmed.ncbi.nlm.nih.gov/27547093/
   Abstract and metadata checked: compression/expansion and interval solves;
   no claim to have implemented PLUS from this abstract.

8. Official Jolt Architecture documentation, continuous collision section.
   https://github.com/jrouwe/JoltPhysics/blob/master/Docs/Architecture.md
   Shows that an established engine can explicitly stop a cast body at its
   collision until the next step. Such a policy would not meet Judas's desired
   consume-the-remainder requirement; popularity is not proof of that guarantee.

All new derivations, exact witnesses and candidate models are identified as
our research. None of these sources validates the combined Judas proposal.
