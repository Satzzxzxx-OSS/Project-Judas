#!/usr/bin/env python3
"""One M55 clean Release/production+async gate; current liquid project lifecycle/export.
Historical assertions are imported unchanged with output/configuration adapters.
"""
import importlib.util,json,os,shutil,subprocess,sys,time,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m55/final'
def main():
 resume='--resume-tail' in sys.argv
 if not resume:assert not OUT.exists(),'Preserve prior evidence'
 backup=ROOT/'.cache/m55-before-clean-release';
 if not resume:assert not backup.exists()
 backup.parent.mkdir(exist_ok=True)
 if not resume and (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True,exist_ok=resume);start=time.monotonic()
 result={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'commands':[],'human_validation':'PENDING; operator authority'}
 if resume:result=json.loads((OUT/'RESULTS.json').read_text())
 def save():(OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests','judas_package_application_tests','judas_classification_tests','judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests','judas_physics_cast_application_tests','judas_joint_application_tests','judas_skeletal_animation_tests','judas_animation_application_tests','judas_pose_composition_application_tests','judas_ragdoll_tests','judas_ragdoll_application_tests','judas_fluid_demo_integration_tests','judas_character_motor_application_tests'})
 original=subprocess.run
 def release(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command=command+['-DCMAKE_BUILD_TYPE=Release']
  if isinstance(command,list) and command and Path(command[0]).name=='judas_shooter_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/shooter_game/shooter_game.judasproj'),str(OUT/'shooter-application')]
  return original(command,*args,**kwargs)
 subprocess.run=release
 try:
  if not resume:result['regression']=gate.production(OUT,6)
 finally:subprocess.run=original
 save();assert result['regression']['overall_pass'],'production gate failed; inspect before any rerun'
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 def run(name,command,extra=None,cwd=ROOT,expected=0):
  command=[str(c) for c in command];begin=time.monotonic()
  try:p=original(command,cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=60)
  except subprocess.TimeoutExpired as failure:
   output=failure.stdout or b'';output=output.decode(errors='replace') if isinstance(output,bytes) else output
   (OUT/(name+'-timeout.log')).write_text(output);raise
  (OUT/(name+'.log')).write_text(p.stdout);result['commands'].append({'name':name,'command':command,'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-begin});save();assert p.returncode==expected,name+' failed';return p.stdout
 if not resume:
  # Both M54 and M55 suites ran once in production discovery.
  shutil.copytree(ROOT/'build/m55-visual',OUT/'visual')
  run('cookbook',[ROOT/'build/judasjs_examples_tests',ROOT/'build/m55-examples'])
  node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
  result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m51-typescript/package/lib/typescript.js','--runtime',ROOT/'build/m55-examples/surface.json']))
  fixture=ROOT/'build/m55-editor-project';shutil.copytree(ROOT/'projects/liquid_surface_demo',fixture)
  for scene in ('lab','radial'):run('bake-'+scene,[ROOT/'build/judas_liquid_bake',fixture/'liquid_surface_demo.judasproj','Scenes/'+scene+'.judas'])
  text=run('editor-bake-play-stop',[ROOT/'build/judas_editor',fixture/'liquid_surface_demo.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor'),'JUDAS_EDITOR_AUTOTEST_LIQUID_BAKE':'1'});assert 'authored scene after play/stop is IDENTICAL' in text and 'liquid bake 20: PASS' in text and 'script asset=' not in text
  package=ROOT/'.cache/m55-package';moved=Path('/tmp/Judas_Dynamic_Surface_M55');assert not package.exists() and not moved.exists()
  exportStart=time.monotonic();run('export',[ROOT/'build/judas_export',ROOT/'projects/liquid_surface_demo/liquid_surface_demo.judasproj',package]);result['export_seconds']=time.monotonic()-exportStart
  shutil.move(str(package),str(moved));result['package_path']=str(moved);result['package_bytes']=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file());result['packaged_assets']=len(list(moved.rglob('*.judasmeta')))
  steps=OUT/'package.steps';steps.write_text('STEPS 120\nLOG_EVERY 60\nSCREENSHOT 110 '+str(OUT/'package-world.png')+'\n')
  text=run('moved-package',[moved/'judas'],{'JUDAS_TEST_SCRIPT':str(steps)},Path('/tmp'));assert 'Scene: Dynamic Surface / Flat Pool' in text and 'script asset=' not in text and 'Asset problem:' not in text
 fixture=ROOT/'build/m55-editor-project'
 transition=ROOT/'build/m55-transition-project'
 if not transition.exists():shutil.copytree(ROOT/'projects/liquid_surface_demo',transition)
 (transition/'Assets/scripts/lab.js').write_text("""import {world,liquid,LiquidVolume,scenes,session,console,ui} from 'judas';
export const properties={radial:{type:'boolean',default:false}};
export default class {
 constructor(){this.clock=0;this.pending=false;}
 fixedUpdate(dt){if(this.pending)return;this.clock+=dt;if(this.clock<.2)return;
  const stage=session.get('m55Stage')||0,id=stage===2?'120':'20',e=world.entity(id);if(!e.valid)return;const main=e.liquid;if(!main)return;
  if(liquid.errors.length)throw Error(liquid.errors[0].message);
  const total=liquid.accounting();if(Math.abs(total.error)>total.tolerance)throw Error('ledger');
  if(!main.state.surface||Math.abs(main.state.surface.volume-main.state.volume)>1e-8)throw Error('partition');
  if(stage===0){const target=world.entity('21').liquid;if(!target)return;session.set('m55Old',main.handle);if(Math.abs(main.transferTo(target,.8)-.8)>1e-9||Math.abs(main.state.volume-.2)>1e-9)throw Error('800 L');session.set('m55Stage',1);console.log('M55_DRAIN_1000_TO_200');this.pending=true;scenes.reload();}
  else if(stage===1){if(Math.abs(main.state.volume-1)>1e-9)throw Error('reload quantity');const old=new LiquidVolume(session.get('m55Old'));if(old.valid)throw Error('stale world handle');let threw=false;try{old.state;}catch(e){threw=true;}if(!threw)throw Error('stale access');console.log('M55_RELOAD_SAFE');session.set('m55Stage',2);this.pending=true;scenes.load('Scenes/radial.judas');}
  else if(stage===2){if(Math.abs(main.state.volume-65)>1e-8)throw Error('radial quantity');const sample=liquid.sample({x:0,y:20.6,z:0});if(!sample||sample.up.y<.99)throw Error('radial query');const storage=world.entity('121').liquid;main.transferTo(storage,.8);storage.transferTo(main,.8);console.log('M55_RADIAL_REFILL_QUERY_PASS');session.set('m55Stage',3);this.pending=true;scenes.load('Scenes/lab.judas');}
  else {if(Math.abs(main.state.volume-1)>1e-9||liquid.accounting().detached!==0)throw Error('transition leak');console.log('M55_TRANSITIONS_RELOAD_EXPORT_PASS');ui.quit();}
 }
}
""")
 text=run('transitions',[ROOT/'build/judas',transition/'liquid_surface_demo.judasproj']);assert 'M55_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 transitionPackage=ROOT/'.cache/m55-transition-package';transitionMoved=Path('/tmp/Judas_M55_Transition_Proof');assert not transitionPackage.exists() and not transitionMoved.exists()
 run('export-transition-probe',[ROOT/'build/judas_export',transition/'liquid_surface_demo.judasproj',transitionPackage]);shutil.move(str(transitionPackage),str(transitionMoved))
 text=run('moved-transition-probe',[transitionMoved/'judas'],cwd=Path('/tmp'));assert 'M55_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 cavity=fixture/'Assets/liquid/main.judascavity';cavity.write_text(cavity.read_text()+'\n')
 text=run('stale-export-rejection',[ROOT/'build/judas_export',fixture/'liquid_surface_demo.judasproj',ROOT/'.cache/m55-stale-export'],expected=1);assert 'Stale liquid basin' in text
 result['seconds']=time.monotonic()-start;result['overall_pass']=True;save();print(json.dumps(result,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
