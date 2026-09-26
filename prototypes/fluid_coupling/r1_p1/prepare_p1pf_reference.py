#!/usr/bin/env python3
"""Export reference RNG INPUTS and rerun unchanged P1-PF Python scripts in a copy.

Requires NumPy 2.3.5; full reruns also require SciPy 1.17.0. A prototype-local
.reference-p1pf/ package directory is used when present. --inputs-only does not
execute a projection and needs no SciPy. No reference answers enter the C++ input
CSV. The original handoff tree is never written.
"""
from __future__ import annotations
import argparse
import ast
import csv
import dataclasses
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import types
from typing import Callable
import zipfile
import numpy as np

P=Path(__file__).resolve().parent
H=P/'evidence/p1pf/handoff'
OUT=P/'evidence/p1pf/reference_rerun'
COPY=OUT/'execution'
INPUT=P/'fixtures/p1pf_random_inputs.csv'

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def dump(path,value):path.write_text(json.dumps(value,indent=2)+'\n')

def integrity():
    entries=json.loads((H/'SHA256.json').read_text())
    verified=[]
    for name,expected in entries.items():
        actual=sha(H/name)
        if actual!=expected:raise ValueError(f'Handoff integrity failure: {name}')
        verified.append({'path':name,'sha256':actual})
    nested=[]
    original=H/'evidence/original/Judas_Frozen_Geometry_Projection_Research.zip'
    with zipfile.ZipFile(original) as z:
        manifests=[n for n in z.namelist() if n.endswith('SHA256.json')]
        if len(manifests)!=1:raise ValueError('Expected one original archive manifest')
        name=manifests[0];base=Path(name).parent
        for item,expected in json.loads(z.read(name)).items():
            target=(base/item).as_posix();actual=hashlib.sha256(z.read(target)).hexdigest()
            if actual!=expected:raise ValueError(f'Original archive integrity failure: {target}')
            nested.append({'path':target,'sha256':actual})
    result={'status':'PASS','handoff_entries':verified,'original_archive_entries':nested}
    dump(OUT/'integrity.json',result)
    return result

def geometry_classes():
    # Unchanged definitions are loaded only to count DOFs for RNG shapes. Neither
    # project() nor main() is invoked by this loader; linalg is not substituted.
    path=H/'reference/projection_checks.py';tree=ast.parse(path.read_text())
    wanted={'cross','IncompatibleConstraints','Body','Face','Projection','FrozenMAC','geometry_body'}
    selected=[n for n in tree.body if isinstance(n,(ast.ClassDef,ast.FunctionDef)) and n.name in wanted]
    if {n.name for n in selected}!=wanted:raise ValueError('Reference geometry definitions changed')
    future=ast.ImportFrom(module='__future__',names=[ast.alias(name='annotations')],level=0)
    module=types.ModuleType('_p1pf_input_geometry');sys.modules[module.__name__]=module
    module.__dict__.update(np=np,math=math,dataclass=dataclasses.dataclass,Callable=Callable,__file__=str(path))
    code=ast.fix_missing_locations(ast.Module(body=[future]+selected,type_ignores=[]))
    exec(compile(code,str(path),'exec'),module.__dict__)
    return module

