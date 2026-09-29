# FTFT4 persistent-support fixture oracle correction

The first phase correctly checks inelastic response when an incoming impact is incident to validated cached support at a positive gap. No physical fixture, tolerance, or production code was changed.

The second phase originally assumed the two spheres still touched after drift and demanded inelastic floor capture. The actual represented centres were x=-0.75 and x=0.25000002980232239: their signed gap is **2.9802322387695312e-08 m**, and the real zero-margin collision query reports no touching pairs. Consequently each subsequent sphere/floor impact is isolated and must preserve authored restitution 1.

From the actual initial floor gap 0.010999977588653564 and represented timestep 0.019999999552965164, mechanics gives final y=0.5090000219643116, vy=1. Observed y=0.50900006294250488, vy=1. The 4.10e-8 m discrepancy is within the existing 2e-6 m gate. The velocity gate stays 2e-5.

The corrected test asserts the positive-gap/no-contact prerequisites explicitly, without an outcome-dependent expectation branch. It retains all physical inputs, tests the analytical isolated-impact result, and still verifies horizontal remaining-time travel. Earlier raw failures remain in `../correctness5/impact-timing.log`. `ImpactTimingTests.before.cpp` preserves the exact pre-correction test.

The observational source links the current production static engine, performs ordinary world steps, and prints represented initial/final state plus the independent one-dimensional oracle. It does not change production mechanisms.
