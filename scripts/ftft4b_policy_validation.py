#!/usr/bin/env python3
"""Reproduce FTFT4B stop-condition evidence. Success means measurement complete,
NOT a physical pass. The policy and existing timing/energy failures remain exit 1.
No historical evidence is overwritten. No new impact law is wired into Step.
"""
import argparse,hashlib,json,subprocess,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'docs/evidence/ftft4b/implementation'
TARGETS=['judas_rigid_contact_tests','judas_contact_geometry_tests','judas_contact_lifecycle_tests','judas_broadphase_tests','judas_contact_cache_tests','judas_contact_storage_tests','judas_physics_tests','judas_lifecycle_tests']
UNITS=['RigidBody','RigidBodyGravity','Contacts','ContactSolver','RadialTerrain','Broadphase','Narrowphase','PhysicsWorld']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=ROOT/'docs/evidence/ftft4b/policy-replay');a=parser.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);records=[]
 def run(name,cmd,allowed=(0,)):
  t=time.monotonic();p=subprocess.run([str(x) for x in cmd],cwd=ROOT,capture_output=True,text=True,timeout=600)
  (out/(name+'.log')).write_text(p.stdout+p.stderr);entry=dict(name=name,command=list(map(str,cmd)),exit_code=p.returncode,seconds=time.monotonic()-t);records.append(entry);(out/'commands.json').write_text(json.dumps(records,indent=2)+'\n')
  print(name,p.returncode,flush=True)
  if p.returncode not in allowed:raise RuntimeError(name+' execution/setup failure (not physical evidence)')
  return p.stdout
 run('configure',['cmake','-S','.','-B','build','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_CXX_FLAGS=-ffp-contract=off'])
 run('build',['cmake','--build','build','--target','judas_rigid_motion_tests','judas_poisson_policy_witness','judas_timing_defect_probe',*TARGETS,'-j4'])
 run('motion',['build/judas_rigid_motion_tests'])
 policy=run('policy',['build/judas_poisson_policy_witness'],(1,));rows=[json.loads(s) for s in policy.splitlines() if s.startswith('{')];assert rows[-1]['energy_failure']
 # Independent exact reference reads the preserved actual-engine inputs. Run a
 # copy beside this execution's output to avoid overwriting the original proof.
 (out/'policy-witness-final.jsonl').write_text(policy)
 (out/'exact_poisson_check.py').write_bytes((BASE/'exact_poisson_check.py').read_bytes())
 run('exact',[sys.executable,out/'exact_poisson_check.py'],(1,))
 run('original-energy-variants',['build/judas_timing_defect_probe','energy'],(1,))
 run('extended-temporal',['build/judas_timing_defect_probe','extended'])
 binary=ROOT/'build/ftft4b-policy-temporal';adapter=ROOT/'docs/evidence/ftft4b/baseline/adapters'
 flags=['/usr/bin/c++','-std=c++17','-O3','-DNDEBUG','-ffp-contract=off','-Isrc']
 run('compile-temporal',flags+[adapter/'world_probe.cpp',*['src/'+u+'.cpp' for u in UNITS],'-o',binary])
 trace=run('temporal',[binary]);(out/'temporal.csv').write_text(trace)
 for mode in ['geometry','full']:run('temporal-'+mode,[sys.executable,adapter/'check_world_trace.py',out/'temporal.csv','--mode',mode],(0,) if mode=='geometry' else (1,))
 original=ROOT/'build/ftft4b-policy-original'
 run('compile-original',flags+[ROOT/'docs/evidence/ftft4b/baseline/research/code/production_energy_witness.cpp',*['src/'+u+'.cpp' for u in UNITS],'-o',original])
 run('original',[original],(1,))
 for name in TARGETS:run(name,[ROOT/'build'/name])
 for flag in ['--player','--invalid']:run('geometry-'+flag[2:],['build/judas_contact_geometry_tests',flag])
 hashes={str(p.relative_to(ROOT)):sha(p) for folder in ['src','tests','scripts'] for p in sorted((ROOT/folder).rglob('*')) if p.is_file() and '__pycache__' not in p.parts}
 hashes['CMakeLists.txt']=sha(ROOT/'CMakeLists.txt')
 (out/'source-sha256.json').write_text(json.dumps(hashes,indent=2)+'\n')
 (out/'summary.json').write_text(json.dumps(dict(status='VERIFICATION_COMPLETE_METHOD_CONTRADICTION',physical_acceptance=False,policy_gain=rows[-1]['gain'],world_impact_repair_implemented=False,commands=len(records),source_fingerprints='source-sha256.json',not_run=['full application/editor/FTFT1-3 validation','full FTFT4A independent oracle','performance gate','full PLUS/SVD/friction/TOI integration','fluid prototypes']),indent=2)+'\n')
if __name__=='__main__':main()
