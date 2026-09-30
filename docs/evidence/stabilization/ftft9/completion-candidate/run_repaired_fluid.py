#!/usr/bin/env python3
"""Rerun only FluidWorld-dependent regressions after the wall-crossing anchor fix."""
import hashlib,json,os,pathlib,subprocess,time
ROOT=pathlib.Path(__file__).resolve().parents[5];OUT=pathlib.Path(__file__).resolve().parent/'repaired-fluid';OUT.mkdir(exist_ok=False)
def hashes():return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for d in ('src','tests') for p in sorted((ROOT/d).rglob('*')) if p.suffix in ('.cpp','.h')}
r={'source_before':hashes(),'runs':[],'scope':'Affected fluid targets only; the one broad run is preserved separately.'};env=dict(os.environ,SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1')
def run(name,cmd):
 start=time.monotonic();p=subprocess.run(cmd,cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/(name+'.log')).write_text(p.stdout);r['runs'].append({'name':name,'command':cmd,'exit_code':p.returncode,'seconds':time.monotonic()-start});(OUT/'results.json').write_text(json.dumps(r,indent=2)+'\n');print(name,p.returncode,p.stdout[-1900:],flush=True);return p.returncode
names=['judas_fluid_boundary_contact_tests','judas_fluid_world_tests','judas_fluid_rigid_coupling_tests','judas_terrain_fluid_tests','judas_fluid_cavity_impact_tests','judas_fluid_smoothing_budget_tests','judas_fluid_container_integration_tests','judas_production_fluid_tests','judas_player_fluid_integration_tests']
if run('build',['flock','build/ftft9-build.lock','cmake','--build','build','--target',*names,'judas_production_fluid_performance','-j4']):raise SystemExit(1)
witness=ROOT/'docs/evidence/stabilization/ftft9/brief-resume1/coarse_gap_witness.cpp';r['unchanged_witness_sha256']=hashlib.sha256(witness.read_bytes()).hexdigest()
if run('compile-witness',['/usr/bin/c++','-std=gnu++17','-ffp-contract=off','-O3','-DNDEBUG','-Wall','-Wextra','-Isrc',str(witness),'build/libjudas_engine.a','/usr/lib/x86_64-linux-gnu/libSDL2.so','build/libjudas_glad.a','-o','build/ftft9_coarse_gap_witness']):raise SystemExit(1)
if run('closing-gap',['./build/ftft9_coarse_gap_witness']):raise SystemExit(1)
for name in names:
 cmd=['./build/'+name]
 if name in ('judas_fluid_container_integration_tests','judas_production_fluid_tests','judas_player_fluid_integration_tests'):cmd+=['--output',str(OUT/name)]
 if run(name,cmd):break
r['source_after']=hashes();r['source_unchanged']=r['source_before']==r['source_after'];r['pass']=len(r['runs'])==len(names)+3 and all(x['exit_code']==0 for x in r['runs']) and r['source_unchanged'];(OUT/'results.json').write_text(json.dumps(r,indent=2)+'\n');raise SystemExit(0 if r['pass'] else 1)
