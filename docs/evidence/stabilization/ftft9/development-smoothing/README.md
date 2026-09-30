# Isolated smoothing accounting

`judas_fluid_smoothing_budget_tests` executes ordinary `FluidWorld::Step` with one substep, zero gravity, no boundaries, zero density-position iterations and the normal .15 velocity-smoothing coefficient. Thus only the production smoothing velocity exchange affects its budget. The independent oracle directly sums particle mass times velocity and kinetic energy.

`before.log/json` preserves two genuine failures (physical exit 1): unequal masses gain 0.0168627500534 kg m/s, and equal masses with an asymmetric neighbourhood gain 0.0100630223751 kg m/s. The exact pre-fix source is retained beside the logs. Energy decreases in both; the demonstrated defect is momentum creation.

The parent repair uses the symmetric denominator `max(rho_i,rho_j)` and rejects nonfinite or out-of-range smoothing coefficients instead of clipping them. `after-symmetric.log/json` records both momentum residuals exactly zero in the reported double sums and all four invalid configurations rejected; physical exit 0.

For coefficient s in [0,1], each row sum is `s*sum_j(m_j*W_ij/max(rho_i,rho_j)) <= s*sum_j(m_j*W_ij/rho_i) <= s`. The velocity update is a convex combination. A pair contributes equal/opposite momentum because its denominator is shared. Summing the mass-weighted convex kinetic-energy bound therefore proves this smoothing operation cannot create kinetic energy in exact arithmetic. This statement is about smoothing alone, not the complete PBF position correction or exterior-body approximation.

Reproduce: `cmake --build build --target judas_fluid_smoothing_budget_tests -j2 && build/judas_fluid_smoothing_budget_tests`.
