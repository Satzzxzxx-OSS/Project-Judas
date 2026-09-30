#!/usr/bin/env python3
"""Existing FTFT9 fixtures; operator brief acceptance and retained diagnostics."""
import argparse,hashlib,json,os,pathlib,subprocess,time
ROOT=pathlib.Path(__file__).resolve().parents[5]
parser=argparse.ArgumentParser();parser.add_argument('--output',default='focused');parser.add_argument('--half-only',action='store_true');args=parser.parse_args()
OUT=pathlib.Path(__file__).resolve().parent/args.output
OUT.mkdir(exist_ok=False)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def fingerprints():return {str(p.relative_to(ROOT)):sha(p) for d in ('src','tests') for p in sorted((ROOT/d).rglob('*')) if p.is_file() and p.suffix in ('.cpp','.h')}
record={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'source_before':fingerprints(),'runs':[]}
env=dict(os.environ,SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1')
def save():(OUT/'results.json').write_text(json.dumps(record,indent=2)+'\n')
def run(name,cmd):
 begin=time.monotonic();r=subprocess.run(cmd,cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 (OUT/(name+'.log')).write_text(r.stdout);record['runs'].append({'name':name,'command':cmd,'exit_code':r.returncode,'seconds':time.monotonic()-begin});save()
 print(name,'exit',r.returncode,r.stdout[-3500:],flush=True);return r.returncode
names=['judas_fluid_hydrostatics_tests','judas_production_fluid_tests','judas_player_fluid_integration_tests','judas_fluid_container_integration_tests','judas_production_fluid_performance']
build=run('build',['flock','build/ftft9-build.lock','cmake','--build','build','--target',*names,'-j4'])
if build:raise SystemExit(build)
for name in names:
 cmd=['./build/'+name]
 if name!='judas_fluid_hydrostatics_tests':cmd+=['--output',str(OUT/name)]
 if args.half_only and name=='judas_production_fluid_tests':cmd+=['--case','half_025']
 run(name,cmd)
record['source_after']=fingerprints();record['source_unchanged']=record['source_before']==record['source_after']
record['overall_pass']=len(record['runs'])==len(names)+1 and all(r['exit_code']==0 for r in record['runs']) and record['source_unchanged'];save()
raise SystemExit(0 if record['overall_pass'] else 1)
