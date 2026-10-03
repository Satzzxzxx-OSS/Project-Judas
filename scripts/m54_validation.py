#!/usr/bin/env python3
"""One M54 clean Release/production+async gate; current liquid project lifecycle/export.
Historical assertions are imported unchanged with output/configuration adapters.
"""
import importlib.util,json,os,shutil,subprocess,sys,time,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m54/final'
def main():
 resume='--resume-focused' in sys.argv
 if not resume:assert not OUT.exists(),'Preserve prior evidence'
 backup=ROOT/'.cache/m54-before-clean-release';
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
 # Both focused M54 targets already ran once in the production target discovery.
 shutil.copytree(ROOT/'build/m54-visual',OUT/'visual',dirs_exist_ok=resume)
 run('cookbook',[ROOT/'build/judasjs_examples_tests',ROOT/'build/m54-examples'])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m51-typescript/package/lib/typescript.js','--runtime',ROOT/'build/m54-examples/surface.json']))
 fixture=ROOT/'build/m54-editor-project';shutil.copytree(ROOT/'projects/liquid_reservoir_demo',fixture,dirs_exist_ok=resume)
 for scene in ('lab','radial'):run('bake-'+scene,[ROOT/'build/judas_liquid_bake',fixture/'liquid_reservoir_demo.judasproj','Scenes/'+scene+'.judas'])
 text=run('editor-bake-play-stop',[ROOT/'build/judas_editor',fixture/'liquid_reservoir_demo.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor'),'JUDAS_EDITOR_AUTOTEST_LIQUID_BAKE':'1'});assert 'authored scene after play/stop is IDENTICAL' in text and 'liquid bake 20: PASS' in text and 'script asset=' not in text
 package=ROOT/'.cache/m54-package';moved=Path('/tmp/Judas_Liquid_Reservoir_M54')
 if resume and moved.exists():
  previous=moved.with_name(moved.name+'_before_followup');assert not previous.exists();moved.rename(previous)
 assert not package.exists() and not moved.exists()
 exportStart=time.monotonic();run('export',[ROOT/'build/judas_export',ROOT/'projects/liquid_reservoir_demo/liquid_reservoir_demo.judasproj',package]);result['export_seconds']=time.monotonic()-exportStart
 shutil.move(str(package),str(moved));result['package_path']=str(moved);result['package_bytes']=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file());result['packaged_assets']=len(list(moved.rglob('*.judasmeta')))
 steps=OUT/'package.steps';steps.write_text('STEPS 120\nLOG_EVERY 60\nSCREENSHOT 110 '+str(OUT/'package-world.png')+'\n')
 text=run('moved-package',[moved/'judas'],{'JUDAS_TEST_SCRIPT':str(steps)},Path('/tmp'));assert 'Scene: Reservoir Lab' in text and 'script asset=' not in text and 'Asset problem:' not in text
 transition=ROOT/'build/m54-transition-project';shutil.copytree(ROOT/'projects/liquid_reservoir_demo',transition,dirs_exist_ok=resume)
 (transition/'Assets/scripts/lab.js').write_text("""import {world,liquid,LiquidVolume,scenes,session,console,ui} from 'judas';
export const properties={radial:{type:'boolean',default:false}};
export default class {
 constructor(){this.clock=0;this.spawn=null;this.pending=false;}
 fixedUpdate(dt){if(this.pending)return;this.clock+=dt;if(this.clock<.2)return;
  const stage=session.get('m54Stage')||0,id=stage===2?'120':'20',e=world.entity(id);if(!e.valid)return;const main=e.liquid;if(!main)return;
  if(liquid.errors.length)throw Error(liquid.errors[0].message);
  const total=liquid.accounting();if(Math.abs(total.error)>total.tolerance)throw Error('ledger');
  if(stage===0){const target=world.entity('21').liquid;if(!target)return;session.set('m54Old',main.handle);if(Math.abs(main.transferTo(target,.8)-.8)>1e-9||Math.abs(main.state.volume-.2)>1e-9)throw Error('800 L');session.set('m54Stage',1);console.log('M54_DRAIN_1000_TO_200');this.pending=true;scenes.reload();}
  else if(stage===1){if(Math.abs(main.state.volume-1)>1e-9&&!this.spawn)throw Error('reload quantity');const old=new LiquidVolume(session.get('m54Old'));if(old.valid)throw Error('stale world handle');let threw=false;try{old.state;}catch(e){threw=true;}if(!threw)throw Error('stale access');
   if(!this.spawn){this.spawn=world.spawnPrefab('54545454545454545454545454540008',{position:{x:0,y:2,z:1}});return;}const cup=this.spawn.liquid;if(!cup)return;main.transferTo(cup,.01);this.spawn.destroy();if(cup.valid)throw Error('stale destroyed liquid');if(Math.abs(liquid.accounting().total-1)>1e-9)throw Error('destroy loss');console.log('M54_RELOAD_PREFAB_DESTROY_SAFE');session.set('m54Stage',2);this.pending=true;scenes.load('Scenes/radial.judas');}
  else if(stage===2){if(Math.abs(main.state.volume-2)>1e-9)throw Error('radial quantity');const p=e.transform.position;const up={x:p.x/20,y:p.y/20,z:p.z/20};const sample=liquid.sample({x:p.x+up.x*.1,y:p.y+up.y*.1,z:p.z+up.z*.1});if(!sample||Math.abs(sample.normal.x)<.5)throw Error('radial arbitrary gravity');const storage=world.entity('121').liquid;main.transferTo(storage,.8);if(Math.abs(main.state.volume-1.2)>1e-9)throw Error('radial drain');storage.transferTo(main,.8);console.log('M54_RADIAL_REFILL_QUERY_PASS');session.set('m54Stage',3);this.pending=true;scenes.load('Scenes/lab.judas');}
  else {if(Math.abs(main.state.volume-1)>1e-9||liquid.accounting().detached!==0)throw Error('transition leak');console.log('M54_TRANSITIONS_RELOAD_EXPORT_PASS');ui.quit();}
 }
}
""")
 text=run('transitions',[ROOT/'build/judas',transition/'liquid_reservoir_demo.judasproj']);assert 'M54_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 transitionPackage=ROOT/'.cache/m54-transition-package';transitionMoved=Path('/tmp/Judas_M54_Transition_Proof');assert not transitionPackage.exists() and not transitionMoved.exists()
 run('export-transition-probe',[ROOT/'build/judas_export',transition/'liquid_reservoir_demo.judasproj',transitionPackage]);shutil.move(str(transitionPackage),str(transitionMoved))
 text=run('moved-transition-probe',[transitionMoved/'judas'],cwd=Path('/tmp'));assert 'M54_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
 # Semantically identical source bytes with a changed fingerprint must reject stale bakes.
 cavity=fixture/'Assets/liquid/main.judascavity';cavity.write_text(cavity.read_text()+'\n')
 text=run('stale-export-rejection',[ROOT/'build/judas_export',fixture/'liquid_reservoir_demo.judasproj',ROOT/'.cache/m54-stale-export'],expected=1);assert 'Stale liquid basin' in text
 result['seconds']=time.monotonic()-start;result['overall_pass']=True;save();print(json.dumps(result,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
