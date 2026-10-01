#!/usr/bin/env python3
"""One clean Release build and existing production/editor/runtime validation.
Only evidence output locations and reviewed shared-source exceptions differ.
"""
import importlib.util,json,subprocess,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/evidence/m37/final'
def main():
    assert not OUT.exists(),'Do not overwrite evidence'
    backup=ROOT/'.cache/m37-before-clean-release'
    assert not backup.exists()
    (ROOT/'.cache').mkdir(exist_ok=True)
    if (ROOT/'build').exists():(ROOT/'build').rename(backup)
    OUT.mkdir(parents=True)
    spec=importlib.util.spec_from_file_location('existing_gate',ROOT/'scripts/ftft9_validation.py')
    gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
    gate.OUTPUT_TESTS.update({'judas_render_camera_tests','judas_visual_particle_tests','judas_particle_application_tests'})
    original=subprocess.run
    def release(command,*args,**kwargs):
        if command==['cmake','-S','.','-B','build']:command.append('-DCMAKE_BUILD_TYPE=Release')
        return original(command,*args,**kwargs)
    start=time.monotonic();subprocess.run=release
    try:result=gate.production(OUT,4)
    finally:subprocess.run=original
    (OUT/'M37_RESULTS.json').write_text(json.dumps({'clean_release':True,'regression':result,'seconds':time.monotonic()-start,'human_visual_validation':'PENDING; operator authority'},indent=2)+'\n')
    return 0 if result['overall_pass'] else 1
if __name__=='__main__':raise SystemExit(main())
