# Local hydrostatic column envelope

This change replaces literal interval union **within each resolved lateral column** with the finite local interval `[min lo, max hi]`. The existing positive weighted average across different columns, exact solid-footprint exclusion, frame construction, quadrature, and continuous bulk velocity/acceleration kernel are unchanged.

## Reproduction and result

`before.log` records 83 checks with one failure: the ordinary production pool's captured 752-particle release state supports a fully submerged 0.6 m box as only 0.850580633 occupied. The unchanged 80 earlier checks pass. The capture comes from `body-development/resume-neutral/neutral_020_release_particles.csv`, settled by the actual production path; it is not a synthetic solver answer.

The particle arrangement is disordered. Literal union interprets sample-space gaps as air, so even a body wholly inside the liquid loses support. The column envelope treats particles as samples of local connected bulk, consistent with the authorized approximate hydrostatic model.

`after.log` records the same 83 checks passing: occupancy is 1, buoyancy is 2117.28515625 N using measured local fluid acceleration −0.00775469234213 m/s². It is not forced to the analytical weight. `library-final.log` adds rotation/translation checks of that captured field, for 85/85 passing through the built engine library in about 0.22 s. Source fingerprints and exact build/run commands are in `library-final.json`. Before/final sources are preserved separately.

The new full-support gate is 0.99: for this reference neutral body's 2/s drag rate, an independent constant 1% gravity-support deficit produces approximately 0.172 m drift in four seconds, below the requested 0.20 m limit. The complete dynamic body and player tests remain separate acceptance evidence.

## Explicit approximation and limitations

This is a finite local hydrostatic envelope, not an exact liquid occupancy or mass reconstruction. An internal air pocket between particle layers inside the local window is filled. The final log demonstrates that limitation explicitly. Distant disconnected layers outside that window still report dry, as the unchanged earlier assertion checks. No claim is made that every separated blob or bubble is represented accurately.

The change does not alter particle mass, positions, velocities, PBF transport, bulk acceleration filtering, body mass, scene identity or any reaction ownership. No fraction clipping or output repair occurs.

Reproduce from repository root:

```
flock build/ftft9-build.lock cmake --build build --target judas_fluid_hydrostatics_tests -j2
build/judas_fluid_hydrostatics_tests
```
