#!/usr/bin/env python3
"""Verify executed compiled evidence and preserve an auditable P1-PF packet."""
import csv, datetime, hashlib, json, math, os, platform, shutil, subprocess
from pathlib import Path
P=Path(__file__).resolve().parent
E=P/'evidence/p1pf/current'
BASE=P/'evidence/p1pf/baseline/rerun_p1cm_current'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):
    with p.open() as f:return list(csv.DictReader(f))
def peak(rows,key):return max(abs(float(r[key])) for r in rows)
projection=read(E/'projection_summary.csv');neg=read(E/'negative_controls.csv');adapters=read(E/'adapters_summary.csv');counts=read(E/'projection_counts.csv')[0]
oracles=read(E/'projection_oracles.csv');asteps=read(E/'adapters_transport_steps.csv')
rejections=read(E/'input_rejections.csv');assert len(rejections)==9 and all(r['status']=='REJECTED' for r in rejections)
assert len(projection)==42 and len(neg)==5 and len(adapters)==6 and len(asteps)==72
assert counts['status']=='PASS' and counts['positives']=='42' and counts['negative_controls']=='5' and counts['idempotence_calls']=='6'
expected=json.loads((P/'evidence/p1pf/handoff/reference/projection_results.json').read_text())
assert [r['name'] for r in projection]==[r['name'] for r in expected['results']]
assert {r['name'] for r in neg}=={r['name'] for r in expected['negative']}
assert all(r['status']=='SOLVED' for r in projection)
assert all(r['status']=='DETECTED' for r in neg)
assert all(r['status']=='PASS' for r in adapters+asteps)
assert all(r['status'] in ['PASS','MEASURED'] for r in oracles)
for k in ['constraint_residual','impulse_residual','energy_identity_residual','momentum_budget_residual','mass_partition_error']:assert peak(projection,k)<=1e-9,k
for k in ['constraint_normalized','impulse_normalized','energy_normalized','px_budget_normalized','py_budget_normalized','idempotence_error']:assert peak(adapters,k)<1e-9,k
for k in ['fraction_bytes_unchanged','mass_bytes_unchanged','advector_bytes_unchanged','transport_momentum_bytes_unchanged']:assert all(r[k]=='1' for r in adapters),k
assert peak(adapters,'common_velocity_error')<1e-8
cm=P/'evidence/p1cm/current'
legacy=read(cm/'jsgt_summary.csv');momentum=read(cm/'momentum_summary.csv')
assert len(legacy)==25 and len(momentum)==30 and all(r['status']=='PASS' for r in legacy+momentum)
regression={}
for name in ['jsgt_summary.csv','momentum_summary.csv']:
    old=read(BASE/name);new=read(cm/name);assert len(old)==len(new)
    for a,b in zip(old,new):
        for k in a:
            if k!='runtime_s':assert a[k]==b[k],(name,k,a[k],b[k])
    regression[name]='All non-timing fields identical'
for name in ['jsgt_trace.csv','jsgt_steps.csv','jsgt_nonlinear_oracle.csv','momentum_steps.csv','momentum_fraction_trace.csv','momentum_controls.csv','fraction_regression_momentum.csv']:
    assert sha(BASE/name)==sha(cm/name),name
    regression[name]='Byte-identical'
assert not (E/'FAILURE.txt').exists(),'Unarchived failure evidence; no acceptance over stale failure'
source_names=['main.cpp','vof.hpp','transport_oracles.hpp','nonlinear_transport_oracle.hpp','mass_momentum.hpp','momentum_validation.hpp','frozen_projection.hpp','p1pf_validation.hpp','p1pf_adapters.hpp','CMakeLists.txt','run_p1cm.sh','record_p1cm.py','run_p1pf.sh','record_p1pf.py','prepare_p1pf_reference.py','P1PF_METHOD.md','fixtures/p1pf_random_inputs.csv','dependencies/OpenBLAS-COPYRIGHT']
S=E/'source_snapshot';S.mkdir(exist_ok=True)
for name in source_names:
    target=S/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(P/name,target)
