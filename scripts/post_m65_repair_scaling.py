#!/usr/bin/env python3
"""Unchanged S1 generator; fresh temporary copies and unique measurements."""
import os,subprocess,tempfile,shutil,time,json,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1]
source=R/'docs/evidence/post_m65_consumers/scaling/original-reproduction'
name=sys.argv[1];binary=Path(sys.argv[2]).resolve();out=R/'docs/evidence/post_m65_repairs/scaling'/name
out.mkdir(parents=True,exist_ok=False)
env=dict(os.environ,SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',JUDAS_PROFILE='0',JUDAS_METADATA_TRACE='1',JUDAS_WORLD_STATE='none',JUDAS_ENGINE_ROOT=str(R))
results=[]
with tempfile.TemporaryDirectory(prefix='judas-s1-') as tmp:
 p=Path(tmp)/'project';shutil.copytree(source,p)
 for n in (0,400,800,1600):
  for scripted in (0,1):
   subprocess.run([sys.executable,'make.py',str(n),str(scripted)],cwd=p,check=True)
   stem=f'{n}-{scripted}'
   for steps in (1,3000):
    commands=p/'run.txt';commands.write_text(f'STEPS {steps}\nLOG_EVERY 1000000\n')
    env['JUDAS_TEST_SCRIPT']=str(commands)
    t=time.perf_counter();r=subprocess.run([str(binary),str(p/'repro.judasproj')],cwd='/tmp',env=env,capture_output=True,text=True,timeout=300)
    elapsed=time.perf_counter()-t
    (out/f'{stem}-{steps}.log').write_text(r.stdout+r.stderr)
    record=dict(entities=n,scripted=bool(scripted),steps=steps,elapsed=elapsed,exit=r.returncode)
    results.append(record);(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(record,flush=True)
    assert r.returncode==0
