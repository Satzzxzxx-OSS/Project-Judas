# Primary sources inspected for this research

[1] Batty, Bertails, Bridson, **A Fast Variational Framework for Accurate Solid-Fluid
Coupling**, 2007. Sections 2 and 3; kinetic energy, pressure projection, body force/
torque mapping and limits for thin shells. PDF equations/figures visually inspected.
https://cs.uwaterloo.ca/~c2batty/papers/Batty07.pdf

[2] Robinson-Mosher, Schroeder, Fedkiw, **A symmetric positive definite formulation
for monolithic fluid structure interaction**, Journal of Computational Physics,
2011 (author manuscript). Section 4.1-4.3, Eqs.8-12: massive fluid duals, interface
constraint impulses, transpose body reactions, same application locations for torque.
PDF pp.5-7 visually inspected. Our B/sign/area-scaled notation is a specialization,
not their verbatim complete algorithm.
https://physbam.stanford.edu/papers/stanford2010-05.pdf

[3] van der Eijk and Wellens, **Two-phase free-surface flow interaction with moving
bodies using a consistent, momentum preserving method**, JCP 474 (2023), 111796.
Sections 2, 4.4-4.5, Eqs.32-37. Used to check two-phase scope and distinguish its
Crank-Nicolson body update from the present projection component. PDF p.14 inspected.
https://repository.tudelft.nl/file/File_29dd44e8-6939-4496-a557-c8e6d6e60ef7?preview=1

[4] Azevedo, Batty, Oliveira, **Preserving Geometry and Topology for Fluid Flows with
Thin Obstacles and Narrow Gaps**, 2016. Sections 3-4. Thin-wall connectivity is not
solved by single background-cell pressure ownership; this paper is not two-way JSGT.
https://cs.uwaterloo.ca/~c2batty/papers/Azevedo2016.pdf

[5] PETSc official documentation, **MatSetNullSpace**. Singular-system compatibility
and the difference between an exact compatible solution and least-squares repair.
https://petsc.org/release/manualpages/Mat/MatSetNullSpace/

[6] PETSc official documentation, **KSPCG**. Symmetry/definiteness requirements of CG.
This citation does not require or recommend adding PETSc to the prototype.
https://petsc.org/release/manualpages/KSP/KSPCG/

No source supplies Judas's complete next implementation unchanged. The equations,
geometry restriction, reference program and acceptance interpretation are our
explicit derivation/specialization, separately tested here.

Implementation-library references checked during handoff preparation:

[7] Eigen official JacobiSVD documentation. Used only for the numerical
factorization library, not as a fluid-model reference.
https://eigen.tuxfamily.org/dox/classEigen_1_1JacobiSVD.html

[8] Eigen 3.4.0 COPYING.README. Records primarily MPL2 licensing and the
EIGEN_MPL2_ONLY build option. Preserve actual dependency notices; do not relabel
third-party code as Judas MIT source.
https://gitlab.com/libeigen/eigen/-/raw/3.4.0/COPYING.README
