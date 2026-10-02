#!/usr/bin/env python3
"""One clean Release/production gate, then the actual editor export and moved game.
Reuses existing production validation without changing its assertions. Output
adapters protect historical evidence. No binaries are stored in evidence.
"""
import importlib.util, json, os, shutil, subprocess, sys, time, hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/evidence/m38/final'

def main():
    retry = '--export-only' in sys.argv
    start=time.monotonic()
    original=subprocess.run
    if retry:
        results=json.loads((OUT/'RESULTS.json').read_text())
        assert results['regression']['overall_pass']
        results['narrow_correction']='Editor export automation request moved before ImGui::Render; export algorithm unchanged. Only affected editor/export/startup checks repeated.'
    else:
        assert not OUT.exists(), 'Do not overwrite evidence'
        OUT.mkdir(parents=True)
        backup=ROOT/'.cache/m38-before-clean-release'
        assert not backup.exists()
        (ROOT/'.cache').mkdir(exist_ok=True)
        if (ROOT/'build').exists(): (ROOT/'build').rename(backup)
        spec=importlib.util.spec_from_file_location('existing_gate',ROOT/'scripts/ftft9_validation.py')
        gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
        gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests',
                                'judas_particle_application_tests','judas_package_application_tests'})
        def release(command,*args,**kwargs):
            if command==['cmake','-S','.','-B','build']: command.append('-DCMAKE_BUILD_TYPE=Release')
            return original(command,*args,**kwargs)
        subprocess.run=release
        try: regression=gate.production(OUT,4)
        finally: subprocess.run=original
        results={'clean_release':True,'regression':regression,'commands':[]}
        (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
        if not regression['overall_pass']: return 1
    env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')}
    env.update(SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',SDL_AUDIODRIVER='dummy')
    def run(name,command,extra=None,cwd=ROOT):
        before=time.monotonic()
        process=original(command,cwd=cwd,env=dict(env,**(extra or {})),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (OUT/(name+("_retry" if retry else "")+'.log')).write_text(process.stdout)
        results['commands'].append({'name':name,'command':list(map(str,command)),'cwd':str(cwd),'exit_code':process.returncode,'seconds':time.monotonic()-before,'environment':extra or {}})
        (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
        if process.returncode: raise RuntimeError(name+' failed')
        return process.stdout
    # The editor automation acts through the same Export project request/button
    # and runs its existing Play/Stop workflow on a copied authored project.
    editor_project=ROOT/'build/m38-editor-project'
    if editor_project.exists(): shutil.rmtree(editor_project)
    shutil.copytree(ROOT/'projects/export_demo',editor_project)
    package=ROOT/'.cache/m38-game'
    text=run('editor_export',['./build/judas_editor',str(editor_project/'export_demo.judasproj')],
        {'JUDAS_EDITOR_AUTOTEST':str(OUT/('editor_retry' if retry else 'editor')),'JUDAS_EDITOR_AUTOTEST_EXPORT':str(package),'JUDAS_WORLD_STATE':'none'})
    assert '[editor autotest] export project: Exported ' in text
    assert 'authored scene after play/stop is IDENTICAL' in text
    # Move the actual exported Release executable and all data out of both
    # source/build trees. The normal copied binary selects package startup even
    # with a poisoned repository override and unrelated cwd. This small existing
    # script is a blocking screenshot/startup reference, NOT async acceptance.
    moved=Path('/tmp')/('Judas_M38_Moved_'+str(os.getpid()))
    shutil.move(package,moved)
    script=OUT/'three_steps.txt';script.write_text('STEPS 3\nLOG_EVERY 1\nSCREENSHOT 2 '+str(OUT/'exported-runtime.png')+'\n')
    text=run('exported_runtime',[str(moved/'judas')],{'JUDAS_TEST_SCRIPT':str(script),'JUDAS_ENGINE_ROOT':'/not/the/repository','XDG_DATA_HOME':str(OUT/'runtime-user-data')},Path('/'))
    assert 'Scene: Judas exported game ('+str(moved) in text
    # Keep an easily launched review package in ignored storage; package itself
    # is not evidence committed to Git. Operator can copy it elsewhere again.
    review=ROOT/'.cache/M38-Judas-Export-Demo'
    if review.exists(): shutil.rmtree(review)
    shutil.move(moved,review)
    results.update(seconds=time.monotonic()-start,review_package=str(review),
                   package_bytes=sum(p.stat().st_size for p in review.rglob('*') if p.is_file()),
                   registered_assets=len(list(review.rglob('*.judasmeta'))),
                   human_visual_and_listening_validation='PENDING; operator authority')
    (OUT/'RESULTS.json').write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps(results,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
