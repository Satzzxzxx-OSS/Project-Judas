#!/usr/bin/env python3
"""Verify raw executed evidence and record a reproducible isolated checkpoint."""
import csv, datetime, hashlib, json, math, platform, shutil, subprocess
from pathlib import Path
P=Path(__file__).resolve().parent
E=P/'evidence/p1cm/current'
def rows(path):
    with path.open() as f:return list(csv.DictReader(f))
def maximum(rs,key,absolute=False):return max((abs(float(r[key])) if absolute else float(r[key])) for r in rs)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
legacy=rows(P/'results/jsgt_summary.csv')
baseline=rows(P/'evidence/p1cm/baseline/rerun_results/jsgt_summary.csv')
assert len(legacy)==len(baseline)==25
for old,new in zip(baseline,legacy):
    for key in old:
        if key!='runtime_s':assert old[key]==new[key],(old['test'],key,old[key],new[key])
# The fraction paths should reproduce whole per-stage/per-step records as well.
for name in ['jsgt_trace.csv','jsgt_steps.csv','jsgt_nonlinear_oracle.csv']:
    assert sha(P/'results'/name)==sha(P/'evidence/p1cm/baseline/rerun_results'/name),name
momentum=rows(E/'momentum_summary.csv'); steps=rows(E/'momentum_steps.csv')
regression=rows(E/'fraction_regression_momentum.csv'); controls=rows(E/'momentum_controls.csv')
assert len(momentum)==30 and len(steps)==465 and len(regression)==1190
assert all(r['status']=='PASS' and r['steps']==r['requested_steps'] for r in momentum)
assert all(r['valid']=='1' for r in regression)
assert all(r['accepted']=='1' for r in steps)
assert all(r['status'] in ['PASS','DETECTED'] for r in controls)
for name in ['fine_mass_norm','dual_mass_norm','combination_mass_norm']:
    assert maximum(steps,name)<=1e-12
for name in ['core_cumulative_mass_norm','core_cumulative_px_norm','core_cumulative_py_norm']:
    assert maximum(steps,name)<=1e-11
assert maximum(steps,'combination_energy_norm')<=1e-12
refine=[r for r in momentum if r['test']=='smooth_momentum_refinement']
for key in ['final_L1_x','final_L1_y']:
    assert all(float(a[key])>float(b[key]) for a,b in zip(refine,refine[1:]))
assert 'P1-C-M momentum validation PASS' in (E/'run_output.txt').read_text()
# Attach the actual final fraction evidence without changing the preserved checkpoint.
for name in ['jsgt_summary.csv','jsgt_trace.csv','jsgt_steps.csv','jsgt_nonlinear_oracle.csv']:
    shutil.copy2(P/'results'/name,E/name)
source_names=['main.cpp','mass_momentum.hpp','momentum_validation.hpp','vof.hpp','transport_oracles.hpp','nonlinear_transport_oracle.hpp','CMakeLists.txt','run_p1cm.sh','record_p1cm.py']
S=E/'source_snapshot';S.mkdir(exist_ok=True)
for name in source_names:shutil.copy2(P/name,S/name)
fingerprints={name:sha(P/name) for name in source_names}
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=P,text=True).strip()
status=subprocess.check_output(['git','-c','core.fsmonitor=false','status','--short'],cwd=P.parents[2],text=True)
# Reruns rewrite tracked prototype evidence. Reject changes outside this isolated
# directory instead of assuming all prototype sources/results are untracked.
prefix='prototypes/fluid_coupling/r1_p1/'
def changed_paths(cached=False):
    command=['git','-c','core.fsmonitor=false','diff','--no-relative']
    if cached:command.append('--cached')
    return subprocess.check_output(command+['--name-only','-z'],cwd=P.parents[2],text=True).split('\0')[:-1]
