# FTFT4B source references

## Selected simultaneous impact model

Uchida, T. K.; Sherman, M. A.; Delp, S. L. (2015).
Making a meaningful impact: modelling simultaneous frictional collisions in spatial multibody systems.
Proceedings of the Royal Society A, 471(2177), 20140859.
DOI: 10.1098/rspa.2014.0859
Open article:
https://pmc.ncbi.nlm.nih.gov/articles/PMC4984984/

Key reasons selected:
- spatial simultaneous frictional impact;
- Poisson compression/expansion restitution;
- induced-impact rounds;
- redundant contact load spreading;
- circular Coulomb friction;
- changing sliding direction;
- rolling/sliding/impending-slip transitions;
- explicit numerical tolerances;
- separate impact and persistent-contact handling in the associated Simbody architecture.

## Simbody implementation/documentation

ImpulseSolver:
https://simbody.github.io/simbody-3.6-doxygen/api/classSimTK_1_1ImpulseSolver.html

General documentation:
https://simbody.github.io/3.7.0/

License:
https://simbody.github.io/3.7.0/simbody_license_page.html

Simbody is Apache-2.0 licensed.

FTFT4B does not authorize making Simbody a Judas dependency.
It is a published/open-source implementation reference.

## Set-valued simultaneous impacts (supporting context)

Halm, M.; Posa, M. (2024).
Set-valued rigid-body dynamics for simultaneous, inelastic, frictional impacts.
International Journal of Robotics Research 43(10), 1594–1628.
DOI: 10.1177/02783649241236860

https://journals.sagepub.com/doi/10.1177/02783649241236860

This source is useful context for simultaneous-impact ambiguity, energy dissipation and finite-duration results in the purely inelastic/set-valued setting.
It is not the selected FTFT4B restitution model.

## Prior Judas-specific research included in this package

- FTFT4 contact geometry/timing research
- FTFT4B event timing research
- FTFT4B impact-response research

Those packages contain the independent exact/scalar oracles and rejected alternatives that motivated the final selection.
