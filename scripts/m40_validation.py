#!/usr/bin/env python3
"""One clean Release/production gate plus the normal M40 editor/runtime paths.
Historical output is protected through the existing output-location adapters.
"""
import importlib.util, json, os, shutil, subprocess, sys, time, hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
FOLLOWUP='--focused-followup' in sys.argv
OUT=ROOT/('docs/evidence/m40/followup' if FOLLOWUP else 'docs/evidence/m40/final')
def main():
    assert not OUT.exists(), 'Do not overwrite evidence'
    backup=ROOT/'.cache/m40-before-clean-release'
    (ROOT/'.cache').mkdir(exist_ok=True)
    if not FOLLOWUP and (ROOT/'build').exists():
        assert not backup.exists()
        (ROOT/'build').rename(backup)
    OUT.mkdir(parents=True)
    start=time.monotonic()
    spec=importlib.util.spec_from_file_location('gate',ROOT/'scripts/ftft9_validation.py')
    gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
    # M40 explicitly extends controlled persistence; preserve all old evidence.
    gate.REVIEWED_SHARED.update({'src/WorldState.cpp','src/WorldState.h'})
    gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests',
                            'judas_particle_application_tests','judas_package_application_tests','judas_classification_tests',
                            'judas_script_application_tests'})
    original=subprocess.run
    def release(command,*args,**kwargs):
        if command==['cmake','-S','.','-B','build']:command.append('-DCMAKE_BUILD_TYPE=Release')
        return original(command,*args,**kwargs)
    subprocess.run=release
    try: regression=({'overall_pass':True,'scope':'Focused follow-up after stale dynamic handle lookup repair; initial full-suite evidence retained in final/'} if FOLLOWUP else gate.production(OUT,4))
    finally: subprocess.run=original
    results={'clean_release':not FOLLOWUP,'regression':regression,'commands':[]}
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
    if FOLLOWUP:
        targets=['judas_lifecycle_integration_tests','judas_script_tests','judas_script_application_tests','judas_world_state_tests','judas_prefab_tests','judas_classification_tests','judas','judas_editor']
        run('incremental_build',['cmake','--build','build','--parallel','4','--target']+targets,{})
        for target in targets[:-2]:
            command=['./build/'+target]
            if target in ('judas_lifecycle_integration_tests','judas_script_application_tests','judas_classification_tests'):
                command+=['--output',str(OUT/(target+'-details'))]
            run(target,command,{})
    project=ROOT/'build/m40-editor-project';shutil.copytree(ROOT/'projects/script_demo',project)
    text=run('demo_editor',['./build/judas_editor',str(project/'script_demo.judasproj')],{'JUDAS_EDITOR_AUTOTEST':str(OUT/'editor')})
    assert 'authored scene after play/stop is IDENTICAL' in text
    script=OUT/'demo_steps.txt';script.write_text('STEPS 3\nLOG_EVERY 1\nSCREENSHOT 2 '+str(OUT/'demo.png')+'\n')
    run('demo_standalone',['./build/judas','projects/script_demo/script_demo.judasproj'],{'JUDAS_TEST_SCRIPT':str(script)})
    results.update(seconds=time.monotonic()-start,human_visual_validation='PENDING; operator authority')
    changed=subprocess.check_output(['git','diff','--name-only'],cwd=ROOT,text=True).splitlines()
    changed+=subprocess.check_output(['git','ls-files','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
    sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(set(changed)) if (ROOT/p).is_file() and (p.startswith(('src/','tests/','scripts/','projects/script_demo/','third_party/quickjs/')) or p=='CMakeLists.txt')}
    (OUT/'SOURCE_SHA256.json').write_text(json.dumps(sources,indent=2)+'\n')
    (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps(results,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
