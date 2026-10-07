#!/usr/bin/env python3
"""Preserve Void Courier S1's inert-entity reproduction on accepted M65.

Same 3000 fixed steps, one empty fixedUpdate versus none, no draw submissions.
Outputs are new evidence; no engine optimization is performed here.
"""
from pathlib import Path
import gzip, hashlib, json, os, shutil, subprocess, tempfile, time
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/evidence/post_m65_consumers/scaling'
SOURCE=Path('/home/conner/Documents/GitHub/ClaudesEdge/notes/repro_entity_scaling')
def main():
    OUT.mkdir(parents=True,exist_ok=False)
    shutil.copytree(SOURCE,OUT/'original-reproduction')
    # Remove source-generated variants: make.py is the reproducible authority.
    results=[]
    with tempfile.TemporaryDirectory(prefix='judas-consumer-scaling-') as tmp:
        p=Path(tmp);shutil.copytree(SOURCE,p,dirs_exist_ok=True)
        env={k:v for k,v in os.environ.items() if not k.startswith(('JUDAS_','FTFT_'))}
        env.update(SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',JUDAS_PROFILE='1',JUDAS_TEST_SCRIPT=str(p/'steps.txt'))
        for scripted in [False,True]:
            for n in [0,400,800]:
                name=f'{n}-'+('script' if scripted else 'inert')
                subprocess.run(['python3',str(p/'make.py'),str(n),str(int(scripted))],cwd=p,check=True)
                env['JUDAS_PROFILE_OUTPUT']=str(OUT/(name+'-profile.json'))
                begin=time.perf_counter()
                run=subprocess.run([str(ROOT/'build/judas'),str(p/'repro.judasproj')],cwd='/tmp',env=env,capture_output=True,text=True,timeout=120)
                elapsed=time.perf_counter()-begin
                (OUT/(name+'.log')).write_text(run.stdout+run.stderr)
                assert run.returncode==0,(name,run.stderr)
                results.append({'entities':n,'scripted':scripted,'steps':3000,'elapsed_seconds':elapsed})
                print(name,round(elapsed,4),flush=True)
    (OUT/'measurements.json').write_text(json.dumps({'runtime_sha256':hashlib.sha256((ROOT/'build/judas').read_bytes()).hexdigest(),'results':results,'policy':'Original minimal reproduction; sequential processes; controlled harness clock; no GL draw pass, includes startup, profiler and teardown.'},indent=2)+'\n')
if __name__=='__main__':main()
