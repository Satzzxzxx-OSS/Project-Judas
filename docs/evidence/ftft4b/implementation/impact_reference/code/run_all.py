from pathlib import Path
import argparse,hashlib,json,os,platform,shutil,subprocess,sys,time
import numpy as np
import scipy
ROOT=Path(__file__).resolve().parents[1]

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--replay-rejected',action='store_true');args=ap.parse_args()
 (ROOT/'results').mkdir(exist_ok=True);(ROOT/'.build').mkdir(exist_ok=True)
 env=os.environ.copy();env.update(OPENBLAS_NUM_THREADS='1',OMP_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
 records=[]
 def call(cmd,name,cwd=ROOT):
  start=time.perf_counter();r=subprocess.run(cmd,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=90)
  (ROOT/'results'/f'{name}.stdout.txt').write_text(r.stdout);(ROOT/'results'/f'{name}.stderr.txt').write_text(r.stderr)
  records.append(dict(name=name,command=cmd,returncode=r.returncode,runtime_s=time.perf_counter()-start))
  if r.returncode:raise RuntimeError(f'{name}: exit {r.returncode}; inspect results')
  return r.stdout
 for mod in ['event_progress','normal_blocks','energetic_point','spatial_point','spatial_ode_oracle','coupled_spatial_experiment']:
  call([sys.executable,str(ROOT/'code'/f'{mod}.py')],mod)
 cc=shutil.which('c++') or shutil.which('g++')
 if not cc:raise RuntimeError('C++17 compiler required for scalar reference')
 binary=ROOT/'.build/scalar_friction_probe'
 call([cc,'-std=c++17','-O3','-ffp-contract=off',str(ROOT/'code/scalar_friction_probe.cpp'),'-o',str(binary)],'compile_scalar')
 text=call([str(binary)],'scalar_friction_probe');scalar=json.loads(text);(ROOT/'results/scalar_friction_probe.json').write_text(json.dumps(scalar,indent=2))
 for key in scalar:assert scalar[key]['energy_change']>.03
 e=json.loads((ROOT/'results/event_progress.json').read_text());n=json.loads((ROOT/'results/normal_blocks.json').read_text());p=json.loads((ROOT/'results/energetic_point.json').read_text());s=json.loads((ROOT/'results/spatial_point.json').read_text());o=json.loads((ROOT/'results/spatial_ode_oracle.json').read_text())
 assert e['cases']==360 and not e['unresolved'] and e['max_events']==272
 assert [x['events']for x in e['old_failures_resolved']]==[272,184,141]
 assert e['normal_witness']['K_before']==e['normal_witness']['K_after']
 assert n['cases']==600 and max(n['max_errors'].values())<1e-8
 assert p['cases']==2400 and not p['failed_cases'] and p['max_errors']['energy_identity']<1e-9
 assert s['spatial_cases']==48 and not s['failures'] and s['max_tangent_rotation_error']<1e-8
 assert s['max_planar_errors_by_tolerance'][-1]<s['max_planar_errors_by_tolerance'][0]
 assert o['cases']==12 and not o['failed'] and o['max_velocity_errors'][-1]<o['max_velocity_errors'][0]
 c=json.loads((ROOT/'results/coupled_spatial_experiment.json').read_text())
 assert c['cases']==12 and c['complete']==12 and not c['unresolved']
 rejected=[]
 if args.replay_rejected:
  for label in ['spatial_initial','midpoint_rechecked']:
   work=ROOT/'.build'/f'replay_{label}';(work/'code').mkdir(parents=True,exist_ok=True);(work/'results').mkdir(exist_ok=True)
   shutil.copy2(ROOT/'evidence'/label/'spatial_point.py',work/'code/spatial_point.py')
   shutil.copy2(ROOT/'code/energetic_point.py',work/'code/energetic_point.py')
   call([sys.executable,str(work/'code/spatial_point.py')],f'replay_{label}',work)
   r=json.loads((work/'results/spatial_point.json').read_text())
   assert r['failures'] and r['max_planar_errors_by_tolerance'][-1]>.007
   rejected.append(dict(candidate=label,failures=r['failures'],max_planar_errors=r['max_planar_errors_by_tolerance']))
   shutil.copy2(work/'results/spatial_point.json',ROOT/'results'/f'replayed_{label}.json')
 meta=dict(research_reproduced=True,production_acceptance=False,production_engine_run=False,
  explicit_open_results=dict(no_capture=e['without_capture'],normal_blocks_inducing_other_contacts=n['blocks_inducing_other_contacts'],general_multi_contact_friction='not implemented',general_termination='not proved',production_cost='not measured'),
  interpreter=sys.version,platform=platform.platform(),numpy=np.__version__,scipy=scipy.__version__,compiler=subprocess.check_output([cc,'--version'],text=True).splitlines()[0],records=records,
  code_hashes={str(f.relative_to(ROOT)):sha(f)for folder in ['code','reference']for f in (ROOT/folder).rglob('*')if f.is_file()and '__pycache__'not in f.parts},
  rejected_replays=rejected,total_measured_command_s=sum(r['runtime_s']for r in records))
 (ROOT/'results/RUN_METADATA.json').write_text(json.dumps(meta,indent=2));print(json.dumps({k:v for k,v in meta.items()if k not in ['records','code_hashes']},indent=2))
if __name__=='__main__':main()
