#!/usr/bin/env python3
"""Build/run the focused and full relevant FTFT1 suites; no prototype runs."""
import hashlib,json,pathlib,re,subprocess,time
ROOT=pathlib.Path(__file__).resolve().parents[3]
OUT=pathlib.Path(__file__).resolve().parent
TARGETS=['judas_scene_fingerprint_tests','judas_world_state_tests','judas_lifecycle_tests','judas_scene_tests','judas_project_tests']
results=[]
def run(command,name):
    start=time.monotonic()
    with (OUT/(name+'.log')).open('w') as log:
        result=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    text=(OUT/(name+'.log')).read_text()
    item={'name':name,'command':command,'exit_code':result.returncode,'elapsed_seconds':time.monotonic()-start}
    if name in TARGETS:
        item['passing_checks']=len(re.findall(r'^\s*(?:PASS|OK)\s+',text,re.M))
        item['failing_checks']=len(re.findall(r'^\s*FAIL\s+',text,re.M))
        item['executable_sha256']=hashlib.sha256((ROOT/'build'/name).read_bytes()).hexdigest()
    results.append(item)
    (OUT/'regressions.json').write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps(item),flush=True)
    if result.returncode:raise SystemExit(result.returncode)
run(['cmake','-S','.','-B','build'],'configure')
run(['cmake','--build','build','--target',*TARGETS,'judas','judas_editor','-j4'],'build')
for target in TARGETS:run(['./build/'+target],target)
subprocess.run(['git','diff','--exit-code','587759e93add89f32774cbc51fe47878fab127ec','--','prototypes/fluid_coupling/r1_p1/'],cwd=ROOT,check=True)
print('FTFT1 regressions PASS; protected prototype tree unchanged')
