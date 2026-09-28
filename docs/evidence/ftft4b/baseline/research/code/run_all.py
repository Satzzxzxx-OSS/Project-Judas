#!/usr/bin/env python3
"""Run independent references; never builds/changes Judas.
Source-style defects and research-candidate limitations are EXPECTED findings.
"""
import subprocess,sys,pathlib,tempfile,json,hashlib,platform,time,os
ROOT=pathlib.Path(__file__).resolve().parents[1]
NAMES=['event_checks.py','capture_checks.py','induced_impact_checks.py','rotation_checks.py','motion_ledger_checks.py','angular_event_checks.py','diagnose_impact.py']
def main():
 env=os.environ.copy();env.update(OPENBLAS_NUM_THREADS='1',OMP_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
 run=[]
 for name in NAMES:
  t=time.perf_counter();p=subprocess.run([sys.executable,str(ROOT/'code'/name)],env=env,text=True,capture_output=True)
  (ROOT/'results'/(name+'.log')).write_text(p.stdout+p.stderr)
  run.append(dict(program=name,exit=p.returncode,seconds=time.perf_counter()-t))
  if p.returncode:raise RuntimeError(p.stderr)
 with tempfile.TemporaryDirectory(prefix='ftft4b_binary_') as out:
  binary=pathlib.Path(out)/'probe'
  command=['g++','-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(ROOT/'code/source_scalar_probe.cpp'),'-o',str(binary)]
  subprocess.run(command,check=True,capture_output=True,text=True)
  p=subprocess.run([str(binary)],check=True,capture_output=True,text=True)
  (ROOT/'results/source_scalar_probe.csv').write_text(p.stdout)
  compiler=subprocess.run(['g++','--version'],capture_output=True,text=True).stdout.splitlines()[0]
 paths=[p for p in (ROOT/'code').glob('*') if p.is_file()]
 meta=dict(scope='independent component research; no current Judas execution',known_user_checkpoint='422fd275c5f8b54ad4ff37d851eab24a36e8e089',python=sys.version,platform=platform.platform(),compiler=compiler,
      programs=run,source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})
 (ROOT/'results/RUN_METADATA.json').write_text(json.dumps(meta,indent=2))
 print(json.dumps(meta,indent=2))
if __name__=='__main__':main()
