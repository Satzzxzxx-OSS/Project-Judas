# Explicit coarse-box endpoint exclusion

The existing sequential solid projection can leave a coarse particle inside a second expanded solid when there is no room between margins. The preserved actual dense-body snapshot motivating this repair had three centers below its floor (minimum y approximately -0.074725 m); this focused test does not replace that integrated oracle. Prior collision snippets and boundary source are retained unchanged.

## Declared approximation

After the existing two projection passes, an endpoint is tested against every prepared box, sphere and terrain. If it is unambiguously inside any expanded solid, only box primitives actually encountered by this particle supply escape geometry. Their six expanded face planes form candidate sets of one, two or three planes. The closest point to the input pre-projection position on each independent set is computed in double precision by its Gram system. Rank-deficient combinations are skipped at dimensionless 64-double-epsilon determinant tolerance. Every representable candidate is checked against ALL prepared solids; minimum squared displacement selects the feasible point by deterministic enumeration.

GeometryRoundoff is 16-float-epsilon times max(1, |point|+|solid center|+bound extent). Plane offsets receive twice this numerical margin for conservative representability. This is a numerical geometric margin, not a body/mass/scene classifier or physical force. Final active planes are recognized within four geometry-roundoff budgets. A missing feasible candidate increments unresolvedGeometryCount and is not silently declared solved.

The displacement is added only to the existing solidCorrections term, so geometric push-out is excluded from reconstructed velocity. Particle identity, count and mass are preserved. This endpoint exclusion can displace a coarse sample substantially and does NOT claim conservative continuum remapping, exterior momentum conservation or a physical swept-fluid path. Spheres and terrain validate/reject candidates but do not supply escape candidates in this first bounded policy.

Box contacts retain the exact substep-stable PreparedBox pointer and face axis. After successful escape, old rows are removed only when the final point is provably clear of that primitive or the old finite expanded face patch. All actual endpoint-active box planes are reconstructed from the same geometry and retain their normal/owner/point velocity. A side face can therefore replace an obsolete bottom face without leaving the contradictory bottom velocity row. No cavity/owner/mass rule changes here.

Diagnostics expose geometryEscapeCount, unresolvedGeometryCount, geometryEscapeCandidates and maximumGeometryEscapeDistance for the last executed step. Candidate combinatorics depend on encountered primitives rather than all world faces; all validation costs occur inside the timed ordinary fluid step.

## First focused execution (attempt 1)

12 cases / 60 checks pass. All prior 8 cases / 24 assertions are unchanged. Four new fixtures squeeze one particle between a floor and moving box, using identity/arbitrary rotation and both collider orders. All select the same feasible lateral escape, preserve the final side/floor normal rows, remove the obsolete bottom row, preserve mass/count, and complete the actual auxiliary normal solve without a cap. Each escape validates 124 candidates and reports zero unresolved geometry. Maximum measured escape displacement is 0.442317 m; this is visible approximation cost, not an inferred physical travel distance. Identity fixtures have zero final KE; rotated fixtures have 3.48e-13 J.

The first build had a missing <algorithm> test include. BUILD_FAILURE.log preserves it. STALE_BINARY_NOT_ACCEPTANCE.log is the old eight-case binary accidentally executed after that compile failure and is explicitly excluded from new evidence. The corrected build and after.log provide acceptance of these focused checks only. No second numerical algorithm attempt or broad regression campaign was performed here.

## Static-wall side preservation (attempt 2, final)

A nearest exterior endpoint alone can choose the opposite face of a thin wall. The final policy therefore adds a geometric path barrier for encountered boxes whose start/end poses are identical (including q/-q equivalent rotations). When the original substep start was outside a box, the straight segment from that start to a candidate must not cross its resolved UNEXPANDED interior. Exact slab intervals are used in local coordinates; only the declared float geometry-roundoff margin removes ambiguous tangent/interior distinctions. An already-inside start has no invented side guarantee, and genuinely moving geometry is not treated as a stationary barrier. Spheres and terrain do not gain a path barrier in this bounded extension.

The same escape enumeration now triggers when the existing sequential endpoint already crossed such a stationary interior, even if it emerged outside every expanded solid. This prevents a thin floor's bottom face from silently becoming an accepted endpoint. Candidate work and unresolved cases remain exposed; no particle mass, body/cavity classification or impulse law changes.

The final focused execution is 14 cases / 78 checks, all passing. The additional identity/arbitrarily rotated thin-floor fixtures select the valid top-side escape and retain side/floor velocity response; all previous assertions remain unchanged. attempt1.log and attempt1-source preserve the passing first policy. The final after.log, final-source and METADATA.json identify the tested second policy. No further numerical algorithms or wider regression runs were executed here.
