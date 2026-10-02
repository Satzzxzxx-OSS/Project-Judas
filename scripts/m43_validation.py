#!/usr/bin/env python3
"""One clean Release/production gate plus the normal M43 editor/runtime paths.
Historical output is protected through the existing output-location adapters.
"""
import importlib.util, json, os, shutil, subprocess, sys, time, hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/evidence/m43/final'
def main():
    assert not OUT.exists(), 'Do not overwrite evidence'
    backup=ROOT/'.cache/m43-before-clean-release'
    assert not backup.exists()
    (ROOT/'.cache').mkdir(exist_ok=True)
    if (ROOT/'build').exists(): (ROOT/'build').rename(backup)
    OUT.mkdir(parents=True)
    start=time.monotonic()
    spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py')
    gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
    gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests',
                            'judas_particle_application_tests','judas_package_application_tests','judas_classification_tests',
                            'judas_script_application_tests','judas_runtime_ui_tests','judas_runtime_ui_application_tests','judas_touch_event_tests','judas_touch_application_tests','judas_scene_transition_tests'})
    original=subprocess.run
    def release(command,*args,**kwargs):
        if command==['cmake','-S','.','-B','build']:command.append('-DCMAKE_BUILD_TYPE=Release')
        return original(command,*args,**kwargs)
    subprocess.run=release
    try: regression=gate.production(OUT,4)
    finally: subprocess.run=original
    results={'clean_release':True,'regression':regression,'commands':[]}
    (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
    if not regression['overall_pass']:return 1
    env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')}
    env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none')
    def run(name,command,extra):
        before=time.monotonic();process=original(command,cwd=ROOT,env=dict(env,**extra),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (OUT/(name+'.log')).write_text(process.stdout)
        results['commands'].append({'name':name,'command':command,'exit_code':process.returncode,'seconds':time.monotonic()-before})
        (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
        assert process.returncode==0,name+' failed'
        return process.stdout
    project=ROOT/'build/m43-editor-project';shutil.copytree(ROOT/'projects/scene_demo',project)
    text=run('demo_editor',['./build/judas_editor',str(project/'scene_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')})
    assert 'authored scene after play/stop is IDENTICAL' in text
    script=OUT/'demo_steps.txt';script.write_text('STEPS 3\nLOG_EVERY 1\nSCREENSHOT 2 '+str(OUT/'demo.png')+'\n')
    run('demo_standalone',['./build/judas','projects/scene_demo/scene_demo.judasproj'],{'JUDAS_TEST_SCRIPT':str(script)})
    # Exercise a moved package's real standalone loop, with project-owned JS
    # requesting A -> B -> A and quit (no runtime/harness bypass).
    fixture=ROOT/'build/m43-export-project';shutil.copytree(ROOT/'projects/scene_demo',fixture)
    hud=fixture/'Assets/scripts/hud.js';source=hud.read_text()
    needle="this.hud.get('pause_options').text='Reload this scene';}"
    assert needle in source
    source=source.replace(needle,"""this.hud.get('pause_options').text='Reload this scene';
    if(!session.get('exportStarted')){session.set('exportStarted',true);this.spawn();}
    else if(scenes.current==='Scenes/b.judas'){console.log('M43_PACKAGE_B key='+session.get('hasKey'));scenes.load('Scenes/a.judas');}
    else {console.log('M43_PACKAGE_A session='+session.get('hasKey'));ui.quit();}}
    """)
    hud.write_text(source)
    package=ROOT/'.cache/m43-export-package';moved=ROOT/'.cache/m43-moved-package'
    assert not package.exists() and not moved.exists()
    run('export',['./build/judas_export',str(fixture/'scene_demo.judasproj'),str(package)],{})
    assert (package/'Scenes/b.judas').is_file()
    package.rename(moved)
    before=time.monotonic()
    process=original([str(moved/'judas')],cwd='/tmp',env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=60)
    (OUT/'moved_package.log').write_text(process.stdout)
    assert process.returncode==0 and 'M43_PACKAGE_B key=true' in process.stdout and 'M43_PACKAGE_A session=true' in process.stdout
    results['commands'].append({'name':'moved_package_transitions','command':[str(moved/'judas')],'cwd':'/tmp','exit_code':process.returncode,'seconds':time.monotonic()-before})
    results.update(seconds=time.monotonic()-start,human_visual_validation='PENDING; operator authority')
    changed=subprocess.check_output(['git','diff','--name-only'],cwd=ROOT,text=True).splitlines()
    changed+=subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
    sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(set(changed)) if (ROOT/p).is_file() and (p.startswith(('src/','tests/','scripts/','projects/scene_demo/')) or p=='CMakeLists.txt')}
    (OUT/'SOURCE_SHA256.json').write_text(json.dumps(sources,indent=2)+'\n')
    (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps(results,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
