#!/usr/bin/env python3
"""Focused M52 candidate gate. Does not invoke unrelated production suites."""
import subprocess,os,json,shutil,time,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m52/final';BUILD=ROOT/'.cache/m52-release'
def main():
 assert not OUT.exists(),'Preserve prior evidence'
 OUT.mkdir(parents=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')}
 env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 result={'starting_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'commands':[], 'full_production_rerun':False,'reason':'Only new native code exposes the existing PhysicsWorld impulse-at-point; no solver/runtime policy change. Focused scripting, joints and application checks cover it.'}
 def run(name,cmd,extra=None,cwd=ROOT):
  start=time.monotonic();p=subprocess.run([str(c) for c in cmd],cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=240)
  (OUT/(name+'.log')).write_text(p.stdout);result['commands'].append({'name':name,'exit_code':p.returncode,'seconds':time.monotonic()-start,'command':[str(c) for c in cmd]})
  (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');assert p.returncode==0,name+' failed';return p.stdout
 run('build-final-checks',['cmake','--build',BUILD,'--target','judas_shooter_game_tests','judasjs_examples_tests','-j4'])
 run('game',[BUILD/'judas_shooter_game_tests'])
 run('application',[BUILD/'judas_shooter_application_tests',ROOT/'projects/shooter_game/shooter_game.judasproj',OUT/'application'])
 run('script-regression',[BUILD/'judas_script_tests'])
 run('joint-regression',[BUILD/'judas_joint_tests'])
 run('surface',[BUILD/'judasjs_examples_tests',ROOT/'build/m52-examples','surface'])
 run('impulse-point',[BUILD/'judasjs_examples_tests',ROOT/'build/m52-examples','impulse-point'])
 run('stale-handle',[BUILD/'judasjs_examples_tests',ROOT/'build/m52-examples','stale-handle'])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m51-typescript/package/lib/typescript.js','--runtime',ROOT/'build/m52-examples/surface.json']))
 fixture=ROOT/'build/m52-editor-project';assert not fixture.exists();shutil.copytree(ROOT/'projects/shooter_game',fixture)
 text=run('editor',[BUILD/'judas_editor',fixture/'shooter_game.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')});assert 'authored scene after play/stop is IDENTICAL' in text and 'script asset=' not in text
 package=ROOT/'.cache/m52-package';moved=Path('/tmp/Judas_Spring_Range_M52');assert not package.exists() and not moved.exists()
 exportStart=time.monotonic();text=run('export',[BUILD/'judas_export',ROOT/'projects/shooter_game/shooter_game.judasproj',package]);result['export_seconds']=time.monotonic()-exportStart
 shutil.move(str(package),str(moved));result['package_path']=str(moved);result['package_bytes']=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file())
 steps=OUT/'package.steps';steps.write_text('STEPS 60\nLOG_EVERY 60\n')
 text=run('moved-package',[moved/'judas'],{'JUDAS_TEST_SCRIPT':str(steps)},Path('/tmp'));assert 'Scene: Spring Range' in text and 'script asset=' not in text and 'Asset problem:' not in text
 result['human_acceptance']='PENDING: visual, audio, controller hardware and game feel require operator review'
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
