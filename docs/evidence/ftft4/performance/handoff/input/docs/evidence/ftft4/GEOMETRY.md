# FTFT4A pair-local geometry mechanism

This note precedes the implementation. The solver's separated-impact timing is unchanged.

## Represented input and coordinates

Stored binary32 positions, dimensions, child offsets and quaternion components are inputs, not estimates of pre-quantized author intent. Promote each before subtraction/composition. A nonzero quaternion q=(w,x,y,z) denotes the proper rotation R=N/d, d=q·q; N is the usual homogeneous quadratic quaternion matrix. A zero/non-finite quaternion is invalid; report uncertainty rather than inventing identity. No public state or origin representation changes.

Primitive geometry keeps its parent position/orientation and parent-local child center. The narrowphase works relative to parent A's position. Compose rotated child offsets in binary64 in that frame. Returned world float contact points are presentation only. Parent-local double anchors describe one common impulse application midpoint. Separate parent-local witness points describe the two selected surface features. The normal points from B toward A. Signed separation is positive for separated geometry, zero for exact touch, negative for penetration. Box separation is a SAT-axis quantity, not Euclidean distance.

## Sign filter and exact fallback

Sign-critical calculations use outward binary64 intervals for the operations actually evaluated (addition, subtraction, multiplication, division with a strictly positive denominator, absolute value, maximum, and square root). An interval strictly on one side of zero resolves that sign. A zero-containing interval triggers an exact expansion polynomial predicate. It never itself establishes touching.

Exact expansion arithmetic uses error-free TwoSum and FMA TwoProduct to retain every binary64 residual; polynomial signs are obtained from the leading nonzero expansion component. Overflow/underflow which invalidates the error-free-transform assumptions is detected and reported unresolved. Fallback range failures are not known zero. This is an arithmetic mechanism with a stated range limitation, not a universal proof inferred from finite tests.

The exact predicates avoid normalization square roots and rational division. Let dA,dB be positive quaternion norms, NA,NB their rotation numerators, oA,oB child offsets and pA,pB stored parent positions. Define

    D = (pA-pB)*dA*dB + NA*oA*dB - NB*oB*dA
    K = dA*dB

so the center difference is D/K. For any SAT direction t (a column of NA or NB, or the cross product of one column from each), use

    s = |D·t| - dB*sum(hA[k]*|NA[k]·t|)
                - dA*sum(hB[k]*|NB[k]·t|).

All nonzero axes participate, including nearly parallel edge axes. The sign of s is the separation sign; exact zero axes alone are skipped. To compare a positive s against a nonnegative candidate margin m, compare s² with m²*K²*(t·t). This separates contact truth from broadphase/proximity policy. A face-manifold preference may choose among valid nonseparating axes but cannot overwrite a positive separating edge axis.

Sphere/sphere compares |D|²-(rA+rB)²*K². For sphere/box, use box-local numerators l[k]=NB[k]·D with common positive denominator H=K*dB; outside components are max(|l[k]|-hB[k]*H,0). Compare the sum of their squares with r²*H². The same expression with r+margin controls proximity. Exact predicates provide sign and, on fallback, a higher precision expansion estimate for cancellation-prone differences. Distances use rationalized differences where possible: (distance²-radius²)/(distance+radius), preserving a resolved small positive gap.

## Manifold and anchors

Face clipping is Sutherland–Hodgman in the reference box frame using binary64 coordinates. The filtered/exact predicates certify the pair's SAT sign and candidate reach; they do not make every constructed clip coordinate an exact rational value. Independent surface/midpoint/projection residual checks cover those numerical constructions. If a resolved positive SAT axis produces a nonpositive constructed feature gap, the path reports explicit unresolved arithmetic; it does not collapse the known positive geometry to touching. Its vertices are incident-face features; their projections onto the reference plane are reference features. Retain the existing deterministic four-point reduction in local coordinates. Edge contacts use closest points on the selected finite support edges. Parent-local application anchors are constructed from the precise common midpoint, without a float world-point round trip. The solver can continue measuring subsequent anchor displacement relative to this initially coincident pair.

