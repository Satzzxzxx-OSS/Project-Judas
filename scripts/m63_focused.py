#!/usr/bin/env python3
"""M63 real fixed-boundary, cold-process, editor, API and moved-export proof.
Raw M56 profiles stay in ignored scratch storage; no historical evidence is written.
"""
from pathlib import Path
import os, subprocess, sys, json, shutil, time, hashlib, importlib.util
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
def measurements(path):
 spec=importlib.util.spec_from_file_location('measurements',ROOT/'scripts/m62_focused.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 result=module.summarize(path)
 data=json.loads(path.read_text());values={}
 for f in data['frames']:
  if f['id']<=40:continue
  totals={}
  for s in f['scopes']:
   if 'Fracture' in s['name']:totals[s['name']]=totals.get(s['name'],0)+s['inclusive_ns']/1e6
  for n,v in totals.items():values.setdefault(n,[]).append(v)
 result['retained_frame_max_ms']=result.pop('startup_inclusive_max_ms')
 startup=data.get('startup') or {};result['startup_ms']=(startup.get('end_ns',0)-startup.get('start_ns',0))/1e6
 result['fracture_scopes']={n:{'samples':len(v),'median_ms':sorted(v)[len(v)//2],'max_ms':max(v)} for n,v in values.items()}
 return result

def main():
 out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=False);scratch=ROOT/'.cache'/('m63-'+out.name);assert not scratch.exists();scratch.mkdir(parents=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none',XDG_DATA_HOME=str(scratch/'save-data'))
 result={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'commands':[],'human_review':'PENDING','profiles':{}}
 def save():(out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 def run(name,cmd,extra=None,cwd=ROOT):
  start=time.monotonic();p=subprocess.run(list(map(str,cmd)),cwd=cwd,env=dict(env,**(extra or {})),capture_output=True,text=True,timeout=240)
  text=p.stdout+p.stderr;(out/(name+'.log')).write_text(text);result['commands'].append({'name':name,'exit_code':p.returncode,'seconds':time.monotonic()-start,'command':list(map(str,cmd)),'cwd':str(cwd)});save();assert p.returncode==0,name;return text
 project=ROOT/'projects/fracture_lab/fracture_lab.judasproj'
 if '--remaining' not in sys.argv and '--authoring' not in sys.argv:run('core',[ROOT/'build/judas_fracture_tests'])
 for mode in (() if '--authoring' in sys.argv else ('game',) if '--remaining' in sys.argv else ('mechanics','support','probe','radial','write','read','stream','lifetime','game')):
  target=scratch/mode;run(mode,[ROOT/'build/judas_fracture_application_tests',project,mode,target]);result['profiles'][mode]=measurements(target/'profile.json');save()
  if (target/'lab.png').exists():shutil.copy2(target/'lab.png',out/(mode+'.png'))
 if '--authoring' not in sys.argv and '--remaining' not in sys.argv:
  target=scratch/'far-origin';run('far-origin',[ROOT/'build/judas_fracture_application_tests',project,'probe',target],{'JUDAS_WORLD_OFFSET':'1000000000000,2000000000000,-3000000000000'});result['profiles']['far-origin']=measurements(target/'profile.json');save()
 run('cookbook',[ROOT/'build/judasjs_examples_tests',out/'cookbook'])
 result['api']=json.loads(run('api',['/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node',ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m63-typescript/package/lib/typescript.js','--runtime',out/'cookbook/surface.json']));save()
 fixture=scratch/'editor';shutil.copytree(ROOT/'projects/fracture_lab',fixture)
 text=run('editor',[ROOT/'build/judas_editor',fixture/'fracture_lab.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(out/'editor'),'JUDAS_EDITOR_AUTOTEST_FRACTURE':'1'})
 assert 'fracture rigid/soft/partition authoring: PASS / PASS / PASS' in text and 'authored scene after play/stop is IDENTICAL' in text
 package=scratch/'export';run('export',[ROOT/'build/judas_export',project,package]);moved=Path('/tmp')/('Judas_M63_'+out.name);assert not moved.exists();shutil.move(str(package),moved)
 result['package']={'moved':str(moved),'bytes':sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),'assets':len(list(moved.rglob('*.judasmeta'))),'hashes':{str(p.relative_to(moved)):hashlib.sha256(p.read_bytes()).hexdigest() for p in moved.rglob('*') if p.is_file()}}
 for mode in ('probe','lifetime','game'):
  target=scratch/('moved-'+mode);run('moved-'+mode,[ROOT/'build/judas_fracture_application_tests',moved/'game.judasproj',mode,target],cwd=Path('/tmp'));result['profiles']['moved-'+mode]=measurements(target/'profile.json');save()
 result['pass']=True;save();print(json.dumps({'pass':True,'commands':len(result['commands']),'package':str(moved)},indent=2))
if __name__=='__main__':main()
