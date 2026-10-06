#!/usr/bin/env python3
"""Focused final M61 proof. Every save pair is two independent processes.
No operator window/process control; all state is isolated under .cache.
"""
import os,subprocess,json,time,shutil,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=False)
 env=dict({k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')},SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none',XDG_DATA_HOME=str(ROOT/'.cache/m61-final-data'))
 records=[]
 def run(name,args,profile=True,cwd=ROOT):
  local=dict(env)
  if profile:local.update(JUDAS_PROFILE='1',JUDAS_PROFILE_OUTPUT=str(out/(name+'-profile.json')))
  else:local.pop('JUDAS_PROFILE',None);local.pop('JUDAS_PROFILE_OUTPUT',None)
  begin=time.monotonic();p=subprocess.run([str(x) for x in args],cwd=cwd,env=local,capture_output=True,text=True,timeout=240)
  (out/(name+'.log')).write_text(p.stdout+p.stderr);records.append({'name':name,'exit_code':p.returncode,'seconds':time.monotonic()-begin,'command':[str(x) for x in args]});(out/'COMMANDS.json').write_text(json.dumps(records,indent=2)+'\n');assert p.returncode==0,name
 if '--tail' not in sys.argv and '--editor-stream' not in sys.argv:
  run('storage',[ROOT/'build/judas_save_storage_tests',ROOT/'.cache/m61-storage-final'])
  run('world',[ROOT/'build/judas_world_persistence_tests'])
  for label,target,project in [('lab','judas_save_application_tests','save_lab'),('stream','judas_save_streaming_tests','streamed_range')]:
   for mode in ['write','read']+(['read-b'] if label=='stream' else []):
    run(label+'-'+mode,[ROOT/'build'/target,ROOT/'projects'/project/(project+'.judasproj'),mode,out/(label+'-'+mode)])
  run('cookbook',[ROOT/'build/judasjs_examples_tests',out/'cookbook'])
  node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
  run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m58-typescript/package/lib/typescript.js','--runtime',out/'cookbook/surface.json'],False)
 if '--editor-stream' not in sys.argv:
  # A minimal ordinary project has no scripts/liquids/streamed-world costs.
  simple=ROOT/'.cache/m61-simple';(simple/'Scenes').mkdir(parents=True,exist_ok=True);(simple/'Assets').mkdir(exist_ok=True)
  (simple/'simple.judasproj').write_text('JudasProject 1\nname "M61 simple measurement"\nstartup-scene "Scenes/main.judas"\nassets-dir "Assets"\nscenes-dir "Scenes"\nsaves-dir "Saves"\nlegacy-gameplay "false"\n')
  (simple/'Scenes/main.judas').write_text('JudasScene 3\nsettings\n name "M61 simple"\n world-origin 0 0 0\n sun-direction 0 1 0\n sun-color 1 1 1\n ambient 0.4 0.4 0.4\n fluid-scale 13\n fidelity-policy none\n next-id 3\nend\nobject 1 "body"\n position 0 1 0\n rotation 1 0 0 0\n scale 1 1 1\n body dynamic box\n body.half-extents 0.5 0.5 0.5\n body.radius 0.5\n body.terrain ""\n body.mass 1\n body.friction 0.6\n body.restitution 0.05\n body.initial-velocity 0 0 0\n body.pickable false\n body.managed false\n body.compound-count 0\n body.fluid-cavity-count 0\nend\n')
  for label,project in [('simple',simple/'simple.judasproj'),('lab',ROOT/'projects/save_lab/save_lab.judasproj'),('stream',ROOT/'projects/streamed_range/streamed_range.judasproj')]:
   run('performance-'+label,[ROOT/'build/judas_save_performance_tests',project,out/('performance-'+label)])
 # Normal editor Play/Stop, isolated copies, separate EditorSlots namespace.
 for label in (['streamed_range'] if '--editor-stream' in sys.argv else ['save_lab','streamed_range']):
  fixture=ROOT/'.cache'/('m61-editor-'+label);assert not fixture.exists();shutil.copytree(ROOT/'projects'/label,fixture)
  local=dict(env,JUDAS_EDITOR_AUTOTEST=str(out/('editor-'+label)))
  p=subprocess.run([str(ROOT/'build/judas_editor'),str(fixture/(label+'.judasproj'))],cwd=ROOT,env=local,capture_output=True,text=True,timeout=240)
  (out/('editor-'+label+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==0 and 'authored scene after play/stop is IDENTICAL' in p.stdout+p.stderr,label
 print(json.dumps({'pass':True,'commands':records},indent=2))
if __name__=='__main__':main()
