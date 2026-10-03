#!/usr/bin/env python3
"""One M53 clean Release/production gate, then focused API/editor/package checks.
Historical runners/assertions are reused with output-only adapters, never edited.
"""
import importlib.util,json,os,shutil,subprocess,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m53/final'
def main():
 assert not OUT.exists(),'Preserve previous evidence'
 backup=ROOT/'.cache/m53-before-clean-release';assert not backup.exists()
 (ROOT/'.cache').mkdir(exist_ok=True)
 if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True);start=time.monotonic()
 result={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'commands':[],'human_validation':'PENDING; operator authority'}
 def save():(OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests','judas_package_application_tests','judas_classification_tests','judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests','judas_physics_cast_application_tests','judas_joint_application_tests','judas_skeletal_animation_tests','judas_animation_application_tests','judas_pose_composition_application_tests','judas_ragdoll_tests','judas_ragdoll_application_tests','judas_fluid_demo_integration_tests','judas_character_motor_application_tests'})
 original=subprocess.run
 def release(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command=command+['-DCMAKE_BUILD_TYPE=Release']
  if isinstance(command,list) and command and Path(command[0]).name=='judas_shooter_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/shooter_game/shooter_game.judasproj'),str(OUT/'shooter-application')]
  return original(command,*args,**kwargs)
 subprocess.run=release
 try:result['regression']=gate.production(OUT,6)
 finally:subprocess.run=original
 save()
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 def run(name,command,extra=None,cwd=ROOT):
  command=[str(c) for c in command];begin=time.monotonic();p=original(command,cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=240)
  (OUT/(name+'.log')).write_text(p.stdout);result['commands'].append({'name':name,'command':command,'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-begin});save();assert p.returncode==0,name+' failed';return p.stdout
 # Production gate already executed navigation, motor, shooter and presentation regressions once.
 run('cookbook',[ROOT/'build/judasjs_examples_tests',ROOT/'build/m53-examples'])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m51-typescript/package/lib/typescript.js','--runtime',ROOT/'build/m53-examples/surface.json']))
 fixture=ROOT/'build/m53-editor-project';shutil.copytree(ROOT/'projects/shooter_game',fixture)
 run('bake',[ROOT/'build/judas_navigation_bake',fixture/'shooter_game.judasproj','Scenes/range.judas'])
 text=run('editor-bake-play-stop',[ROOT/'build/judas_editor',fixture/'shooter_game.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor'),'JUDAS_EDITOR_AUTOTEST_NAV_BAKE':'1'});assert 'authored scene after play/stop is IDENTICAL' in text and 'navigation bake 550: PASS' in text and 'navigation bake 551: PASS' in text and 'script asset=' not in text
 # Export the actual game, move it out of the checkout and launch from unrelated cwd.
 package=ROOT/'.cache/m53-package';moved=Path('/tmp/Judas_Spring_Range_M53');assert not package.exists() and not moved.exists()
 exportStart=time.monotonic();run('export',[ROOT/'build/judas_export',ROOT/'projects/shooter_game/shooter_game.judasproj',package]);result['export_seconds']=time.monotonic()-exportStart
 shutil.move(str(package),str(moved));result['package_path']=str(moved);result['package_bytes']=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file())
 steps=OUT/'package.steps';steps.write_text('STEPS 120\nLOG_EVERY 60\nSCREENSHOT 110 '+str(OUT/'game.png')+'\n')
 text=run('moved-package',[moved/'judas'],{'JUDAS_TEST_SCRIPT':str(steps)},Path('/tmp'));assert 'Scene: Spring Range' in text and 'script asset=' not in text and 'Asset problem:' not in text
 # Disposable registered-project script verifies real queued reload/replacement, safe handles,
 # nav assets and runtime-prefab cleanup. The shipped game is not modified by this probe.
 transition=ROOT/'build/m53-transition-project';shutil.copytree(ROOT/'projects/shooter_game',transition)
 shutil.copyfile(transition/'Scenes/range.judas',transition/'Scenes/alternate.judas')
 script=transition/'Assets/scripts/arena.js';source=script.read_text().replace('{input,world,ui}','{input,world,ui,navigation,scenes,session,console}').replace('constructor(){','constructor(){this.smokeTime=0;')
 source=source.replace('update(){','''update(dt){this.smokeTime+=dt;
 if(this.smokeTime>.15&&navigation.sample({x:0,y:0,z:-4})){
  if(navigation.errors.length)throw Error('navigation error');
  if(world.queryTags(['navigator']).length!==12)throw Error('stale prefab navigation');
  const path=navigation.path({x:34,y:0,z:-4},{x:0,y:0,z:-4});if(path.status!=='complete'||!path.corners.some(c=>c.link))throw Error('link route');
  const stage=session.get('m53Smoke')||0;console.log('M53_RUNTIME stage='+stage+' scene='+scenes.current);
  if(stage===0){const e=world.spawnPrefab('53535353535353535353535353535302',{position:{x:-20,y:1,z:20}});const a=e.navigation;e.destroy();if(e.valid)throw Error('stale entity');let throws=false;try{a.state;}catch(err){throws=true;}if(!throws)throw Error('stale navigation');session.set('m53Smoke',1);scenes.reload();}
  else if(stage===1){session.set('m53Smoke',2);scenes.load('Scenes/alternate.judas');}
  else if(stage===2){if(scenes.current!=='Scenes/alternate.judas')throw Error('scene replacement');session.set('m53Smoke',3);scenes.load('Scenes/range.judas');}
  else {console.log('M53_TRANSITIONS_RELOAD_EXPORT_PASS');ui.quit();}
  return;
 }
 ''',1);script.write_text(source)
 text=run('transitions',[ROOT/'build/judas',transition/'shooter_game.judasproj']);assert 'M53_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 transitionPackage=ROOT/'.cache/m53-transition-package';transitionMoved=Path('/tmp/Judas_M53_Transition_Proof');assert not transitionPackage.exists() and not transitionMoved.exists()
 run('export-transition-probe',[ROOT/'build/judas_export',transition/'shooter_game.judasproj',transitionPackage]);shutil.move(str(transitionPackage),str(transitionMoved))
 text=run('moved-transition-probe',[transitionMoved/'judas'],cwd=Path('/tmp'));assert 'M53_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 result['seconds']=time.monotonic()-start;result['overall_pass']=result['regression']['overall_pass'];save();print(json.dumps(result,indent=2));return 0 if result['overall_pass'] else 1
if __name__=='__main__':raise SystemExit(main())