## Bounds

Shape AABBs enclose the same R=N/d geometry. Compute interval center/extents, use their outward endpoints, then round conversion to binary32 outward. Compound bounds union actual child boxes, never a convex collision hull. Broadphase fattening stays a candidate policy; it does not change signed separation.

## Interface and diagnostics

Contact gains signedSeparation, preciseNormal, localAnchorA/B, localWitnessA/B, hasLocalAnchors and SeparationState. ContactManifold reports unresolved geometry explicitly. PrimitivePose retains legacy float pose for the existing approximate terrain/player interfaces and additionally retains the parent pose/child offset consumed by the new ComputeContacts(PrimitivePose,PrimitivePose) overload. Primitive exact oracles do not certify terrain's specialized approximation.

Counters record tested sign predicates, interval-resolved predicates, exact fallbacks, unresolved arithmetic and invalid input. Acceptance must report fallback frequency and cost alongside same-machine crate timings. No uncertain classification, debug point, or cache radius supplies physical zero separation.

## Implementation details and exercised fallback

Directed interval endpoints use exact adjacent IEEE binary64 bit patterns instead of a repeated libm call. An independent comparison against `std::nextafter` checks 2,000,026 results, including signed zero, subnormal/normal boundaries, infinities and seeded random bit patterns. This changes only the cost of outward rounding.

The convenience `SphereVsSphere` and `SphereVsBox` APIs lack sphere orientation arguments. They therefore explicitly mark the relevant precise offsets as world-oriented; the solver converts them once into each actual body's parent-local frame. `PrimitiveContacts` already receives both actual orientations and produces parent-local anchors directly. This distinction prevents a rotated sphere's orientation from incorrectly rotating an already world-oriented lever arm.

## Remaining arithmetic and performance limits

The expanded independent fixture run exercised 64 exact fallbacks as well as exact touching, with no unresolved ordinary geometry. Invalid zero/non-finite quaternion cases are explicitly reported. An extreme `FLT_MAX`-scaled nonzero quaternion can overflow the homogeneous predicate range and remains an explicit unresolved diagnostic even when an independent rational oracle establishes overlap. That is a supported-input range limitation, not a known zero or an accepted no-contact result. Normalizing stored public state or adding a new arbitrary-precision dependency was not part of this repair.

The supporting `development/profile_fixture.cpp` repeats the same 1,500-crate/30-step setup through the actual physics implementation five times. It omits the expensive exhaustive acceptance oracle solely to isolate CPU cost; it is not an acceptance test or substitute timing result. Its instrumented flat profile attributes approximately 26.5% to interval multiplication, 5.8% to interval SAT, 9.4% to box bounds and 11.3% to interval addition. Compiler-folded leaf symbols in gprof must not be interpreted as exact-fallback counts; use the actual runtime counters. Repeated uninstrumented full-fixture results remain authoritative for the measured regression. The arithmetic adjacency optimization reduced cost but did not remove the substantial interval-geometry overhead.

The extra `development/rigid_uncertainty.cpp` adapter runs the unchanged complete rigid-contact suite and then requires zero unresolved/invalid geometry counters. It passed with 2,230,499 sign predicates, including 2,082 actual exact fallbacks, and zero unresolved/invalid cases. It supplements the normal production-suite execution; it does not replace or alter it.

## Executed range and API limits

The accepted represented-input family and the additional unchanged rigid suite
have zero unresolved cases. Separate range diagnostics deliberately retain a
known-overlap case with all quaternion components equal to FLT_MAX which exhausts
the homogeneous arithmetic and reports uncertainty. FLT_MIN components resolve.
These are numerical-range observations, not an assertion that arbitrary finite
quaternion magnitudes are fully supported. Scene construction normalizes authored
orientations; the unchanged raw rigid inertia API assumes normalized orientation.
Quaternion sign/scale query equivalence does not certify non-unit raw dynamics.
