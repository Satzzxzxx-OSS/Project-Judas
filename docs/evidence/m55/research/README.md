# Supplied M55 brief/research

Original operator-supplied files, preserved unchanged. Research status predates
accepted M54 and is superseded by checkpoint d9ebc8c7a987047b1d4175ed5d5de7da8dea472d.
The Python results are mathematical checks, not Judas implementation validation.

Current algorithm references consulted narrowly:
- USACE: finite-volume/subgrid storage and face geometry. https://www.hec.usace.army.mil/confluence/rasdocs/ras1dtechref/6.5/theoretical-basis-for-one-dimensional-and-two-dimensional-hydrodynamic-calculations/2d-unsteady-flow-hydrodynamics/numerical-methods
- Tavelli & Zanotti 2026 preprint: implicit pressure/continuity on curved surfaces. https://arxiv.org/html/2605.25544v1
- PBRT transmittance/path length. https://pbr-book.org/4ed/Volume_Scattering/Transmittance

Our discretization is a bounded low-order local-inertia approximation. It does
not inherit a publication's numerical guarantees. See the actual core record.
