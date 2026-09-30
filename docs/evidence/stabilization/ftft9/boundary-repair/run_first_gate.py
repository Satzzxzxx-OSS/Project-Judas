#!/usr/bin/env python3
"""Only the operator's three requested boundary-repair checks; no new fixtures."""
import argparse,hashlib,json,math,os,pathlib,re,subprocess,time
ROOT=pathlib.Path(__file__).resolve().parents[5]
parser=argparse.ArgumentParser();parser.add_argument('--output',default='first-gate');args=parser.parse_args()
OUT=pathlib.Path(__file__).resolve().parent/args.output
OUT.mkdir(exist_ok=False)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def fingerprints():
 return {str(p.relative_to(ROOT)):sha(p) for directory in ('src','tests') for p in sorted((ROOT/directory).rglob('*')) if p.is_file() and p.suffix in ('.cpp','.h')}
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()
record={'head':git('rev-parse','HEAD'),'git_status_before':git('status','--short'),'source_before':fingerprints(),'runs':[]}
env=dict(os.environ,SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1')
def run(name,command):
 start=time.monotonic();result=subprocess.run(command,cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 (OUT/(name+'.log')).write_text(result.stdout)
 record['runs'].append({'name':name,'command':command,'exit_code':result.returncode,'seconds':time.monotonic()-start})
 (OUT/'results.json').write_text(json.dumps(record,indent=2)+'\n')
 print(name,result.returncode,result.stdout[-2200:],flush=True)
 return result
build=run('build',['flock','build/ftft9-build.lock','cmake','--build','build','--target','judas_production_fluid_tests','judas_fluid_rigid_coupling_tests','-j4'])
if build.returncode:raise SystemExit(build.returncode)
witness=ROOT/'docs/evidence/stabilization/ftft9/brief-resume1/coarse_gap_witness.cpp'
compile_result=run('compile-witness',['/usr/bin/c++','-std=gnu++17','-ffp-contract=off','-O3','-DNDEBUG','-Wall','-Wextra','-Isrc',str(witness),'build/libjudas_engine.a','/usr/lib/x86_64-linux-gnu/libSDL2.so','build/libjudas_glad.a','-o','build/ftft9_coarse_gap_witness'])
if compile_result.returncode:raise SystemExit(compile_result.returncode)
record['unchanged_witness_sha256']=sha(witness)
gap=run('closing-gap',['./build/ftft9_coarse_gap_witness'])
values=re.search(r'final=([^ ]+) velocity=([^ ]+).*particles=(\d+)',gap.stdout)
record['gap_additional_observation']=bool(values and int(values[3])==1 and all(math.isfinite(float(v)) for k in (1,2) for v in values[k].split(',')) and max(abs(float(v)) for v in values[2].split(','))<=10.81)
half=run('half_025',['./build/judas_production_fluid_tests','--case','half_025','--output',str(OUT/'half_025')])
focused=run('focused-coupling',['./build/judas_fluid_rigid_coupling_tests'])
record['source_after']=fingerprints();record['source_unchanged_during_execution']=record['source_before']==record['source_after']
record['git_status_after']=git('status','--short')
record['overall_pass']=all(r['exit_code']==0 for r in record['runs']) and record['gap_additional_observation'] and record['source_unchanged_during_execution']
(OUT/'results.json').write_text(json.dumps(record,indent=2)+'\n')
raise SystemExit(0 if record['overall_pass'] else 1)
