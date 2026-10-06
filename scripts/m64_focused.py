#!/usr/bin/env python3
"""M64 focused real-application, cold-process, editor and moved-package proof.
Only new evidence/scratch paths are written. Assertion hosts never ship.
"""
from pathlib import Path
import os,subprocess,sys,json,shutil,time,hashlib,importlib.util
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
def measurements(path):
 spec=importlib.util.spec_from_file_location('m56_summaries',ROOT/'scripts/m62_focused.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 result=m.summarize(path);result['graphics']='native X11 desktop GL, hidden assertion window'
 data=json.loads(path.read_text());values={}
 for frame in data['frames']:
  if frame['id']<=40:continue
  totals={}
  for scope in frame['scopes']:
   if scope['name'] in ('Rigid physics','Scripts fixedUpdate','Scripts update','Render frame'):
    totals[scope['name']]=totals.get(scope['name'],0)+scope['inclusive_ns']/1e6
  for key,value in totals.items():values.setdefault(key,[]).append(value)
 result['scopes']={n:{'samples':len(v),'median_ms':sorted(v)[len(v)//2],'p95_ms':sorted(v)[min(len(v)-1,int(len(v)*.95))],'maximum_ms':max(v)} for n,v in values.items()}
 return result

def main():
 out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=False);scratch=ROOT/'.cache'/('m64-'+out.name);assert not scratch.exists();scratch.mkdir(parents=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none',XDG_DATA_HOME=str(scratch/'save-data'))
 result={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'commands':[],'human_review':'PENDING','profiles':{}}
 def save():(out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 def run(name,cmd,extra=None,cwd=ROOT):
  begin=time.monotonic();p=subprocess.run(list(map(str,cmd)),cwd=cwd,env=dict(env,**(extra or {})),capture_output=True,text=True,timeout=240)
  text=p.stdout+p.stderr;(out/(name+'.log')).write_text(text);result['commands'].append({'name':name,'command':list(map(str,cmd)),'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-begin});save();assert p.returncode==0,name;return text
 project=ROOT/'projects/collision_lab/collision_lab.judasproj';local=project.with_name('collision_lab_local.judasproj')
 run('geometry',[ROOT/'build/judas_collision_geometry_tests'])
 for mode in ('probe','stream','write','read','skate-mesh-on','skate-mesh-off','skate-boxes-on','skate-boxes-off'):
  capture=scratch/mode;run(mode,[ROOT/'build/judas_collision_application_tests',local if mode in ('write','read') else project,mode,capture]);result['profiles'][mode]=measurements(capture/'profile.json');save()
  if (capture/'lab.png').exists():shutil.copy2(capture/'lab.png',out/(mode+'.png'))
 capture=scratch/'far-origin';run('far-origin',[ROOT/'build/judas_collision_application_tests',project,'probe',capture],{'JUDAS_WORLD_OFFSET':'1000000000000,2000000000000,-3000000000000'});result['profiles']['far-origin']=measurements(capture/'profile.json');save()
 run('cookbook',[ROOT/'build/judasjs_examples_tests',out/'cookbook'])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m63-typescript/package/lib/typescript.js','--runtime',out/'cookbook/surface.json']));save()
 fixture=scratch/'editor';shutil.copytree(ROOT/'projects/collision_lab',fixture)
 text=run('editor',[ROOT/'build/judas_editor',fixture/'collision_lab.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(out/'editor'),'JUDAS_EDITOR_AUTOTEST_COLLISION':'1'})
 assert 'shared collision cook/assign: PASS' in text and 'authored scene after play/stop is IDENTICAL' in text
 package=scratch/'export';run('export',[ROOT/'build/judas_export',project,package]);moved=Path('/tmp')/('Judas_M64_'+out.name);assert not moved.exists();shutil.move(str(package),moved)
 result['package']={'moved':str(moved),'bytes':sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),'assets':len(list(moved.rglob('*.judasmeta'))),'hashes':{str(p.relative_to(moved)):hashlib.sha256(p.read_bytes()).hexdigest() for p in moved.rglob('*') if p.is_file()}}
 for mode in ('probe','stream'):
  capture=scratch/('moved-'+mode);run('moved-'+mode,[ROOT/'build/judas_collision_application_tests',moved/'game.judasproj',mode,capture],cwd=Path('/tmp'));result['profiles']['moved-'+mode]=measurements(capture/'profile.json');save()
 result['pass']=True;save();print(json.dumps({'pass':True,'commands':len(result['commands']),'moved_package':str(moved)},indent=2))
if __name__=='__main__':main()