diff_paths=changed_paths();staged_paths=changed_paths(cached=True)
assert all(path.startswith(prefix) for path in diff_paths+staged_paths), 'Changes outside isolated prototype'
diff=''.join(path+'\n' for path in diff_paths)
staged=''.join(path+'\n' for path in staged_paths)
frac_stages=rows(E/'jsgt_trace.csv')+rows(E/'momentum_fraction_trace.csv')
alpha_min=min(float(r['min_alpha']) for r in frac_stages)
alpha_max=max(float(r['max_alpha']) for r in frac_stages)
assert all(int(r['beyond_roundoff_cells'])==0 for r in frac_stages)
mxmass=max(maximum(steps,'core_cumulative_mass_norm'),maximum(regression,'mass_budget_norm'))
mxp=max(maximum(steps,'core_cumulative_px_norm'),maximum(steps,'core_cumulative_py_norm'),maximum(regression,'px_budget_norm'),maximum(regression,'py_budget_norm'))
mxlocal=max(maximum(steps,'fine_mass_norm'),maximum(steps,'dual_mass_norm'),maximum(steps,'combination_mass_norm'),maximum(regression,'fine_mass_norm'),maximum(regression,'dual_mass_norm'),maximum(regression,'combination_mass_norm'))
mxcommon=max(maximum(steps,'common_vector_error'),maximum(regression,'common_x_error'),maximum(regression,'common_y_error'))
mxenergy=max(maximum(steps,'combination_energy_norm'),maximum(regression,'combination_energy_norm'))
run_time=sum(float(r['runtime_s']) for r in momentum)+sum(float(r['runtime_s']) for r in legacy)
meta={'result':'P1-C-M PASS; full P1 incomplete','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
      'repository_head_at_run':head,'production_reference':'48385a78afb2930744f2b87ad6aa55452f18ae18','compiler':subprocess.check_output(['c++','--version'],text=True).splitlines()[0],
      'platform':platform.platform(),'command':f'bash {P}/run_p1cm.sh','build_flags':'C++17 -Wall -Wextra -Wpedantic -O2',
      'fraction_regression_runs':25,'fraction_regression_steps':1190,'momentum_runs':30,'momentum_steps':465,
      'total_compiled_runs':55,'total_compiled_steps':1655,'fraction_regression_bit_identical_except_timing':True,
      'source_sha256':fingerprints,'evidence_sha256':{f.name:sha(f) for f in sorted(E.iterdir()) if f.suffix in ['.csv','.txt'] and f.name!='build_output.txt'},
      'gate_local':1e-12,'gate_global':1e-11,'gate_common':1e-8,'gate_combination_energy':1e-12,
      'gate_fraction':256*2.220446049250313e-16,'git_status':status,'git_diff':diff,'git_staged_diff':staged,
      'runner_performed_git_mutations':False,'tracked_changes_confined_to_prototype':True,'scope':'fixed-grid VOF/mass/component-momentum transport with prescribed advector'}
