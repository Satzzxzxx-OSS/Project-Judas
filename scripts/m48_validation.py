#!/usr/bin/env python3
"""M47+M48: one clean Release/production gate, then ordinary editor/export smoke.
Existing output adapters preserve historical evidence; no simulation bypasses.
"""
import importlib.util,json,os,shutil,subprocess,sys,time,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];FOLLOWUP='--followup' in sys.argv;OUT=ROOT/('docs/evidence/m48/lifecycle-followup' if FOLLOWUP else 'docs/evidence/m48/final')
def main():
 assert not OUT.exists(),'Do not overwrite evidence'
 backup=ROOT/'.cache/m48-before-clean-release'
 if FOLLOWUP:
  assert (ROOT/'build').is_dir() and json.loads((ROOT/'docs/evidence/m48/final/RESULTS.json').read_text())['regression']['overall_pass']
 elif '--resume-build' not in sys.argv:
  assert not backup.exists()
  (ROOT/'.cache').mkdir(exist_ok=True)
  if (ROOT/'build').exists():(ROOT/'build').rename(backup)
 else: assert (ROOT/'build').is_dir() and backup.is_dir(), 'resume only the M47/M48 clean build'
 OUT.mkdir(parents=True);start=time.monotonic()
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests','judas_package_application_tests','judas_classification_tests','judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests','judas_physics_cast_application_tests','judas_joint_application_tests','judas_skeletal_animation_tests','judas_animation_application_tests','judas_pose_composition_application_tests','judas_ragdoll_tests','judas_ragdoll_application_tests'})
 gate.REVIEWED_SHARED.add('src/WorldState.cpp') # reviewed transient articulation-internal exclusion only
 original=subprocess.run
 def release(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command.append('-DCMAKE_BUILD_TYPE=Release')
  return original(command,*args,**kwargs)
 subprocess.run=release
 try:regression=json.loads((ROOT/'docs/evidence/m48/final/RESULTS.json').read_text())['regression'] if FOLLOWUP else gate.production(OUT,4)
 finally:subprocess.run=original
 results={'protected_paths_unchanged':not subprocess.check_output(['git','status','--porcelain','--','prototypes/fluid_coupling/r1_p1','docs/evidence/ftft1','docs/evidence/ftft2','docs/evidence/ftft3','docs/evidence/ftft4','docs/evidence/ftft4b','docs/evidence/ftft5','docs/evidence/ftft6','docs/evidence/ftft7','docs/evidence/ftft8','docs/evidence/ftft9','docs/evidence/m33','docs/evidence/m34','docs/evidence/m35','docs/evidence/m36','docs/evidence/m37','docs/evidence/m38','docs/evidence/m39','docs/evidence/m40','docs/evidence/m41','docs/evidence/m42','docs/evidence/m43','docs/evidence/m44','docs/evidence/m45','docs/evidence/m46','ROADMAP.md'],cwd=ROOT,text=True).strip(),'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'clean_release':not FOLLOWUP,'regression':regression,'regression_reused_from':'docs/evidence/m48/final' if FOLLOWUP else None,'commands':[]}
 def save():(OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
 save()
 if not regression['overall_pass']:return 1
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 def run(name,command,extra={},cwd=ROOT):
  before=time.monotonic();p=original(command,cwd=cwd,env=dict(env,**extra),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
  (OUT/(name+'.log')).write_text(p.stdout);results['commands'].append({'name':name,'command':command,'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-before});save();assert p.returncode==0,name+' failed';return p.stdout
 fixture=ROOT/('build/m48-followup-editor-project' if FOLLOWUP else 'build/m48-editor-project');shutil.copytree(ROOT/'projects/ragdoll_demo',fixture)
 text=run('demo_editor',['./build/judas_editor',str(fixture/'ragdoll_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')})
 assert 'authored scene after play/stop is IDENTICAL' in text
 steps=OUT/'demo_steps.txt';steps.write_text('STEPS 3\nLOG_EVERY 1\nSCREENSHOT 2 '+str(OUT/'demo.png')+'\n')
 run('demo_standalone',['./build/judas','projects/ragdoll_demo/ragdoll_demo.judasproj'],{'JUDAS_TEST_SCRIPT':str(steps)})
 fixture=ROOT/('build/m48-followup-export-project' if FOLLOWUP else 'build/m48-export-project');shutil.copytree(ROOT/'projects/ragdoll_demo',fixture)
 script=fixture/'Assets/scripts/animation.js';source=script.read_text();needle="update(){if(input.pressed('spawn_prefab'))";assert needle in source
 source=source.replace(needle,"""update(){if(this.a.info.ready){this.frames=(this.frames||0)+1;if(this.frames===2){
 this.a.crossFade('Stretch',.5);world.entity('10').ragdoll.enter();this.packageSpawn=this.spawn();this.packageSpawn.ragdoll.enter();
 }if(this.frames===8){if(!world.entity('10').ragdoll.active||!this.packageSpawn.ragdoll.active)throw Error('M48 packaged activation');
 world.entity('10').ragdoll.leave(.2);this.packageSpawn.destroy();
 console.log('M48_PACKAGE_POSE_RAGDOLL clips='+this.a.clips.length+' transition='+this.a.info.transitioning+' retired='+!this.packageSpawn.valid);ui.quit();}}
 if(input.pressed('spawn_prefab'))""");script.write_text(source)
 package=ROOT/('.cache/m48-followup-export-package' if FOLLOWUP else '.cache/m48-export-package');moved=Path('/tmp/Judas_M48_Followup_Package' if FOLLOWUP else '/tmp/Judas_M48_Moved_Package');assert not package.exists() and not moved.exists()
 run('export',['./build/judas_export',str(fixture/'ragdoll_demo.judasproj'),str(package)])
 shutil.move(str(package),str(moved));text=run('moved_package',[str(moved/'judas')],cwd=Path('/tmp'))
 assert 'M48_PACKAGE_POSE_RAGDOLL clips=2 transition=true retired=true' in text
 results.update(seconds=time.monotonic()-start,package_bytes=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),human_visual_validation='PENDING; operator authority')
 paths=subprocess.check_output(['git','diff','--name-only'],cwd=ROOT,text=True).splitlines()+subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
 sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(set(paths)) if (ROOT/p).is_file() and (p.startswith(('src/','tests/','scripts/','projects/ragdoll_demo/','projects/pose_demo/','third_party/cgltf/')) or p in ('CMakeLists.txt','third_party/RUNTIME_NOTICES.txt'))}
 (OUT/'SOURCE_SHA256.json').write_text(json.dumps(sources,indent=2)+'\n');save();print(json.dumps(results,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
