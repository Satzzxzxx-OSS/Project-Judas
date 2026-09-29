#!/usr/bin/env python3
"""FTFT8 physical integration + unchanged production/FTFT1/FTFT2 acceptance."""
import argparse
from contextlib import redirect_stdout, redirect_stderr
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import time
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ftft8_existing',ROOT/'scripts/ftft4_validation.py')
BASE=importlib.util.module_from_spec(spec);spec.loader.exec_module(BASE)


def reused_regressions(directory,jobs):
    record,result=BASE.reuse_ftft2(directory,jobs)
    return dict(record,production_suite_count=result.get('production_suite_count'),
                integration_assertions=result.get('integration_assertions'))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--jobs',type=int,default=4)
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    start=time.monotonic();before=BASE.fingerprints();protected=BASE.protection()
    result={'head':BASE.git('rev-parse','HEAD'),'source_sha256_before':before,'commands':[],'checks':{},'human_visual_validation':'NOT RUN'}
    def save():BASE.save(out/'results.json',result)
    def run(name,command):
        record,text=BASE.execute(name,command,out,timeout=1200);result['commands'].append(record);save()
        if record['exit_code']!=0 or record['failing_lines']:raise RuntimeError(name+' failed; raw results preserved')
        return text
    try:
        run('configure',['cmake','-S','.','-B','build','-DCMAKE_BUILD_TYPE=Release'])
        run('build',['cmake','--build','build','-j'+str(a.jobs)])
        for target in ['judas_thermal_accounting_tests','judas_combustion_integration_tests','judas_combustion_tests',
                       'judas_atmosphere_tests','judas_atmospheric_flight_tests','judas_scene_tests',
                       'judas_world_state_tests','judas_scene_fingerprint_tests']:
            run(target,['./build/'+target]+(['--output',str(out/target)] if target in ['judas_thermal_accounting_tests','judas_combustion_integration_tests'] else []))
        directory=out/'regressions';directory.mkdir()
        result['regressions']=reused_regressions(directory,a.jobs)
        result['checks']['production_and_async']=result['regressions']['overall_pass']
        result['ftft1']=BASE.reuse_ftft1(directory);result['checks']['ftft1']=result['ftft1']['overall_pass']
        result['walks'],_=BASE.walks(out/'harness');result['checks']['near_far']=all(result['walks']['checks'].values())
        result['checks']['source_unchanged_during_run']=before==BASE.fingerprints()
        result['checks']['protected_unchanged_during_run']=protected==BASE.protection()
        result['overall_pass']=all(result['checks'].values())
    except Exception as exc:result['overall_pass']=False;result['failure']=str(exc)
    result['source_sha256_after']=BASE.fingerprints();result['seconds']=time.monotonic()-start;result['git_status']=BASE.git('status','--short');save()
    print(json.dumps({k:result[k] for k in ['overall_pass','checks','seconds']}));return 0 if result['overall_pass'] else 1
if __name__=='__main__':raise SystemExit(main())
