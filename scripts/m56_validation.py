#!/usr/bin/env python3
"""One clean Release + production/actual async gate, then M56 developer/lifecycle/package checks.
No historical scripts/assertions are edited; their outputs go to this milestone.
"""
import importlib.util,json,os,shutil,subprocess,sys,time,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m56/final'
def main():
 finish='--resume-smoke' in sys.argv
 tail='--resume-tail' in sys.argv or finish
 resume='--resume-build' in sys.argv or tail
 if not resume:assert not OUT.exists(),'Preserve prior gate evidence; do not repeat the broad gate.'
 backup=ROOT/'.cache/m56-before-clean-release'
 if not resume:assert not backup.exists()
 backup.parent.mkdir(exist_ok=True)
 if not resume and (ROOT/'build').exists():(ROOT/'build').rename(backup)
 OUT.mkdir(parents=True,exist_ok=resume);result={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'human_validation':'PENDING: operator authority','commands':[]};start=time.monotonic()
 def save():(OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
 gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests','judas_package_application_tests','judas_classification_tests','judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests','judas_physics_cast_application_tests','judas_joint_application_tests','judas_skeletal_animation_tests','judas_animation_application_tests','judas_pose_composition_application_tests','judas_ragdoll_tests','judas_ragdoll_application_tests','judas_fluid_demo_integration_tests','judas_character_motor_application_tests','judas_profiler_application_tests'})
 original=subprocess.run
 def release(command,*args,**kwargs):
  if command==['cmake','-S','.','-B','build']:command=command+['-DCMAKE_BUILD_TYPE=Release']
  if isinstance(command,list) and command and Path(command[0]).name=='judas_shooter_application_tests' and len(command)==1:command=command+[str(ROOT/'projects/shooter_game/shooter_game.judasproj'),str(OUT/'shooter-application')]
  return original(command,*args,**kwargs)
 subprocess.run=release
 if tail:
  result=json.loads((OUT/'RESULTS.json').read_text())
  raw=json.loads((OUT/'production/results.json').read_text())
  changed=[k for k in set(raw['source_sha256_before'])|set(raw['source_sha256_after']) if raw['source_sha256_before'].get(k)!=raw['source_sha256_after'].get(k)]
  assert changed==['tests/PerformanceProfilerTests.cpp'],changed
  assert not raw['checks']['tested_sources_unchanged_during_run']
  assert all(v for k,v in raw['checks'].items() if k!='tested_sources_unchanged_during_run')
  assert all(r['exit_code']==0 and not r.get('failing_lines') for r in raw['runs'])
  result['source_followup']={'original_gate_flag_preserved':True,'during_gate_changed_files':changed,'reason':'Two additional concurrent-counter checks and a native ABBA overhead probe were added to the core test before its compilation. No runtime source changed during the broad gate. Recheck the core against stable source. Later editor-only live-inspection cursor routing is covered by a narrow application rerun; the raw broad results remain immutable.'}
 else:
  try:result['regression']=gate.production(OUT,6)
  finally:subprocess.run=original
  save();assert result['regression']['overall_pass'],'Broad gate failed; inspect actual issue before any rerun.'
 subprocess.run=original
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 def run(name,command,extra=None,cwd=ROOT,expected=0):
  command=[str(x) for x in command];begin=time.monotonic()
  p=original(command,cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=180)
  (OUT/(name+'.log')).write_text(p.stdout);result['commands'].append({'name':name,'command':command,'cwd':str(cwd),'exit_code':p.returncode,'seconds':time.monotonic()-begin});save();assert p.returncode==expected,name+' failed';return p.stdout
 if tail and not finish:
  before=hashlib.sha256((ROOT/'tests/PerformanceProfilerTests.cpp').read_bytes()).hexdigest()
  run('core-source-followup',[ROOT/'build/judas_profiler_tests'])
  assert before==hashlib.sha256((ROOT/'tests/PerformanceProfilerTests.cpp').read_bytes()).hexdigest()
  run('script-followup',[ROOT/'build/judas_profiler_script_tests'])
  run('live-inspection-followup',[ROOT/'build/judas_profiler_application_tests',OUT/'live-inspection-followup'])
  result['source_followup']['stable_core_sha256']=before
  result['source_followup']['focused_pass']=True
  save()
 if finish:
  run('observation-followup',[ROOT/'build/judas_profiler_application_tests',OUT/'observation-followup'])
 else:run('cookbook',[ROOT/'build/judasjs_examples_tests',ROOT/'build/m56-examples'])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 if not finish:result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m56-typescript/package/lib/typescript.js','--runtime',ROOT/'build/m56-examples/surface.json']));save()
 # Read the production-run M56 reports, rather than repeating the paired workload.
 app=OUT/('observation-followup' if finish else 'live-inspection-followup' if tail else 'fixture-details/judas_profiler_application_tests');reports=list(app.glob('*-capture.json'))+list(app.glob('*-UI.json'));assert len(reports)==4
 result['reports']=[]
 for path in reports:
  report=json.loads(path.read_text());assert report['frames'] and report['startup'] and not report['diagnostics']['dropped_events'] and not report['diagnostics']['truncated_records'];assert all(g['source_frame']==f['id'] for f in report['frames'] for g in f['gpu']['passes'])
  result['reports'].append({'path':str(path.relative_to(ROOT)),'frames':len(report['frames']),'gpu_completed':sum(not g['pending'] for f in report['frames'] for g in f['gpu']['passes']),'diagnostics':report['diagnostics']})
 headless=json.loads(Path('/tmp/judas-m56-headless.json').read_text());assert headless['clock']=='steady monotonic nanoseconds';result['headless_parsed']=True
 # Same existing editor Play/Stop automation, with collection and profiler view.
 fixture=ROOT/('build/m56-editor-final-project' if finish else 'build/m56-editor-project');shutil.copytree(ROOT/'projects/shooter_game',fixture)
 text=run('editor-final-play-stop' if finish else 'editor-profile-play-stop',[ROOT/'build/judas_editor',fixture/'shooter_game.judasproj'],{'JUDAS_PROFILE':'1','JUDAS_EDITOR_AUTOTEST_PROFILER':'1','JUDAS_PROFILE_OUTPUT':str(OUT/('editor-final-profile.json' if finish else 'editor-profile.json')),'JUDAS_EDITOR_AUTOTEST':str(OUT/('editor-final' if finish else 'editor'))});assert 'authored scene after play/stop is IDENTICAL' in text;assert 'live cursor + frozen capture while simulation continues: PASS' in text;assert 'closing profiler restores capture: PASS' in text
 editor=json.loads((OUT/('editor-final-profile.json' if finish else 'editor-profile.json')).read_text());assert any(f['mode']=='editor Play' for f in editor['frames']);assert any(s['name']=='Profiler UI' for f in editor['frames'] for s in f['scopes']);assert any('World destroy' in f['boundaries'] for f in editor['frames']);result['editor_capture']=True
 # Safe scene reload with the normal public API in an ignored fixture only.
 transition=ROOT/('build/m56-transition-final-project' if finish else 'build/m56-transition-project');shutil.copytree(ROOT/'projects/shooter_game',transition)
 controller=transition/'Assets/scripts/player.js';controller.write_text("""import {scenes,session,ui,console} from 'judas';export default class {constructor(){this.n=0;this.done=false;}start(){const d=ui.get('range_ui');d.modal=false;d.get('pause').visible=false;}fixedUpdate(){if(this.done||++this.n<10)return;this.done=true;if(!session.get('m56Reload')){session.set('m56Reload',true);scenes.reload();}else{console.log('M56_RELOAD_PASS');ui.quit();}}}""")
 text=run('reload',[ROOT/'build/judas',transition/'shooter_game.judasproj'],{'JUDAS_PROFILE':'1','JUDAS_PROFILE_OUTPUT':str(OUT/'reload-profile.json')});assert 'M56_RELOAD_PASS' in text;assert 'Scene replacement' in (OUT/'reload-profile.json').read_text()
 package=ROOT/'.cache/m56-package';moved=Path('/tmp/Judas_M56_Spring_Range');assert not package.exists() and not moved.exists()
 run('export',[ROOT/'build/judas_export',transition/'shooter_game.judasproj',package]);shutil.move(str(package),str(moved));result['package']=str(moved)
 text=run('moved-package-off',[moved/'judas'],{'JUDAS_PROFILE_OUTPUT':str(OUT/'package-off.json')},Path('/tmp'));assert 'M56_RELOAD_PASS' in text;off=json.loads((OUT/'package-off.json').read_text());assert not off['capture_enabled'] and not off['frames']
 text=run('moved-package-on',[moved/'judas'],{'JUDAS_PROFILE':'1','JUDAS_PROFILE_OUTPUT':str(OUT/'package-on.json')},Path('/tmp'));assert 'M56_RELOAD_PASS' in text;on=json.loads((OUT/'package-on.json').read_text());assert on['capture_enabled'] and on['frames']
 result['seconds']=time.monotonic()-start;result['overall_pass']=True;save();print(json.dumps(result,indent=2))
if __name__=='__main__':main()
