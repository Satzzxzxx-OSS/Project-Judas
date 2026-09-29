#!/usr/bin/env python3
"""FTFT6 real editor/project/runtime and mixed lifecycle validation.

New output directory required; historical evidence is never overwritten.
The editor drives its ordinary requests/Play/frame path. The standalone test
uses Application::Run with the existing clock/observation interface. Neither
implements a second simulation loop. Offscreen GL is not human visual review.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import statistics
import sys
import time
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('ftft6_existing', ROOT/'scripts/ftft4_validation.py')
BASE = importlib.util.module_from_spec(spec)
spec.loader.exec_module(BASE)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--jobs',type=int,default=4)
    p.add_argument('--focused-only',action='store_true',help='Development only; not complete FTFT6 acceptance')
    args=p.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    start=time.monotonic();before=BASE.fingerprints();protected=BASE.protection()
    result={'head':BASE.git('rev-parse','HEAD'),'source_sha256_before':before,'commands':[], 'checks':{},
            'human_visual_validation':'NOT RUN; actual offscreen GL and machine assertions', 'focused_only':args.focused_only}
    def save():BASE.save(out/'results.json',result)
    def run(name,command,extra=None,timeout=1200):
        record,text=BASE.execute(name,command,out,extra,timeout)
        result['commands'].append(record);save()
        if record['exit_code']!=0 or record['failing_lines']:
            raise RuntimeError(name+' failed; raw output preserved')
        return text
    try:
        run('configure',['cmake','-S','.','-B','build','-DCMAKE_BUILD_TYPE=Release'])
        run('build',['cmake','--build','build','-j'+str(args.jobs)])
        editor=out/'editor'
        run('editor-workflow',['./build/judas_editor'],{'JUDAS_EDITOR_STABILIZATION':str(editor)})
        data=json.loads((editor/'result.json').read_text());result['editor']=data
        result['checks']['editor_workflow']=data['overall_pass']
        project=editor/data['project']
        text=run('standalone-editor-project',['./build/judas_project_application_tests',str(project)])
        expected=data['authored_fingerprint']
        result['checks']['same_authored_startup']=('fingerprint='+expected) in text
        times=[float(x) for x in re.findall(r'^FTFT6 FRAME \d+ ([\d.]+)$',text,re.M)]
        result['standalone_frame_ms']={'count':len(times),'median':statistics.median(times),'min':min(times),'max':max(times),'samples':times}
        for target in ['judas_project_application_tests','judas_project_tests','judas_asset_integrity_tests',
                       'judas_lifecycle_integration_tests','judas_lifecycle_tests','judas_world_state_tests',
                       'judas_scene_fingerprint_tests','judas_scene_tests','judas_contact_lifecycle_tests']:
            run(target,['./build/'+target]+(['--output',str(out/'mixed-lifecycle')] if target=='judas_lifecycle_integration_tests' else []))
        if not args.focused_only:
            directory=out/'regressions';directory.mkdir()
            reused,production=BASE.reuse_ftft2(directory,args.jobs)
            result['production_regressions']=reused
            result['production_suite_count']=production.get('production_suite_count')
            result['checks']['production_and_ftft2']=reused['overall_pass']
            result['ftft1']=BASE.reuse_ftft1(directory)
            result['checks']['ftft1']=result['ftft1']['overall_pass']
        result['checks']['source_unchanged_during_run']=before==BASE.fingerprints()
        result['checks']['protected_unchanged_during_run']=protected==BASE.protection()
        result['overall_pass']=all(result['checks'].values())
    except Exception as exc:
        result['overall_pass']=False;result['failure']=str(exc)
    result['seconds']=time.monotonic()-start;result['source_sha256_after']=BASE.fingerprints()
    result['git_status']=BASE.git('status','--short');save()
    print(json.dumps({k:result[k] for k in ['overall_pass','checks','seconds']}))
    return 0 if result['overall_pass'] else 1
if __name__=='__main__':raise SystemExit(main())