transport_evidence=E/'transport_regression';transport_evidence.mkdir(exist_ok=True)
for f in cm.iterdir():
    if f.is_file() and f.name!='build_output.txt':shutil.copy2(f,transport_evidence/f.name)
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=P,text=True).strip()
status=subprocess.check_output(['git','-c','core.fsmonitor=false','status','--short'],cwd=P.parents[2],text=True)
for args in [[],['--cached']]:
    changed=subprocess.check_output(['git','diff',*args,'--name-only'],cwd=P.parents[2],text=True).splitlines()
    assert all(s.startswith('prototypes/fluid_coupling/r1_p1/') for s in changed),changed
cache=(P/'.build-p1pf/CMakeCache.txt').read_text().splitlines()
lib=Path(next(s.split('=',1)[1] for s in cache if s.startswith('P1PF_LAPACK:FILEPATH='))).resolve()
meta={'result':'P1-PF PASS; frozen resolved geometry only; integrated P1 incomplete','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'starting_checkpoint':'0d73f47206cc34674bb9fbbce2cb438f38ce9f22','repository_head':head,'git_status':status,'projection_cases':42,'negative_controls':5,'adapters':6,'adapter_transport_steps':72,'reference_case_identities_preserved':True,'projection_fixture_solver_calls':int(counts['total_solver_calls']),'fixture_idempotence_calls':6,'adapter_idempotence_calls':6,'adapter_solver_calls':12,'preserved_transport_runs':55,'preserved_transport_steps':1655,'regression':regression,'source_sha256':{n:sha(P/n) for n in source_names},'compiler':subprocess.check_output(['c++','--version'],text=True).splitlines()[0],'platform':platform.platform(),'lapack_library':str(lib),'lapack_library_sha256':sha(lib),'OPENBLAS_NUM_THREADS':os.environ.get('OPENBLAS_NUM_THREADS'),'OMP_NUM_THREADS':os.environ.get('OMP_NUM_THREADS'),'command':'bash prototypes/fluid_coupling/r1_p1/run_p1pf.sh','runtime_projection_fixture_seconds':float(counts['runtime_s']),'runtime_adapters_seconds':sum(float(r['runtime_s']) for r in adapters),'runtime_transport_seconds':sum(float(r['runtime_s']) for r in legacy+momentum),'no_commit_push_or_tag':True,'malformed_inputs_rejected':len(rejections),'lapack_version':(E/'lapack_version.txt').read_text().strip()}
meta['evidence_sha256']={str(f.relative_to(E)):sha(f) for f in sorted(E.rglob('*')) if f.is_file() and 'source_snapshot' not in f.parts and f.name not in ['RUN_METADATA.json','build_output.txt']}
(E/'RUN_METADATA.json').write_text(json.dumps(meta,indent=2)+'\n')
parts=['# P1-PF results — PASS\n','**42 frozen-geometry projection cases, five detected negative controls, and six actual compiled-transport adapters passed.** This is a component gate. No body poses were advanced and P1-D/E remains unimplemented.\n','## Reproduce\n','```sh\nbash prototypes/fluid_coupling/r1_p1/run_p1pf.sh\n```\n','Requires CMake, a C++17 compiler, LP64 LAPACK, Bash and Python 3 standard library. See `P1PF_METHOD.md` for exact equations and dependency notices. No Python numerical package is called by the compiled projection or this verification script.\n','## Executed counts and preserved checkpoint\n',f'- Reference component fixtures: **42**; negative controls: **5**.\n- Component idempotence calls: **6**; total component factorization calls including negative controls: **{counts["total_solver_calls"]}**.\n- Actual C++ transport adapters: **6**, each after **12** timesteps; **6** primary projections plus **6** separate idempotence calls.\n- Original transport gate: **55 runs / 1,655 timesteps**, unchanged numerical summaries. Original fraction stage/step/oracle records and four momentum/controls records are byte-identical to the pre-edit rerun.\n- No specified fixture was skipped. The 72 adapter transport steps are additional to the preserved 1,655, not additional body timesteps.\n','## Worst normalized component residuals\n','| Measure | Worst | Gate |\n|---|---:|---:|']
for k in ['constraint_residual','impulse_residual','energy_identity_residual','momentum_budget_residual','mass_partition_error']:parts.append(f'| {k} | {peak(projection,k):.6e} | 1e-9 |')
parts+=['\n## Physical comparisons\n','| Case / quantity | Actual | Expected | Absolute error | Gate |\n|---|---:|---:|---:|---:|']
for r in oracles:
    if r['quantity'] in ['hydro_force_y','hydro_torque','fluid_force_y','support_force_y','piston_velocity','pressure_difference','free_fall_correction','constant_pressure_error','hydro_pressure_relative_error','neutral_body_speed'] or (r['name'].startswith('arbitrary_gravity') and r['quantity'].startswith('pressure_force')):
        parts.append(f'| {r["name"]} / {r["quantity"]} | {float(r["actual"]):.12g} | {float(r["expected"]):.12g} | {float(r["error"]):.3e} | {float(r["tolerance"]):g} |')
parts+=['\nSupported-body component force records for all five densities, all physical oracle checks, free-body linear/angular responses, per-cell pressures/divergences and per-interface normal mismatches are in the CSV files. The piston oracle uses whole-system analytical inertia; the solver contains no such formula.\n','## Smooth pressure refinement\n','| MAC resolution | Pressure L1 (Pa) | Ratio to next grid |\n|---|---:|---:|']
ref=[r for r in oracles if r['quantity']=='pressure_L1']
for i,r in enumerate(ref):parts.append(f'| {r["name"].rsplit("_",1)[1]} | {float(r["actual"]):.12g} | '+(f'{float(r["actual"])/float(ref[i+1]["actual"]):.6f}' if i+1<len(ref) else '—')+' |')
parts+=['\nBoth ratios satisfy the unchanged 3.7–4.4 gate. This smooth, solid-free pressure result does not establish second-order momentum transport or cut-cell accuracy.\n','## Case inventory and conditioning\n','| Case | Matrix rows × columns | Rank / nullity | H min / max | Whitened condition | Constraint residual | Solve seconds |\n|---|---:|---:|---:|---:|---:|---:|']
for r in projection:parts.append(f'| {r["name"]} | {r["rows"]} × {r["columns"]} | {r["rank"]} / {r["nullity"]} | {float(r["mass_min"]):.4g} / {float(r["mass_max"]):.4g} | {float(r["condition"]):.5g} | {float(r["constraint_residual"]):.3e} | {float(r["solve_seconds"]):.6g} |')
parts+=['\nEvery rank threshold and singular extreme is recorded. The sealed piston has one coupled pressure gauge. Per-body mass/inertia and impulse mismatch are recorded separately. Global budget gates include external walls and supports.\n','## Six actual transport adapters\n','| Case | Constraint | Impulse | Energy | Momentum budget | Idempotence | Common-vector error | Time s |\n|---|---:|---:|---:|---:|---:|---:|---:|']
for r in adapters:parts.append(f'| {r["name"]} | {float(r["constraint_normalized"]):.3e} | {float(r["impulse_normalized"]):.3e} | {float(r["energy_normalized"]):.3e} | {max(abs(float(r["px_budget_normalized"])),abs(float(r["py_budget_normalized"]))):.3e} | {float(r["idempotence_error"]):.3e} | {float(r["common_velocity_error"]):.3e} | {float(r["runtime_s"]):.6g} |')
parts+=['\nAll six preserve fractions, fine/dual masses, input momentum snapshots and prescribed advectors byte-for-byte through projection. Output momentum is returned separately. Every periodic seam has one shared DOF. Nonuniform cases are snapshots of prescribed transport, not a validated Navier–Stokes time integrator.\n','## Negative controls and rejected inputs\n','| Deliberately wrong case | Error / detection measure | Required detection | Result |\n|---|---:|---:|---|']
for r in neg:parts.append(f'| {r["name"]} | {float(r["error"]):.12g} | > {r["detection_threshold"]} | {r["status"]} |')
parts+=['\nThe incompatible sealed-wall input throws an explicit incompatibility error (its exact input and diagnostic are in `incompatible_input.txt`). Nine additional malformed-input checks reject zero/negative/nonfinite masses, nonfinite velocities/RHS/operator, a zero row, mismatched dimensions and zero body inertia. They are input-validation checks, not additional physical projection cases. No mean-RHS removal, pressure pin repair, mass floor or damping is used in the accepted solver. Wrong-mass, delayed-reaction, two-pin and missing-rotation calculations exist only in negative controls.\n','## Energy, extrema, timing and scope\n',f'Component velocities range from {min(float(r["velocity_min"]) for r in projection):.12g} to {max(float(r["velocity_max"]) for r in projection):.12g} m/s (angular components are rad/s). Cell pressures range from {min(float(r["pressure_min"]) for r in projection):.12g} to {max(float(r["pressure_max"]) for r in projection):.12g} Pa; values in sealed cases depend on the recorded SVD gauge. No pressure clamps were applied.\n',f'Adapter fraction extrema: [{min(float(r["min_fraction"]) for r in adapters):.17g}, {max(float(r["max_fraction"]) for r in adapters):.17g}], within the unchanged 256-epsilon gate, without repair.\n','Kinetic energies before/after, constraint work, dissipation and raw energy residuals are in every projection summary. Homogeneous cases satisfy the non-increase gate. Moving prescribed walls may supply work. This proves only the tested fixed-H projection identity, not full-timestep energy conservation.\n',f'Measured elapsed times: component fixtures {float(counts["runtime_s"]):.6f} s; six adapters {meta["runtime_adapters_seconds"]:.6f} s; original transport fixtures {meta["runtime_transport_seconds"]:.6f} s. Dense SVD plus heavy diagnostics is a correctness implementation, with no real-time performance claim.\n','## Source and evidence provenance\n','The supplied handoff and original archive remain unchanged. `prepare_p1pf_reference.py` records the independent Python reruns and exports seeded inputs only; C++ answers come from LAPACK and the reusable assembled operator. See `evidence/p1pf/reference_rerun/` for exact versions, input hashes and cross-platform differences.\n','Changes: prototype-only `frozen_projection.hpp`, `p1pf_validation.hpp`, `p1pf_adapters.hpp`, optional target dispatch in `main.cpp`/`CMakeLists.txt`, runner/recorder, method/evidence docs, seeded inputs and dependency notices. `record_p1cm.py` changes only generated scope wording. Fraction, mass and momentum algorithms and original fixtures/oracles are unchanged.\n','Current matching source snapshots and SHA-256 fingerprints are in `evidence/p1pf/current/source_snapshot/` and `RUN_METADATA.json`. Baseline sources/evidence remain under `evidence/p1pf/baseline/`. Full matrix entries, row/DOF/body records and all analytical comparisons accompany the summaries.\n',f'Git HEAD remains `{head}`. No commit, push or tag performed. All tracked changes are confined to the prototype. Captured status:\n\n```text\n{status.rstrip()}\n```\n','**Stop for operator review.** Frozen projection does not earn moving-solid transport, integrated buoyancy, P1-D/E, swimming, 3D or full R1.\n']
(P/'P1PF_RESULTS.md').write_text('\n'.join(parts))
print('P1-PF VERIFIED: 42 projection cases + 5 negative controls + 6 adapters; 55 transport runs preserved.')
