#!/usr/bin/env python3
"""Narrow M53 lifetime/modifier corrections and actual link traversal proof.
Does not repeat the production or async suites.
"""
import json,os,shutil,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m53/lifetime-followup'
def main():
 assert not (OUT/'RESULTS.json').exists(),'Preserve previous follow-up evidence'
 OUT.mkdir(parents=True,exist_ok=True);result={'commands':[],'full_production_rerun':False}
 env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
 def run(name,cmd,cwd=ROOT,extra=None):
  start=time.monotonic();p=subprocess.run([str(x) for x in cmd],cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=240);(OUT/(name+'.log')).write_text(p.stdout);result['commands'].append({'name':name,'exit_code':p.returncode,'seconds':time.monotonic()-start});(OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');assert p.returncode==0,name;return p.stdout
 run('final-affected-build',['cmake','--build',ROOT/'build','--target','judas_navigation_tests','judas_navigation_game_tests','judasjs_examples_tests','judas','judas_editor','judas_export','-j6'])
 run('navigation',[ROOT/'build/judas_navigation_tests'])
 text=run('game-lifetime',[ROOT/'build/judas_navigation_game_tests']);assert text.count('M53_NAV_DESTROY_CALLBACK')==6 and 'script asset=' not in text
 run('navigation-example',[ROOT/'build/judasjs_examples_tests',ROOT/'build/m53-followup-examples','navigation'])
 run('surface',[ROOT/'build/judasjs_examples_tests',ROOT/'build/m53-followup-examples','surface'])
 node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
 result['api']=json.loads(run('api',[node,ROOT/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m51-typescript/package/lib/typescript.js','--runtime',ROOT/'build/m53-followup-examples/surface.json']))
 # An ordinary copied game observes its sixth agent crossing the real collider gap.
 fixture=ROOT/'build/m53-link-project';shutil.copytree(ROOT/'projects/shooter_game',fixture)
 script=fixture/'Assets/scripts/arena.js';s=script.read_text().replace('{input,world,ui}','{input,world,ui,console}').replace('constructor(){','constructor(){this.elapsed=0;')
 s=s.replace('update(){', '''update(dt){this.elapsed+=dt;
 const traveller=world.entity('1015');
 if(this.elapsed>6){if(traveller.transform.position.x>=23||traveller.navigation.state.onLink)throw Error('link crossing did not finish');console.log('M53_PHYSICAL_LINK_TRAVERSAL_PASS x='+traveller.transform.position.x);ui.quit();return;}
 ''',1);script.write_text(s)
 text=run('physical-link',[ROOT/'build/judas',fixture/'shooter_game.judasproj']);assert 'M53_PHYSICAL_LINK_TRAVERSAL_PASS' in text and 'script asset=' not in text
 # Reject a stale authored source before packaging, without altering the real project.
 scene=fixture/'Scenes/range.judas';s=scene.read_text().replace('position 24 2 -20.75','position 23 2 -20.75');scene.write_text(s)
 failed=subprocess.run([str(ROOT/'build/judas_export'),str(fixture/'shooter_game.judasproj'),str(ROOT/'.cache/m53-stale-package')],cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/'stale-export.log').write_text(failed.stdout);assert failed.returncode!=0 and 'Stale navigation bake' in failed.stdout
 # New runtime binaries replace the prior package through the normal clean exporter.
 package=ROOT/'.cache/m53-final-package';moved=Path('/tmp/Judas_Spring_Range_M53_Final');assert not package.exists() and not moved.exists()
 run('export',[ROOT/'build/judas_export',ROOT/'projects/shooter_game/shooter_game.judasproj',package]);shutil.move(str(package),str(moved))
 steps=OUT/'package.steps';steps.write_text('STEPS 120\nLOG_EVERY 60\nSCREENSHOT 110 '+str(OUT/'game.png')+'\n')
 text=run('moved-package',[moved/'judas'],Path('/tmp'),{'JUDAS_TEST_SCRIPT':str(steps)});assert 'Scene: Spring Range' in text and 'script asset=' not in text and 'Asset problem:' not in text
 editor=ROOT/'build/m53-followup-editor-project';shutil.copytree(ROOT/'projects/shooter_game',editor)
 text=run('editor',[ROOT/'build/judas_editor',editor/'shooter_game.judasproj'],extra={'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor'),'JUDAS_EDITOR_AUTOTEST_NAV_BAKE':'1'});assert 'authored scene after play/stop is IDENTICAL' in text and 'navigation bake 550: PASS' in text and 'navigation bake 551: PASS' in text
 result.update(overall_pass=True,package_path=str(moved),package_bytes=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file()),human_validation='PENDING')
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