(E/'RUN_METADATA.json').write_text(json.dumps(meta,indent=2)+'\n')
body=['# P1-C-M results — PASS\n',
'**The compiled Judas prototype passes the specified fixed-grid mass-consistent momentum checks. This does not complete integrated P1.** Pressure, moving solids, cut cells, P1-D/E, buoyancy, swimming and production integration were not run.\n',
'## Reproduce\n',f'```sh\nbash {P}/run_p1cm.sh\n```\n',
'The command configures/builds the existing prototype, executes its single compiled implementation, verifies regression/evidence gates, and records sources/hashes. No external Python packages are required for this C++ validation.\n',
'## Executed scope and checkpoint preservation\n',
'- Before editing: all 25 accepted P1-C runs reran identically except timings; sources and results were copied and fingerprinted in `evidence/p1cm/baseline/`.\n- Final execution: **55 runs / 1,655 timesteps**: 25 existing fraction cases with momentum active (1,190 steps), plus 30 momentum cases (465 steps). No requested test family was skipped.\n- Every prior fraction summary field except timing is identical. Full per-stage, per-step and nonlinear-oracle CSV files are byte-identical to the pre-edit rerun.\n- One actual JSGT phase-flux path supplies all mass/momentum stages. There is no alternate PLIC or phase flux path for momentum.\n',
'## Mechanism and changed files\n',
'- `main.cpp`: opt-in periodic geometry indexing, actual phase-transfer/state records, momentum attached to all legacy runs, new validation runner in the same executable. No original fixture, timestep policy, normal estimate or fraction arithmetic was retuned.\n- New `mass_momentum.hpp`: fine mass/flux/source records, separate staggered partitions, half-duals at physical boundaries, segment-first upwinding, frozen source velocity, both branch updates and conserved combination.\n- New `momentum_validation.hpp`: analytical expectations, fixtures, covariance and negative controls; calls the shared implementation.\n- New `run_p1cm.sh`, `record_p1cm.py`: reproducible execution and evidence verification.\n- New `P1CM_METHOD.md`, `P1CM_RESULTS.md`; updated README/STATUS and scope notices in the three preserved P1-C documents.\n- Added evidence under `evidence/p1cm/`, including the unchanged supplied package, reference rerun, preserved baseline, logs, CSVs and snapshots. The build remains `.build-jsgt/`.\n- `vof.hpp`, original oracle headers and CMake configuration are unchanged.\n',
'## Numerical gate results\n',
'| Quantity | Worst measured normalized error | Gate |\n|---|---:|---:|',
f'| Fine/dual/combination mass consistency | {mxlocal:.6e} | 1e-12 |',
f'| Global mass, including boundary transfer | {mxmass:.6e} | 1e-11 |',
f'| Component momentum, including boundary transfer | {mxp:.6e} | 1e-11 |',
f'| Constant transported vector, raw | {mxcommon:.6e} | 1e-8 * Uref |',
f'| Combination energy inequality | {mxenergy:.6e} | 1e-12 |',
f'\nRaw fractions over all compiled stages: **[{alpha_min:.17g}, {alpha_max:.17g}]**, zero material excursions under the unchanged 256-epsilon tolerance. Stored fractions were not repaired. The existing roundoff endpoint geometry convention remains explicit.\n',
'All masses stayed positive and states finite. Local scales use actual dual volumes, masses, absolute boundary transfers and sources. Raw and normalized values are logged. Core global budgets do not subtract the source residual; a second long-double audit separately reports source-adjusted budgets. Source cancellation is checked, not assumed.\n',
'## Momentum fixtures\n',
'The reference-family random generator is seeded xorshift64*, not NumPy. Its pure/mixed geometry and test categories reproduce the supplied family; invariants are compared, not PLIC/RNG bit patterns. Constant vectors in nonuniform prescribed flow are manufactured transport checks. The dedicated common-co-motion family explicitly tests transported velocity equal to the advector; the open-slab case does likewise, and the uniform reference smooth case degenerates to a constant matching vector.\n',
'| Case | Fine/MAC | Density ratio | Steps | Local mass max norm | Global mass max norm | Global momentum max norm | Common-vector error | Runtime s | Result |\n|---|---:|---:|---:|---:|---:|---:|---:|---:|---|']
for r in momentum:
    body.append(f"| {r['test']} | {r['fine_n']}/{r['MAC_n']} | {float(r['density_ratio']):g} | {r['steps']} | {float(r['local_mass_max_norm']):.3e} | {float(r['cumulative_mass_max_norm']):.3e} | {float(r['cumulative_momentum_max_norm']):.3e} | {float(r['common_vector_max_error']):.3e} | {float(r['runtime_s']):.6g} | {r['status']} |")
body+=['\n## Momentum refinement\n','Exact sine/cosine averages over actual dual rectangles initialize and evaluate the translated solution. rho_l=rho_g=1, advector=(0.37,-0.21), final time=0.5.\n','| Fine/MAC | dt | L1 x | Order x | L1 y | Order y |\n|---|---:|---:|---:|---:|---:|']
previous=None
for r in refine:
    ex,ey=float(r['final_L1_x']),float(r['final_L1_y'])
    ox='—' if previous is None else f'{math.log2(previous[0]/ex):.3f}'
    oy='—' if previous is None else f'{math.log2(previous[1]/ey):.3f}'
    body.append(f"| {r['fine_n']}/{r['MAC_n']} | {float(r['dt']):.9g} | {ex:.10g} | {ox} | {ey:.10g} | {oy} |")
    previous=(ex,ey)
body+=['\nBoth component errors decrease, approaching first-order behavior. No second-order claim is made.\n',
'## Open boundaries, controls and covariance\n',
'The open slab uses periodic y and physical x boundaries. It explicitly checks retained liquid 0.05 and net exports 0.15*(rho_l-rho_g) in mass and 0.3 times that amount in x-momentum. Gas inflow is included; physical boundary half-duals contain no ghost mass.\n',
'| Check | Actual | Expected / diagnostic | Status |\n|---|---:|---:|---|']
for r in controls:
    if r['quantity']=='prolongation_fine_parent_divergence':continue
    body.append(f"| {r['test']}: {r['quantity']} | {float(r['actual']):.12g} | {r['expected']} (diagnostic {float(r['error_or_wrong_result']):.4g}) | {r['status']} |")
