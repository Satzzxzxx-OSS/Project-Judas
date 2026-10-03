#!/usr/bin/env python3
"""One clean candidate build/gate, then current project editor/export smoke.
Reuses unchanged production assertions; relocates their output into new evidence.
"""
import importlib.util,json,os,shutil,subprocess,sys,time,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/fluid_demo_refresh/final'
def main():
 assert not OUT.exists(),'Never overwrite earlier evidence'
 backup=ROOT/'.cache/fluid-demo-before-clean-release'
 if '--resume-build' in sys.argv:
  assert backup.is_dir() and (ROOT/'build').is_dir(),'Resume only the interrupted clean candidate build'
 else:
  assert not backup.exists();(ROOT/'.cache').mkdir(exist_ok=True)
  if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True);start=time.monotonic()
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests','judas_package_application_tests','judas_classification_tests','judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests','judas_physics_cast_application_tests','judas_joint_application_tests','judas_skeletal_animation_tests','judas_animation_application_tests','judas_pose_composition_application_tests','judas_ragdoll_tests','judas_ragdoll_application_tests','judas_fluid_demo_integration_tests'})
 original=subprocess.run
 def release(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command.append('-DCMAKE_BUILD_TYPE=Release')
  return original(command,*args,**kwargs)
 subprocess.run=release
 try:regression=gate.production(OUT,4)
 finally:subprocess.run=original
 results={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'clean_release':True,'regression':regression,'commands':[]}
 def save():(OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
 save()
 if not regression['overall_pass']:return 1
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 def run(name,command,extra=None,cwd=ROOT):
  before=time.monotonic();p=original(command,cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=180)
  (OUT/(name+'.log')).write_text(p.stdout);results['commands'].append({'name':name,'command':command,'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-before});save();assert p.returncode==0,name+' failed';return p.stdout
 fixture=ROOT/'build/fluid-demo-editor-project';shutil.copytree(ROOT/'projects/fluid_demo',fixture)
 text=run('editor',['./build/judas_editor',str(fixture/'fluid_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')})
 assert 'authored scene after play/stop is IDENTICAL' in text
 # A copied project's JS exercises the real public transition/reload API;
 # current authored demo and historical evidence are never edited by automation.
 fixture=ROOT/'build/fluid-demo-export-project';shutil.copytree(ROOT/'projects/fluid_demo',fixture)
 script=fixture/'Assets/scripts/fluid.js';source=script.read_text().replace('{ui,input,scenes}','{ui,input,scenes,session}')
 source=source.replace('uiUpdate(){',"""uiUpdate(){this.frames=(this.frames||0)+1;if(this.frames===12){
 const stage=session.get('smokeStage')||0;
 if(stage===0){session.set('smokeStage',1);console.log('FLUID_SCENE_POOL_READY');scenes.load('Scenes/planet.judas');}
 else if(stage===1){if(scenes.current!=='Scenes/planet.judas')throw Error('planet transition');session.set('smokeStage',2);console.log('FLUID_SCENE_PLANET_READY');scenes.reload();}
 else if(stage===2){if(scenes.current!=='Scenes/planet.judas')throw Error('planet reload');session.set('smokeStage',3);console.log('FLUID_SCENE_PLANET_RELOADED');scenes.load('Scenes/pool.judas');}
 else {if(scenes.current!=='Scenes/pool.judas')throw Error('pool return');console.log('FLUID_SCENE_POOL_RETURNED');ui.quit();}}
 """)
 script.write_text(source)
 # First normal standalone application, then same project relocated in a package.
 text=run('standalone',['./build/judas',str(fixture/'fluid_demo.judasproj')])
 markers=['FLUID_SCENE_POOL_READY','FLUID_SCENE_PLANET_READY','FLUID_SCENE_PLANET_RELOADED','FLUID_SCENE_POOL_RETURNED']
 assert all(m in text for m in markers)
 package=ROOT/'.cache/fluid-demo-export-package';moved=Path('/tmp/Judas_Production_Fluid_Package');assert not package.exists() and not moved.exists()
 run('export',['./build/judas_export',str(fixture/'fluid_demo.judasproj'),str(package)])
 shutil.move(str(package),str(moved));text=run('moved_package',[str(moved/'judas')],cwd=Path('/tmp'));assert all(m in text for m in markers)
 results.update(seconds=time.monotonic()-start,package_bytes=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),human_validation='PENDING; automated movement and state checks are not operator visual/interactive acceptance')
 protected=['prototypes']
 protected += [str(p.relative_to(ROOT)) for p in (ROOT/'docs/evidence').iterdir() if p.is_dir() and p.name!='fluid_demo_refresh']
 results['protected_unchanged']=not subprocess.check_output(['git','status','--porcelain','--',*protected],cwd=ROOT,text=True).strip()
 paths=subprocess.check_output(['git','diff','--name-only'],cwd=ROOT,text=True).splitlines()+subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
 sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(set(paths)) if (ROOT/p).is_file() and (p.startswith(('src/','tests/','scripts/','tools/','projects/fluid_demo/')) or p=='CMakeLists.txt')}
 (OUT/'SOURCE_SHA256.json').write_text(json.dumps(sources,indent=2)+'\n');save();print(json.dumps(results,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
