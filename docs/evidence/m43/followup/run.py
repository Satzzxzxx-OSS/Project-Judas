"""Narrow post-gate cursor/UI + authored demo binding follow-up. No full rerun."""
from pathlib import Path
import subprocess,os,json,time,shutil,hashlib
ROOT=Path(__file__).resolve().parents[4];OUT=Path(__file__).resolve().parent
results=[]
env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')}
env.update(SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1',JUDAS_WORLD_STATE='none')
def run(name,cmd,extra=None,cwd=ROOT):
    start=time.monotonic();p=subprocess.run(cmd,cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=180)
    (OUT/(name+'.log')).write_text(p.stdout)
    results.append(dict(name=name,command=cmd,cwd=str(cwd),exit_code=p.returncode,seconds=time.monotonic()-start))
    (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
    assert p.returncode==0,name
    return p.stdout
run('build',['cmake','--build','build','-j4','--target','judas','judas_editor','judas_scene_transition_tests','judas_runtime_ui_application_tests','judas_input_tests','judas_export'])
run('transitions',['./build/judas_scene_transition_tests','--output',str(ROOT/'build/m43-cursor-after')])
run('runtime_ui',['./build/judas_runtime_ui_application_tests','--output',str(ROOT/'build/m43-ui-followup')])
run('input',['./build/judas_input_tests'])
fixture=ROOT/'build/m43-editor-followup';assert not fixture.exists();shutil.copytree(ROOT/'projects/scene_demo',fixture)
text=run('editor',['./build/judas_editor',str(fixture/'scene_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')})
assert 'authored scene after play/stop is IDENTICAL' in text
# Same project-owned JS automation as the full gate; package runs normal loop.
fixture=ROOT/'build/m43-export-followup';assert not fixture.exists();shutil.copytree(ROOT/'projects/scene_demo',fixture)
hud=fixture/'Assets/scripts/hud.js';source=hud.read_text();needle="this.hud.get('pause_options').text='Reload this scene';}"
assert needle in source
source=source.replace(needle,"""this.hud.get('pause_options').text='Reload this scene';
if(!session.get('exportStarted')){session.set('exportStarted',true);this.spawn();}
else if(scenes.current==='Scenes/b.judas'){console.log('M43_PACKAGE_B key='+session.get('hasKey'));scenes.load('Scenes/a.judas');}
else {console.log('M43_PACKAGE_A session='+session.get('hasKey'));ui.quit();}}
""");hud.write_text(source)
package=ROOT/'.cache/m43-followup-package';moved=Path('/tmp/Judas_M43_Moved_ed6e8fc');assert not moved.exists()
run('export',['./build/judas_export',str(fixture/'scene_demo.judasproj'),str(package)])
shutil.move(str(package),str(moved))
text=run('moved_package',[str(moved/'judas')],cwd=Path('/tmp'))
assert 'M43_PACKAGE_B key=true' in text and 'M43_PACKAGE_A session=true' in text
changed=subprocess.check_output(['git','diff','--name-only'],cwd=ROOT,text=True).splitlines()
changed+=subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(set(changed)) if (ROOT/p).is_file() and (p.startswith(('src/','tests/','scripts/','projects/scene_demo/')) or p=='CMakeLists.txt')}
(OUT/'SOURCE_SHA256.json').write_text(json.dumps(sources,indent=2)+'\n')
print(json.dumps(results,indent=2))