def export_inputs():
    if np.__version__!='2.3.5':raise RuntimeError('Input export requires supplied NumPy 2.3.5 RNG version')
    ref=geometry_classes();groups={};description={}
    rng=np.random.default_rng(418)
    groups['random_fluid_rho']=rng.random((16,16))
    labels=np.full((8,8),-1)
    closed=ref.FrozenMAC(labels,np.ones((16,16)))
    opened=ref.FrozenMAC(labels,np.ones((16,16)),boundary={'top':'pressure'})
    groups['random_fluid_closed_w']=rng.normal(size=len(closed.H))
    groups['random_fluid_open_w']=rng.normal(size=len(opened.H))
    for name in ('random_fluid_rho','random_fluid_closed_w','random_fluid_open_w'):
        description[name]='seed 418; draws sequentially: 16x16 uniform rho then closed normal velocity then open normal velocity; reference resets this RNG for each density contrast'
    rng=np.random.default_rng(561)
    groups['covariance_rho_random']=rng.random((24,24))
    labels=np.full((12,12),-1);labels[5:7,3:5]=0
    body=ref.geometry_body(labels,0,700.)
    geometry=ref.FrozenMAC(labels,np.ones((24,24)),{0:body})
    groups['covariance_w']=rng.normal(size=len(geometry.H))
    for name in ('covariance_rho_random','covariance_w'):
        description[name]='seed 561; 24x24 uniform density draw followed by one normal generalized velocity draw; covariance body mask x[5:7],y[3:5]'
    labels=np.full((10,10),-1);labels[2:4,3:5]=0;labels[6:8,6:8]=1
    bodies={0:ref.geometry_body(labels,0,1.),1:ref.geometry_body(labels,1,1e4)}
    geometry=ref.FrozenMAC(labels,np.ones((20,20)),bodies)
    groups['two_bodies_w']=np.random.default_rng(151).normal(size=len(geometry.H))
    description['two_bodies_w']='seed 151; normal generalized velocity draw; two interior 2x2-cell bodies'
    expected={'random_fluid_rho':256,'random_fluid_closed_w':144,'random_fluid_open_w':144,'covariance_rho_random':576,'covariance_w':311,'two_bodies_w':218}
    assert {name:a.size for name,a in groups.items()}==expected
    INPUT.parent.mkdir(parents=True,exist_ok=True)
    with INPUT.open('w',newline='') as file:
        writer=csv.writer(file,lineterminator='\n');writer.writerow(['group','index','value'])
        for name,array in groups.items():
            for index,value in enumerate(array.ravel(order='C')):writer.writerow([name,index,format(float(value),'.17g')])
    metadata={'scope':'Seeded reference INPUTS only. No pressure, projected velocity, force, torque or acceptance answers.',
              'numpy':np.__version__,'reference_source_sha256':sha(H/'reference/projection_checks.py'),
              'generator_source_sha256':sha(Path(__file__)),'csv_sha256':sha(INPUT),'total_values':sum(expected.values()),
              'density_layout':'Reference arrays are rho[x_index,y_index], flattened C order: index=x*ny+y. Values are unit random inputs: rho=1+(contrast-1)*input.',
              'velocity_layout':'Reference FrozenMAC: faces ordered axis then x index then y index; omit faces with no adjacent fluid. Then each body sorted by ID contributes world Vx,Vy,omega.',
              'geometry_loader':'Unchanged AST class/function definitions used only for DOF counts. No projection or expected-output calculation in input export.',
              'groups':{name:{'count':a.size,'shape':list(a.shape),'construction':description[name]} for name,a in groups.items()}}
    dump(OUT/'INPUTS_METADATA.json',metadata)
    return metadata

def compare(actual,expected):
    # Differences between independent builds may be roundoff; original executed
    # script assertions retain the physical gates. This reports, never conceals,
    # non-timing differences against supplied output.
    a=dict(actual);b=dict(expected);a.pop('seconds',None);b.pop('seconds',None)
    numeric=[];other=[]
    def walk(x,y,path):
        if isinstance(x,dict) and isinstance(y,dict):
            if x.keys()!=y.keys():other.append(path+':keys')
            for k in x.keys()&y.keys():walk(x[k],y[k],path+'/'+k)
        elif isinstance(x,list) and isinstance(y,list):
            if len(x)!=len(y):other.append(path+':length')
            for i,(u,v) in enumerate(zip(x,y)):walk(u,v,path+'/'+str(i))
        elif isinstance(x,(float,int)) and not isinstance(x,bool) and isinstance(y,(float,int)) and not isinstance(y,bool):
            if x!=y:numeric.append({'path':path,'actual':x,'supplied':y,'absolute_difference':abs(x-y),'difference_over_max_1_magnitudes':abs(x-y)/max(1.,abs(x),abs(y))})
        elif x!=y:other.append(path)
    walk(a,b,'')
    return {'equal_excluding_seconds':a==b,'differing_numeric_fields':len(numeric),'maximum_absolute_numeric_difference':max((x['absolute_difference'] for x in numeric),default=0.),'maximum_scaled_numeric_difference':max((x['difference_over_max_1_magnitudes'] for x in numeric),default=0.),'numeric_differences':numeric,'other_differences':other}

