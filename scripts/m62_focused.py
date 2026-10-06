#!/usr/bin/env python3
"""Combined M62 public/editor/lifetime proof. Never writes historical evidence.
Raw M56 captures stay in the ignored working directory; compact timing summaries
and screenshots accompany the recorded commands. Cold restore uses two processes.
"""
from pathlib import Path
import os, subprocess, sys, json, shutil, time, hashlib
ROOT=Path(__file__).resolve().parents[1]
def summarize(path):
 data=json.loads(path.read_text());frames=data['frames'];warm=[f for f in frames if not f.get('startup') and f['id']>40]
 def stats(values):
  values=sorted(values)
  return {'samples':len(values),'median_ms':values[len(values)//2] if values else 0,'p95_ms':values[min(len(values)-1,int(len(values)*.95))] if values else 0,'maximum_ms':max(values,default=0)}
 phases={}
 for f in warm:
  summed={}
  for s in f['scopes']:summed[s['name']]=summed.get(s['name'],0)+s['inclusive_ns']/1e6
  for name,value in summed.items():
   if 'Deformable' in name or name in ('World deformables','Render frame','Scripts fixedUpdate'):phases.setdefault(name,[]).append(value)
 return {'raw_capture':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'frame':stats([(f['end_ns']-f['start_ns'])/1e6 for f in warm]),'fixed':stats([(step['end_ns']-step['start_ns'])/1e6 for f in warm for step in f['fixed']['steps']]),'phases':{n:stats(v) for n,v in phases.items()},'rss_bytes_max':max([f['memory']['linux_resident_bytes'] for f in frames],default=0),'diagnostics':data.get('diagnostics'), 'startup_inclusive_max_ms':max([(f['end_ns']-f['start_ns'])/1e6 for f in frames],default=0),'graphics':'offscreen software GL unless explicitly native'}
def main():
 out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=False);scratch=ROOT/'.cache'/('m62-'+out.name);assert not scratch.exists();scratch.mkdir()
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none',XDG_DATA_HOME=str(scratch/'save-data'))
 records=[];result={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'commands':records,'human_review':'PENDING','profiles':{}}
 def save():(out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 def run(name,cmd,extra=None,cwd=ROOT):
  begin=time.monotonic();p=subprocess.run(list(map(str,cmd)),cwd=cwd,env=dict(env,**(extra or {})),capture_output=True,text=True,timeout=240)
  text=p.stdout+p.stderr;(out/(name+'.log')).write_text(text);records.append({'name':name,'exit_code':p.returncode,'seconds':time.monotonic()-begin,'command':list(map(str,cmd)),'cwd':str(cwd)});save();assert p.returncode==0,name;return text
 run('core',[ROOT/'build/judas_deformable_tests'])
 if '--skip-performance' not in sys.argv:
  run('performance',[ROOT/'build/judas_deformable_performance_tests'])
 project=ROOT/'projects/deformable_lab/deformable_lab.judasproj'
 for mode in ('probe','write','read','stream','lifetime'):
  target=scratch/mode;run(mode,[ROOT/'build/judas_deformable_application_tests',project,mode,target]);result['profiles'][mode]=summarize(target/'profile.json');save()
  if (target/'lab.png').exists():shutil.copy2(target/'lab.png',out/(mode+'.png'))
 run('cookbook',[ROOT/'build/judasjs_examples_tests',out/'cookbook'])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m58-typescript/package/lib/typescript.js','--runtime',out/'cookbook/surface.json']));save()
 fixture=scratch/'editor';shutil.copytree(ROOT/'projects/deformable_lab',fixture)
 source=fixture/'Assets/models/seam-sheet.obj';source.write_text('v -1 0 0\nv 1 0 0\nv 1 2 0\nv -1 2 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nf 1/1 2/2 3/3\nf 1/4 3/2 4/1\n')
 source.with_suffix('.obj.judasmeta').write_text('JudasAssetMeta 1\nid "62626262626262626262626262629996"\ntype mesh\nsource ""\n')
 text=run('editor-authoring',[ROOT/'build/judas_editor',fixture/'deformable_lab.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(out/'editor'),'JUDAS_EDITOR_AUTOTEST_DEFORMABLE':'1','JUDAS_EDITOR_AUTOTEST_DEFORMABLE_SOURCE':'62626262626262626262626262629996'})
 assert 'sheet/block authoring: PASS / PASS' in text and 'indexed import authoring: PASS' in text and 'authored scene after play/stop is IDENTICAL' in text
 # Export the ordinary project, not the edited automation copy. An instrumented
 # host runs against a moved unmodified package; no assertion host ships with it.
 package=scratch/'export';text=run('export',[ROOT/'build/judas_export',project,package]);moved=Path('/tmp')/('Judas_M62_'+out.name);assert not moved.exists();shutil.move(str(package),moved)
 result['package']={'moved':str(moved),'bytes':sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),'assets':len(list(moved.rglob('*.judasmeta'))),'hashes':{str(p.relative_to(moved)):hashlib.sha256(p.read_bytes()).hexdigest() for p in moved.rglob('*') if p.is_file()}}
 for mode in ('probe','lifetime'):
  target=scratch/('moved-'+mode);run('moved-'+mode,[ROOT/'build/judas_deformable_application_tests',moved/'game.judasproj',mode,target],cwd=Path('/tmp'));result['profiles']['moved-'+mode]=summarize(target/'profile.json');save()
 result['pass']=True;save();print(json.dumps({'pass':True,'checks':len(records),'package':str(moved)},indent=2))
if __name__=='__main__':main()
