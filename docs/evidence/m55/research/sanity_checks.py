"""Independent M55 research sanity checks, NOT a Judas solver or benchmark.

Models tested:
  1. Zero-mean height offsets are not volume preserving at a wet/dry shore.
  2. Zero-mean radial offsets are not volume preserving on a spherical shell.
  3. A closed, fully wet, linearized 1D surface-wave system advanced by a
     theta-method has shared conservative face fluxes and does not acquire
     an explicit gravity-wave stability restriction when depth increases.

Not tested: nonlinear transport, wet/dry numerical method, moving solids,
containers, curved dynamics, scattering, rendering, or game performance.
Dependencies: numpy, scipy. Run: python3 sanity_checks.py
"""
from __future__ import annotations
import json
from pathlib import Path
import numpy as np
from scipy.sparse import diags, eye
from scipy.sparse.linalg import factorized


def linear_surface(depth: float) -> dict[str, float | int | bool]:
    n = 128
    dx, width, dt, g, theta = 0.25, 1.0, 1.0/30.0, 9.81, 0.55
    area = dx*width
    steps = 600
    # D maps INTERNAL face discharges to outward cell discharge. End walls
    # have zero flow, not periodic boundaries and not absorbing boundaries.
    D = diags([np.ones(n-1), -np.ones(n-1)], [0, -1], shape=(n,n-1), format='csc')
    G = -D.T
    beta = g*depth*width/dx
    L = D @ (beta*G)
    solve = factorized(eye(n, format='csc') - dt*dt*theta*theta/area*L)
    x = (np.arange(n)+0.5)*dx
    eta = 0.05*np.exp(-((x-0.35*n*dx)/1.0)**2)
    eta -= eta.mean()  # Initial volume-neutral perturbation, not a runtime repair.
    q = np.zeros(n-1)
    initial_volume = area*np.sum(depth+eta)
    energy = lambda e,f: float(0.5*g*area*np.dot(e,e) + 0.5*dx/(depth*width)*np.dot(f,f))
    e0 = energy(eta,q)
    previous_energy = e0
    max_drift, max_continuity, max_energy_increase = 0.0, 0.0, 0.0
    min_depth = float(np.min(depth+eta))
    for _ in range(steps):
        old_eta, old_q = eta.copy(), q.copy()
        rhs = old_eta - dt/area*(D@old_q) + dt*dt*theta*(1-theta)/area*(L@old_eta)
        eta = solve(rhs)
        q = old_q - dt*beta*(G@((1-theta)*old_eta+theta*eta))
        continuity = area*(eta-old_eta) + dt*(D@((1-theta)*old_q+theta*q))
        max_continuity = max(max_continuity,float(np.max(np.abs(continuity))))
        max_drift = max(max_drift, abs(float(area*np.sum(depth+eta)-initial_volume)))
        e = energy(eta,q)
        max_energy_increase = max(max_energy_increase,e-previous_energy)
        previous_energy = e
        min_depth = min(min_depth,float(np.min(depth+eta)))
    assert np.isfinite(eta).all() and np.isfinite(q).all()
    assert max_drift <= 1e-10*max(initial_volume,1.0)
    assert max_energy_increase <= 1e-12*max(e0,1.0)
    return {
        'cells': n, 'depth_m': depth, 'dt_s': dt, 'steps': steps,
        'gravity_wave_courant': float(np.sqrt(g*depth)*dt/dx),
        'max_volume_drift_m3': max_drift,
        'max_cell_continuity_residual_m3': max_continuity,
        'max_energy_increase_per_step_per_density': max_energy_increase,
        'final_to_initial_perturbation_energy': energy(eta,q)/e0,
        'minimum_total_depth_m': min_depth,
        'finite': True,
    }


def main() -> None:
    bed = np.array([0.0,0.5,1.5])
    baseline = np.ones(3)
    offsets = np.array([-0.1,0.0,0.1])
    # max describes the geometric water volume, NOT a numerical clamp.
    v_before = float(np.maximum(baseline-bed,0).sum())
    v_after = float(np.maximum(baseline+offsets-bed,0).sum())
    r, a, omega = 10.0, 0.1, 1.0
    radial_change = omega*((r+a)**3+(r-a)**3-2*r**3)/3
    expected_radial_change = 2*omega*r*a*a
    assert abs(radial_change-expected_radial_change)<1e-10
    out = {
        'scope': __doc__,
        'wet_dry_zero_mean_counterexample': {
            'cell_area_m2': 1.0, 'bed_m': bed.tolist(),
            'baseline_surface_m': baseline.tolist(), 'offsets_m': offsets.tolist(),
            'sum_offsets_m': float(offsets.sum()),
            'volume_before_m3': v_before, 'volume_after_m3': v_after,
            'volume_error_litres': (v_after-v_before)*1000,
        },
        'radial_zero_mean_counterexample': {
            'radius_m': r, 'offsets_m': [a,-a], 'solid_angle_each_sr': omega,
            'volume_increase_m3': radial_change,
            'analytic_volume_increase_m3': expected_radial_change,
        },
        'linear_surface_cases': [linear_surface(d) for d in (1.0,10.0,100.0)],
        'limitations': [
            'No Judas implementation was accessed or altered.',
            'No hardware/runtime performance claim.',
            'The time integration is a linear fully-wet illustration only.',
            'Stability at a large Courant number does not establish wave accuracy.',
            'No production wet/dry, curved, or fluid-solid solver is validated.',
        ],
    }
    path = Path(__file__).with_name('sanity_results.json')
    path.write_text(json.dumps(out, indent=2)+'\n')
    print(json.dumps(out,indent=2))

if __name__=='__main__':
    main()
