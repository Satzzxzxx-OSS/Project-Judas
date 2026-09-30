#!/usr/bin/env python3
"""Select the two unchanged integration fixtures; do not replace their engine path."""
import hashlib,json,os,pathlib,subprocess,time
ROOT=pathlib.Path(__file__).resolve().parents[5];OUT=pathlib.Path(__file__).resolve().parent/'focused'
OUT.mkdir(exist_ok=False)
def hashes():return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for d in ('src','tests') for p in sorted((ROOT/d).rglob('*')) if p.suffix in ('.cpp','.h')}
r={'source_before':hashes(),'runs':[]};env=dict(os.environ,SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1')
for name,cmd in [('build',['flock','build/ftft9-build.lock','cmake','--build','build','--target','judas_player_fluid_integration_tests','judas_fluid_hydrostatics_tests','-j4']),('sampler',['./build/judas_fluid_hydrostatics_tests']),('player-fall',['./build/judas_player_fluid_integration_tests','--fall-only','--output',str(OUT/'fixtures')])]:
 start=time.monotonic();p=subprocess.run(cmd,cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/(name+'.log')).write_text(p.stdout);r['runs'].append({'name':name,'command':cmd,'exit_code':p.returncode,'seconds':time.monotonic()-start});print(name,p.returncode,p.stdout[-2400:],flush=True)
 if p.returncode:break
r['source_after']=hashes();r['source_unchanged']=r['source_after']==r['source_before'];r['pass']=len(r['runs'])==3 and all(x['exit_code']==0 for x in r['runs']) and r['source_unchanged'];(OUT/'results.json').write_text(json.dumps(r,indent=2)+'\n');raise SystemExit(0 if r['pass'] else 1)
