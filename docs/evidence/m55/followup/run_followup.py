import os,json,subprocess,time,shutil
from pathlib import Path
root=Path('/home/conner/Documents/GitHub/Project-Judas'); out=root/'docs/evidence/m55/followup'
env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
result={'scope':'Narrow geometry-cache and cap-topology follow-up; broad gate preserved, not repeated','commands':[],'human_review':'PENDING'}
def run(name,args,extra=None,cwd=root):
 begin=time.monotonic();p=subprocess.run([str(a) for a in args],cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=60)
 (out/(name+'.log')).write_text(p.stdout);result['commands'].append(dict(name=name,args=[str(a) for a in args],cwd=str(cwd),code=p.returncode,seconds=time.monotonic()-begin));(out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');assert p.returncode==0,(name,p.stdout);print(name,'PASS',flush=True);return p.stdout
run('cookbook',[root/'build/judasjs_examples_tests',root/'build/m55-followup-examples'])
node='/home/conner/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
result['api']=json.loads(run('api',[node,root/'scripts/check_judasjs_api.mjs','--typescript','/tmp/judas-m51-typescript/package/lib/typescript.js','--runtime',root/'build/m55-followup-examples/surface.json']))
fixture=root/'build/m55-cache-editor-project';assert not fixture.exists();shutil.copytree(root/'projects/liquid_surface_demo',fixture)
text=run('editor-play-stop',[root/'build/judas_editor',fixture/'liquid_surface_demo.judasproj'],{'JUDAS_EDITOR_AUTOTEST':str(out/'editor'),'JUDAS_EDITOR_AUTOTEST_LIQUID_BAKE':'1'});assert 'authored scene after play/stop is IDENTICAL' in text and 'liquid bake 20: PASS' in text and 'script asset=' not in text
package=root/'.cache/m55-final-package';moved=Path('/tmp/Judas_Dynamic_Surface_M55_Final');assert not package.exists() and not moved.exists()
run('export',[root/'build/judas_export',root/'projects/liquid_surface_demo/liquid_surface_demo.judasproj',package]);shutil.move(str(package),str(moved));result['package_path']=str(moved);result['package_bytes']=sum(p.stat().st_size for p in moved.rglob('*') if p.is_file());result['packaged_assets']=len(list(moved.rglob('*.judasmeta')))
steps=out/'package.steps';steps.write_text('STEPS 120\nLOG_EVERY 60\nSCREENSHOT 110 '+str(out/'package-world.png')+'\n')
text=run('moved-package',[moved/'judas'],{'JUDAS_TEST_SCRIPT':str(steps)},Path('/tmp'));assert 'Scene: Dynamic Surface / Flat Pool' in text and 'script asset=' not in text and 'Asset problem:' not in text
transition=root/'build/m55-cache-transition-project';assert not transition.exists();shutil.copytree(root/'projects/liquid_surface_demo',transition)
probe=(root/'build/m55-transition-project/Assets/scripts/lab.js').read_text();assert "export const properties={radial:" in probe;(transition/'Assets/scripts/lab.js').write_text(probe);(out/'transition-probe.js').write_text(probe)
text=run('transitions',[root/'build/judas',transition/'liquid_surface_demo.judasproj']);assert 'M55_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
package=root/'.cache/m55-final-transition-package';moved=Path('/tmp/Judas_M55_Final_Transition_Proof');assert not package.exists() and not moved.exists()
run('export-transition-probe',[root/'build/judas_export',transition/'liquid_surface_demo.judasproj',package]);shutil.move(str(package),str(moved))
text=run('moved-transition-probe',[moved/'judas'],cwd=Path('/tmp'));assert 'M55_TRANSITIONS_RELOAD_EXPORT_PASS' in text and 'script asset=' not in text
result['overall_pass']=True;(out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
