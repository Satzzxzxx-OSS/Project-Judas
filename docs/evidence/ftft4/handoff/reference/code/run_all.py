"""Reproduce independent FTFT4 research checks. No access to Judas required.
Never labels this as a production verification. Requires numpy/mpmath and g++.
"""
from pathlib import Path
import subprocess,sys,json,time,platform,os
import numpy,mpmath
ROOT=Path(__file__).resolve().parents[1]
def main():
    (ROOT/'results').mkdir(exist_ok=True);build=ROOT/'.build';build.mkdir(exist_ok=True)
    env=dict(os.environ,OPENBLAS_NUM_THREADS='1',OMP_NUM_THREADS='1')
    commands=[];start=time.perf_counter()
    for name in ['geometry_checks.py','distance_checks.py','impact_checks.py','broadphase_checks.py']:
        command=[sys.executable,str(ROOT/'code'/name)]
        p=subprocess.run(command,env=env,text=True,capture_output=True,check=True)
        (ROOT/'results'/f'{name[:-3]}_stdout.txt').write_text(p.stdout+p.stderr)
        commands.append({'target':name,'exit':p.returncode})
    compile_cmd=['g++','-O2','-std=c++17','-fno-fast-math',str(ROOT/'code/gamma_probe.cpp'),'-o',str(build/'gamma_probe')]
    subprocess.run(compile_cmd,env=env,check=True,capture_output=True)
    p=subprocess.run([str(build/'gamma_probe')],env=env,text=True,check=True,capture_output=True)
    (ROOT/'results/gamma_cpp.csv').write_text(p.stdout)
    ver=subprocess.run(['g++','--version'],text=True,check=True,capture_output=True).stdout.splitlines()[0]
    metadata={'scope':'INDEPENDENT REFERENCE RESEARCH, NOT JUDAS SOURCE AUDIT OR PRODUCTION RUN',
              'judas_source_fetched':False,'judas_source_compiled':False,'judas_tree_adapter_executed':False,
              'last_user_reported_commit':'3404388f1af48419505c802fc11dbf8a0b577f38',
              'python':platform.python_version(),'numpy':numpy.__version__,'mpmath':mpmath.__version__,
              'compiler':ver,'cpp_flags':compile_cmd[1:4],'commands':commands,
              'cpp_gamma_cases':5,'seconds':time.perf_counter()-start}
    (ROOT/'results/RUN_METADATA.json').write_text(json.dumps(metadata,indent=2))
    print(json.dumps(metadata,indent=2))
if __name__=='__main__':main()
