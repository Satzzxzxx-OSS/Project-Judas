#!/usr/bin/env python3
"""M45: one clean Release/production gate, then ordinary editor/export smoke.
Existing output adapters preserve historical evidence; no simulation bypasses.
"""
import importlib.util,json,os,shutil,subprocess,sys,time,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m45/final'
def main():
 assert not OUT.exists(),'Do not overwrite evidence'
 backup=ROOT/'.cache/m45-before-clean-release';assert not backup.exists()
 (ROOT/'.cache').mkdir(exist_ok=True)
 if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True);start=time.monotonic()
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests','judas_package_application_tests','judas_classification_tests','judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests','judas_physics_cast_application_tests','judas_joint_application_tests'})
 gate.REVIEWED_SHARED.add('src/WorldState.cpp')
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
 def run(name,command,extra={},cwd=ROOT):
  before=time.monotonic();p=original(command,cwd=cwd,env=dict(env,**extra),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
  (OUT/(name+'.log')).write_text(p.stdout);results['commands'].append({'name':name,'command':command,'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-before});save();assert p.returncode==0,name+' failed';return p.stdout
 fixture=ROOT/'build/m45-editor-project';shutil.copytree(ROOT/'projects/joint_demo',fixture)
 text=run('demo_editor',['./build/judas_editor',str(fixture/'joint_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')})
 assert 'authored scene after play/stop is IDENTICAL' in text
 steps=OUT/'demo_steps.txt';steps.write_text('STEPS 3\nLOG_EVERY 1\nSCREENSHOT 2 '+str(OUT/'demo.png')+'\n')
 run('demo_standalone',['./build/judas','projects/joint_demo/joint_demo.judasproj'],{'JUDAS_TEST_SCRIPT':str(steps)})
 fixture=ROOT/'build/m45-export-project';shutil.copytree(ROOT/'projects/joint_demo',fixture)
 script=fixture/'Assets/scripts/queries.js';source=script.read_text();needle="update(){if(input.pressed('spawn_prefab'))";assert needle in source
 source=source.replace(needle,"""update(){this.frames=(this.frames||0)+1;if(this.frames===5){
 if(!this.door.valid||!this.slider.valid||!this.door.state.active)throw Error('M45 package joint');
 this.door.setMotor(.5,10);console.log('M45_PACKAGE_JOINT active='+this.door.state.active);ui.quit();}
 if(input.pressed('spawn_prefab'))""");script.write_text(source)
 package=ROOT/'.cache/m45-export-package';moved=Path('/tmp/Judas_M45_Moved_Package');assert not package.exists() and not moved.exists()
 run('export',['./build/judas_export',str(fixture/'joint_demo.judasproj'),str(package)])
 shutil.move(str(package),str(moved));text=run('moved_package',[str(moved/'judas')],cwd=Path('/tmp'))
 assert 'M45_PACKAGE_JOINT active=true' in text
 results.update(seconds=time.monotonic()-start,package_bytes=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),human_visual_validation='PENDING; operator authority')
 paths=subprocess.check_output(['git','diff','--name-only'],cwd=ROOT,text=True).splitlines()+subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
 sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(set(paths)) if (ROOT/p).is_file() and (p.startswith(('src/','tests/','scripts/','projects/joint_demo/')) or p=='CMakeLists.txt')}
 (OUT/'SOURCE_SHA256.json').write_text(json.dumps(sources,indent=2)+'\n');save();print(json.dumps(results,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