body+=['\nThe counterflow primitive is the same segment donor function used in active transport. The wrong-old-mass and independent-velocity-average diagnostics use actual branch states but never feed the accepted result. Source-only algebra is an auxiliary negative control; source cancellation is also checked on actual nonuniform runs.\n',
f"The maximum prolongation fine-minus-parent divergence defect is {max(float(r['actual']) for r in controls if r['quantity']=='prolongation_fine_parent_divergence'):.6e}. Existing P1-C velocities were never prolonged.\n",
'## Energy and velocity diagnostics\n',
'Energy is sum_q sum_K Pq^2/(2Mq), treating the two component partitions separately. Combination convexity is checked against the mean branch energy. Full-step energy is independently recorded; it is not inferred from that inequality. Donor-cell diffusion is expected.\n',
'| Case | Ratio | Initial E | Final E | Max step growth norm | Minimum component velocity | Maximum component velocity |\n|---|---:|---:|---:|---:|---:|---:|']
for r in momentum:
    ss=[s for s in steps if s['test']==r['test'] and s['density_ratio']==r['density_ratio'] and s['dt']==r['dt']]
    body.append(f"| {r['test']} | {float(r['density_ratio']):g} | {float(ss[0]['kinetic_before']):.9g} | {float(ss[-1]['kinetic_after']):.9g} | {float(r['full_step_energy_max_growth_norm']):.3e} | {min(min(float(v['wx_min']),float(v['wy_min'])) for v in ss):.9g} | {max(max(float(v['wx_max']),float(v['wy_max'])) for v in ss):.9g} |")
body+=['\nNo material full-step energy growth appeared in the closed cases; the small positive values are numerical roundoff. This is observed behavior for these fixtures, not a full transport or Navier-Stokes stability theorem. Per-component stage extrema, raw energy changes and budgets are available in `momentum_steps.csv`; legacy-case component extrema and energies are in `fraction_regression_momentum.csv`.\n',
'## Legacy fraction regression with momentum active\n',
'| Case | Fine grid | Steps | Fraction L1 | Runtime s | Result |\n|---|---:|---:|---:|---:|---|']
for r in legacy:body.append(f"| {r['test']} | {r['n']} | {r['completed_steps']} | {float(r['final_L1']):.9g} | {float(r['runtime_s']):.6g} | {r['status']} |")
body += [f'\nTotal final compiled-fixture runtime: **{run_time:.6f} s**, including fraction reconstruction, analytical expectations and heavy CSV diagnostics. The 30 new momentum runs account for {sum(float(r["runtime_s"]) for r in momentum):.6f} s. These are not production performance claims.\n',
'## Supporting Python evidence\n',
'The supplied manifest and nested original-archive manifest passed all 13 hashes. The corrected fixed-grid functions reran as 18 cases / 216 timesteps plus two analytical witnesses and exactly matched the shipped numerical records. Bundled Python 3.12.14 and NumPy 2.3.5 were used. Shapely was unavailable, so a recorded AST loader executed unchanged selected functions while omitting only unused Shapely imports and excluded moving-geometry functions. No moving-geometry checks were run or claimed. See `evidence/p1cm/reference_rerun/`. Its PASS is supporting evidence only.\n',
'## Source fingerprints and repository state\n',
'| File | SHA-256 |\n|---|---|']
for name,h in fingerprints.items():body.append(f'| {name} | `{h}` |')
body += [f'\nRepository HEAD at execution: `{head}`. Tracked and staged changes were checked to be confined to the isolated prototype. Git status captured by this run, before the checkpoint commit:\n\n```text\n{status.rstrip()}\n```\n',
'Current raw evidence and matching source snapshots are in `evidence/p1cm/current/`. `RUN_METADATA.json` fingerprints both sources and executed evidence. The preserved baseline and earlier complete-run logs remain separate. The build/evidence runner does not commit, tag or push. Its metadata records the execution state; the Git checkpoint is a separate operator-authorized action. Machine-specific `build_output.txt` is excluded from version control and the evidence manifest.\n',
'**Decision: P1-C-M PASS under the specified gates. Stop here for operator review. Integrated P1, pressure/body coupling and P1-D/E remain incomplete.**\n']
(P/'P1CM_RESULTS.md').write_text('\n'.join(body))
print(f'P1-C-M VERIFIED: 55 compiled runs / 1655 timesteps; baseline fraction records identical; report {P}/P1CM_RESULTS.md')