def full_rerun():
    COPY.mkdir(parents=True,exist_ok=True)
    # Copy only required unchanged source/dependencies, never inherited metadata
    # or supplied result files that would become stale when a rerun writes output.
    for name in ('projection_checks.py','adapter_reference_checks.py','compatibility_checks.py','requirements.txt'):
        shutil.copy2(H/'reference'/name,COPY/name)
    env=os.environ.copy();env.update(OPENBLAS_NUM_THREADS='1',OMP_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
    package=P/'.reference-p1pf'
    if package.exists():env['PYTHONPATH']=str(package)+(os.pathsep+env['PYTHONPATH'] if env.get('PYTHONPATH') else '')
    version=json.loads(subprocess.check_output([sys.executable,'-c','import json,numpy,scipy; print(json.dumps({"numpy":numpy.__version__,"scipy":scipy.__version__}))'],env=env,text=True))
    if version!={'numpy':'2.3.5','scipy':'1.17.0'}:raise RuntimeError(f'Reference dependency versions differ: {version}')
    runs={}
    for script,result,log in [('projection_checks.py','projection_results.json','projection_run_output.txt'),('adapter_reference_checks.py','adapter_reference_results.json','adapter_run_output.txt')]:
        assert sha(COPY/script)==sha(H/'reference'/script)
        with (OUT/log).open('w') as output:completed=subprocess.run([sys.executable,script],cwd=COPY,env=env,stdout=output,stderr=subprocess.STDOUT)
        if completed.returncode:raise RuntimeError(f'Unchanged {script} failed; see {OUT/log}')
        payload=json.loads((COPY/result).read_text());supplied=json.loads((H/'reference'/result).read_text())
        assert payload['status']=='PASS'
        if script=='projection_checks.py':assert payload['cases']==42 and payload['negative_controls']==5
        else:assert payload['cases']==6 and payload['transport_timesteps']==72
        comparison=compare(payload,supplied);dump(OUT/(Path(result).stem+'_comparison.json'),comparison)
        runs[script]={'command':[sys.executable,script],'script_sha256':sha(COPY/script),'result_sha256':sha(COPY/result),'log_sha256':sha(OUT/log),'status':payload['status'],'cases':payload['cases'],'seconds':payload['seconds'],'comparison_equal_excluding_seconds':comparison['equal_excluding_seconds']}
    for name in ('projection_checks.py','adapter_reference_checks.py','compatibility_checks.py'):
        assert sha(COPY/name)==sha(H/'reference'/name)
    return version,runs

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--inputs-only',action='store_true');args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True);verified=integrity();inputs=export_inputs()
    metadata={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'python':sys.version,'platform':platform.platform(),'generator_sha256':sha(Path(__file__)),'reference_source_sha256':{n:sha(H/'reference'/n) for n in ('projection_checks.py','adapter_reference_checks.py','compatibility_checks.py')},'input_csv_sha256':sha(INPUT),'input_counts':{k:v['count'] for k,v in inputs['groups'].items()},'handoff_manifest_entries':len(verified['handoff_entries']),'original_manifest_entries':len(verified['original_archive_entries']),'projection_runs_executed':not args.inputs_only,'CXX_acceptance_claimed':False}
    if args.inputs_only:
        metadata['status']='INPUT_EXPORT_ONLY';dump(OUT/'INPUT_EXPORT_RUN.json',metadata)
    else:
        version,runs=full_rerun();metadata.update(status='PASS',dependencies=version,runs=runs);dump(OUT/'RUN_METADATA.json',metadata)
    print(json.dumps(metadata,indent=2))
if __name__=='__main__':main()
