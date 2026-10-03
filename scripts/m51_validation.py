#!/usr/bin/env python3
"""M51: single clean Release/production gate, public API and ordinary project smokes."""
import importlib.util,json,os,shutil,subprocess,sys,time,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m51/final'
def main():
 assert not OUT.exists(),'Preserve previous evidence'
 backup=ROOT/'.cache/m51-before-clean-release';assert not backup.exists()
 (ROOT/'.cache').mkdir(exist_ok=True)
 if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True);start=time.monotonic()
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests','judas_package_application_tests','judas_classification_tests','judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests','judas_physics_cast_application_tests','judas_joint_application_tests','judas_skeletal_animation_tests','judas_animation_application_tests','judas_pose_composition_application_tests','judas_ragdoll_tests','judas_ragdoll_application_tests','judas_fluid_demo_integration_tests','judas_character_motor_application_tests'})
 original=subprocess.run
 def release(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command.append('-DCMAKE_BUILD_TYPE=Release')
  return original(command,*args,**kwargs)
 subprocess.run=release
 try:regression=gate.production(OUT,4)
 finally:subprocess.run=original
 result={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'clean_release':True,'regression':regression,'commands':[]}
 def save():(OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 save()
 if not regression['overall_pass']:return 1
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 def run(name,command,extra=None,cwd=ROOT):
  begin=time.monotonic();p=original(command,cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=240)
  (OUT/(name+'.log')).write_text(p.stdout);result['commands'].append({'name':name,'command':command,'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-begin});save();assert p.returncode==0,name+' failed';return p.stdout
 run('examples',[str(ROOT/'build/judasjs_examples_tests'),str(ROOT/'build/m51-examples')])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 result['api']=json.loads(run('api',[node,str(ROOT/'scripts/check_judasjs_api.mjs'),'--typescript','/tmp/judas-m51-typescript/package/lib/typescript.js','--runtime',str(ROOT/'build/m51-examples/surface.json')]))
 fixture=ROOT/'build/m51-editor-project';shutil.copytree(ROOT/'projects/boundary_demo',fixture)
 text=run('editor',[str(ROOT/'build/judas_editor'),str(fixture/'boundary_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')});assert 'authored scene after play/stop is IDENTICAL' in text
 steps=OUT/'demo_steps.txt';steps.write_text('STEPS 3\nLOG_EVERY 1\nSCREENSHOT 2 '+str(OUT/'demo.png')+'\n')
 text=run('standalone',[str(ROOT/'build/judas'),str(ROOT/'projects/boundary_demo/boundary_demo.judasproj')],{'JUDAS_TEST_SCRIPT':str(steps)});assert 'script asset=' not in text
 # Disposable ordinary project JS performs the real queued outer-loop transitions.
 fixture=ROOT/'build/m51-export-project';shutil.copytree(ROOT/'projects/boundary_demo',fixture)
 script=fixture/'Assets/scripts/controller.js';source=script.read_text().replace('{input,ui,world,scenes}','{input,ui,world,scenes,session,console}')
 needle=' uiUpdate(){';assert needle in source
 source=source.replace(needle,''' uiUpdate(){if(this.props.controlled&&this.elapsed>.08&&!this.smokeDone){this.smokeDone=true;
 const stage=session.get('m51Smoke')||0;
 console.log('M51_RUNTIME scene='+scenes.current+' capture='+input.pointerCapture+' valid='+this.entity.valid);
 if(stage===0){const t=this.entity.transform;t.position.x+=2;const spawned=world.spawnPrefab('49494949494949494949494949494903',t);if(!spawned.character)throw Error('motor prefab');spawned.destroy();session.set('m51Smoke',1);scenes.load('Scenes/flight.judas');}
 else if(stage===1){if(scenes.current!=='Scenes/flight.judas')throw Error('flight');session.set('m51Smoke',2);scenes.load('Scenes/planet.judas');}
 else if(stage===2){if(scenes.current!=='Scenes/planet.judas')throw Error('planet');session.set('m51Smoke',3);scenes.load('Scenes/pool.judas');}
 else if(stage===3){if(scenes.current!=='Scenes/pool.judas')throw Error('pool');session.set('m51Smoke',4);scenes.reload();}
 else if(stage===4){if(scenes.current!=='Scenes/pool.judas')throw Error('reload');session.set('m51Smoke',5);scenes.load('Scenes/flat.judas');}
 else {if(scenes.current!=='Scenes/flat.judas')throw Error('return');console.log('M51_TRANSITIONS_RELOAD_EXPORT_PASS');ui.quit();}}
 ''',1);script.write_text(source)
 text=run('transitions',[str(ROOT/'build/judas'),str(fixture/'boundary_demo.judasproj')]);assert 'M51_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 package=ROOT/'.cache/m51-export-package';moved=Path('/tmp/Judas_M51_Moved_Package');assert not package.exists() and not moved.exists()
 run('export',[str(ROOT/'build/judas_export'),str(fixture/'boundary_demo.judasproj'),str(package)])
 shutil.move(str(package),str(moved));text=run('moved_package',[str(moved/'judas')],cwd=Path('/tmp'));assert 'M51_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 result.update(seconds=time.monotonic()-start,package_bytes=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),human_validation='PENDING; operator authority')
 protected=['prototypes','projects/fluid_demo']+[str(p.relative_to(ROOT)) for p in (ROOT/'docs/evidence').iterdir() if p.is_dir() and p.name!='m51']
 result['protected_unchanged']=not subprocess.check_output(['git','status','--porcelain','--',*protected],cwd=ROOT,text=True).strip()
 paths=subprocess.check_output(['git','diff','--name-only'],cwd=ROOT,text=True).splitlines()+subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
 sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(set(paths)) if (ROOT/p).is_file() and (p.startswith(('src/','tests/','scripts/','tools/','projects/boundary_demo/')) or p=='CMakeLists.txt')}
 (OUT/'SOURCE_SHA256.json').write_text(json.dumps(sources,indent=2)+'\n');save();print(json.dumps(result,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
