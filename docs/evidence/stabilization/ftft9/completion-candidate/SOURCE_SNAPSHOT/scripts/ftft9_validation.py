#!/usr/bin/env python3
"""FTFT9 actual hybrid integration and preserved production/FTFT1--3 gates.

Historical scripts/assertions are unchanged. Explicit adapters only relocate
output, allow reviewed scene-metadata source edits, and generate current-schema
valid saves for the old FTFT1 application smoke test.
"""
import argparse
from contextlib import redirect_stdout, redirect_stderr
import json
from pathlib import Path
import re
import subprocess
import sys
import time
import types
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
import importlib.util
spec=importlib.util.spec_from_file_location('ftft9_existing',ROOT/'scripts/ftft4_validation.py')
BASE=importlib.util.module_from_spec(spec);spec.loader.exec_module(BASE)
REVIEWED_SHARED={'src/RuntimeWorld.cpp','src/RuntimeWorld.h','src/SceneFingerprint.cpp','src/SceneFingerprint.h'}
OUTPUT_TESTS={'judas_production_fluid_tests','judas_fluid_container_integration_tests',
              'judas_player_fluid_integration_tests','judas_combustion_integration_tests',
              'judas_thermal_accounting_tests','judas_lifecycle_integration_tests',
              'judas_orbital_integrity_tests','judas_spacecraft_frame_integration_tests'}


def production(directory,jobs):
    original_load,original_run=BASE.load_module,subprocess.run
    adapters=[]
    def load(name,path):
        module=original_load(name,path)
        if path.name=='ftft2_validation.py':
            module.PROTECTED=[p for p in module.PROTECTED if p not in REVIEWED_SHARED]
        return module
    def run(command,*args,**kwargs):
        if isinstance(command,list) and command and Path(command[0]).name in OUTPUT_TESTS and '--output' not in command:
            command.extend(['--output',str(directory/'fixture-details'/Path(command[0]).name)])
            adapters.append(list(command))
        return original_run(command,*args,**kwargs)
    try:
        BASE.load_module=load;subprocess.run=run
        record,result=BASE.reuse_ftft2(directory,jobs)
    finally:
        BASE.load_module=original_load;subprocess.run=original_run
    record.update(reviewed_shared_source_exceptions=sorted(REVIEWED_SHARED),fixture_output_adapters=adapters,
                  production_suite_count=result.get('production_suite_count'),integration_assertions=result.get('integration_assertions'))
    return record


def persistence(directory):
    original=ROOT/'docs/evidence/ftft1/runtime_smoke/run.py'
    source=original.read_text()
    version=int(re.search(r'kSceneFingerprintVersion\s*=\s*(\d+)',(ROOT/'src/SceneFingerprint.h').read_text())[1])
    needle='compatibility 1 sha256'
    if source.count(needle)!=1:raise RuntimeError('Historical smoke format changed; inspect adapter')
    adapted=source.replace(needle,f'compatibility {version} sha256')
    module=types.ModuleType('ftft9_current_schema_smoke');module.__file__=str(original)
    exec(compile(adapted,str(original),'exec'),module.__dict__)
    module.OUT=directory/'ftft1-current-schema';module.OUT.mkdir()
    (module.OUT/'fingerprint_tool.cpp').write_bytes((original.parent/'fingerprint_tool.cpp').read_bytes())
    with (directory/'ftft1_reuse.log').open('w') as stream,redirect_stdout(stream),redirect_stderr(stream):code=module.main()
    result=json.loads((module.OUT/'results.json').read_text())
    return {'overall_pass':code==0 and result['automated_acceptance'],'exit_code':code,
            'historical_script_sha256':BASE.sha(original),'compatibility_fixture_schema':version,
            'adapter':'Only the generated valid/mismatch save header uses the current canonical schema. Runtime/editor assertions and legacy-file rejection are unchanged.',
            'results':str((module.OUT/'results.json').relative_to(ROOT))}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--jobs',type=int,default=4)
    args=p.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    start=time.monotonic();before=BASE.fingerprints();protected=BASE.protection()
    result={'head':BASE.git('rev-parse','HEAD'),'source_sha256_before':before,'commands':[],'checks':{},'human_visual_validation':'NOT RUN'}
    def save():BASE.save(out/'results.json',result)
    try:
        directory=out/'regressions';directory.mkdir()
        result['regressions']=production(directory,args.jobs);save()
        result['checks']['all_production_and_async']=result['regressions']['overall_pass']
        result['ftft1']=persistence(directory);result['checks']['ftft1']=result['ftft1']['overall_pass'];save()
        result['walks'],_=BASE.walks(out/'harness');result['checks']['near_far']=all(result['walks']['checks'].values())
        result['checks']['source_unchanged_during_run']=before==BASE.fingerprints()
        result['checks']['protected_unchanged_during_run']=protected==BASE.protection()
        result['overall_pass']=all(result['checks'].values())
    except Exception as error:result['overall_pass']=False;result['failure']=str(error)
    result['source_sha256_after']=BASE.fingerprints();result['seconds']=time.monotonic()-start;result['git_status']=BASE.git('status','--short');save()
    print(json.dumps({k:result[k] for k in ['overall_pass','checks','seconds']}));return 0 if result['overall_pass'] else 1
if __name__=='__main__':raise SystemExit(main())
