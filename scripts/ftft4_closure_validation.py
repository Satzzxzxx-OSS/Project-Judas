#!/usr/bin/env python3
"""FTFT4 stabilization acceptance against the actual current PhysicsWorld.

Modes are separately reproducible and never overwrite old evidence. Existing
physical witnesses retain their assertions. Performance is seven alternating,
sequential c43/current pairs; total engine steps include all cache preparation.
The whole crate fixture includes an exhaustive oracle and is reported separately.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import statistics
import subprocess
import sys
import time
import zipfile
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
BASELINE = 'c43af4c0b65c597bff7fd03bed55173f5c483139'
BUILD = ROOT / 'build/ftft4-closure'
COMPILER = '/usr/bin/c++'
FLAGS = ['-O3', '-DNDEBUG', '-std=gnu++17', '-Wall', '-Wextra', '-ffp-contract=off']
TARGETS = ['judas_rigid_contact_tests', 'judas_contact_geometry_tests', 'judas_contact_lifecycle_tests',
           'judas_broadphase_tests', 'judas_contact_cache_tests', 'judas_contact_storage_tests',
           'judas_physics_tests', 'judas_lifecycle_tests', 'judas_rigid_motion_tests',
           'judas_impact_timing_tests', 'judas_impact_solver_tests', 'judas_timing_defect_probe',
           'judas_terrain_physics_tests', 'judas_terrain_fluid_tests', 'judas_project_tests']

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def save(path, value): Path(path).write_text(json.dumps(value, indent=2) + '\n')
def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    value = importlib.util.module_from_spec(spec); spec.loader.exec_module(value); return value

def fingerprints(root=ROOT):
    paths = [root / 'CMakeLists.txt']
    paths += [p for d in ['src', 'tests', 'scripts'] for p in (root/d).rglob('*')
              if p.is_file() and '__pycache__' not in p.parts]
    return {str(p.relative_to(root)): sha(p) for p in sorted(paths)}

def rows(text): return [json.loads(s) for s in text.splitlines() if s.startswith('{')]
def summary(values):
    return {'median': statistics.median(values), 'min': min(values), 'max': max(values), 'samples': values}

def physics_units(root):
    text = (root/'CMakeLists.txt').read_text()
    body = re.search(r'set\(JUDAS_PHYSICS_SOURCES\s+(.*?)\)', text, re.S)
    if not body: raise RuntimeError('Cannot identify actual production physics source list')
    result = re.findall(r'src/[A-Za-z0-9_]+\.cpp', body.group(1))
    if 'src/PhysicsWorld.cpp' not in result: result.append('src/PhysicsWorld.cpp')
    return result

class Run:
    def __init__(self, out): self.out=out; self.commands=[]
    def __call__(self, name, command, timeout=1800, required=True):
        start=time.monotonic();command=list(map(str,command))
        process=subprocess.run(command,cwd=ROOT,text=True,capture_output=True,timeout=timeout)
        (self.out/(name+'.log')).write_text(process.stdout+process.stderr)
        self.commands.append({'name':name,'command':command,'exit_code':process.returncode,
                              'seconds':time.monotonic()-start,'log':name+'.log'})
        save(self.out/'commands.json',self.commands)
        print(name,process.returncode,flush=True)
        if required and process.returncode: raise RuntimeError(name+' failed (see retained raw log)')
        return process.stdout
    def compile(self, name, source, destination, root=ROOT, objects=None):
        args=[COMPILER,*FLAGS,'-I'+str(root/'src'),'-I'+str(ROOT/'tests'),source]
        args += objects if objects is not None else [root/p for p in physics_units(root)]
        self(name,args+['-o',destination])
        return {'source':str(source),'source_sha256':sha(source),'binary':str(destination),'binary_sha256':sha(destination)}

def correctness(run, jobs):
    run('configure',['cmake','-S',ROOT,'-B',ROOT/'build','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_CXX_FLAGS=-ffp-contract=off'])
    run('build',['cmake','--build',ROOT/'build','--target',*TARGETS,'-j'+str(jobs)])
    timing=rows(run('impact-timing',[ROOT/'build/judas_impact_timing_tests']))
    energy=rows(run('original-energy-36',[ROOT/'build/judas_timing_defect_probe','energy']))
    extended=rows(run('extended-temporal',[ROOT/'build/judas_timing_defect_probe','extended']))
    extended_checks=[]
    for r in extended:
        expected=r['e']*(r['step']*r['dt']-r['initial_gap'])
        extended_checks.append(abs(r['gap']-expected)<2e-6 and abs(r['velocity'][1]-r['e'])<2e-6)
    if len(extended)!=240 or not all(extended_checks): raise RuntimeError('Unchanged 120-frame temporal fixture failed physical oracle')
    adapter=ROOT/'docs/evidence/ftft4b/baseline/adapters'
    binaries=[run.compile('compile-unchanged-temporal',adapter/'world_probe.cpp',BUILD/'temporal'),
              run.compile('compile-unchanged-energy',ROOT/'docs/evidence/ftft4b/baseline/research/code/production_energy_witness.cpp',BUILD/'energy')]
    text=run('unchanged-temporal',[BUILD/'temporal']);(run.out/'temporal.csv').write_text(text)
    oracle=json.loads(run('unchanged-temporal-oracle',[sys.executable,adapter/'check_world_trace.py',run.out/'temporal.csv','--mode','full']))
    run('unchanged-energy',[BUILD/'energy'])
    for name in TARGETS:
        if name not in ['judas_impact_timing_tests','judas_timing_defect_probe']:run(name,[ROOT/'build'/name])
    for flag in ['--player','--invalid']:run('geometry-'+flag[2:],[ROOT/'build/judas_contact_geometry_tests',flag])
    python=ROOT/'build/ftft4-reference-venv/bin/python'
    if not python.exists(): raise RuntimeError('Existing independent oracle Python environment unavailable')
    run('independent-geometry-oracle',[python,ROOT/'docs/evidence/ftft4/geometry_oracle.py','--binary',ROOT/'build/judas_contact_geometry_tests','--output',run.out/'geometry-oracle'])
    checks={'new_impact_tests':timing[-1]['pass'],'original_energy_36':len(energy)==36 and all(not r['energy_failure'] and not r['momentum_failure'] for r in energy),
            'extended_240':all(extended_checks),'unchanged_temporal_45':oracle['result']=='PASS' and oracle['checks']==45}
    return {'checks':checks,'overall_pass':all(checks.values()),'impact_summary':timing[-1],
            'maximum_energy_gain':max(r['delta_energy'] for r in energy),
            'maximum_momentum_residual':max(r['momentum_residual'] for r in energy),
            'physical_acceptance':True,'binaries':binaries,
            'scope':'Actual production engine. PLUS/Poisson is rejected; its historical failing evidence is unchanged.'}

def benchmark(run):
    P=module('ftft4_closure_previous',ROOT/'scripts/ftft4p_validation.py')
    A=module('ftft4_closure_allocation',ROOT/'scripts/ftft4alloc_validation.py')
    archive=run.out/'baseline-source.zip'
    run('baseline-archive',['git','archive','--format=zip','--output='+str(archive),BASELINE,'src','CMakeLists.txt','tests/BroadphaseTests.cpp'])
    baseline_root=BUILD/'baseline-source'
    baseline_root.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(archive) as z:
        if any(n.startswith('/') or '..' in Path(n).parts for n in z.namelist()):raise RuntimeError('Unsafe baseline archive')
        z.extractall(baseline_root)
    variants={}
    for label,root in [('baseline',baseline_root),('current',ROOT)]:
        directory=BUILD/label;directory.mkdir(parents=True,exist_ok=True);objects=[]
        for unit in physics_units(root):
            obj=directory/(Path(unit).stem+'.o')
            run(label+'-compile-'+Path(unit).stem,[COMPILER,*FLAGS,'-I'+str(root/'src'),'-c',root/unit,'-o',obj]);objects.append(obj)
        driver=run.out/(label+'-crate-driver.cpp');driver.write_text(A.generate(root/'tests/BroadphaseTests.cpp'))
        crate=run.compile(label+'-crate-link',driver,directory/'crate',root,objects)
        mixed=run.compile(label+'-mixed-link',ROOT/'tests/ContactPerformanceTests.cpp',directory/'mixed',root,objects)
        variants[label]={'root':str(root),'source_sha256':fingerprints(root),'crate':crate,'mixed':mixed}
    original=P.scale_body((baseline_root/'tests/BroadphaseTests.cpp').read_text())
    if original!=P.scale_body((ROOT/'tests/BroadphaseTests.cpp').read_text()):raise RuntimeError('Crate workload differs from baseline')
    records=[]
    for trial in range(7):
        for label in (['baseline','current'] if trial%2==0 else ['current','baseline']):
            for kind in ['crate','mixed']:
                output=run(f'{trial}-{label}-{kind}',[variants[label][kind]['binary']])
                records.append({'trial':trial,'label':label,'kind':kind,'rows':rows(output)})
    timings={};checks={}
    for label in variants:
        rr=[r['rows'][0] for r in records if r['label']==label and r['kind']=='crate']
        timings[label]={key+'_ms':summary([r[key]['ms'] for r in rr]) for key in ['whole','construction','teardown','oracle','exhaustive','tree_check']}
        timings[label]['all_steps_ms']=summary([r['all_steps_ms'] for r in rr])
        timings[label]['first_step_ms']=summary([r['steps'][0]['outer']['ms'] for r in rr])
        timings[label]['step_median_ms']=summary([statistics.median(s['outer']['ms'] for s in r['steps']) for r in rr])
        timings[label]['steady_step_median_ms']=summary([statistics.median(s['outer']['ms'] for s in r['steps'][1:]) for r in rr])
        timings[label]['engine_step_median_ms']=summary([statistics.median(s['engine_ms'] for s in r['steps']) for r in rr])
        checks[label+'_crate_contacts']=len(rr)==7 and all(s['pairs']==1500 and s['points']==6000 and s['unresolved']==0 for r in rr for s in r['steps'])
    current=timings['current']['step_median_ms']['median'];baseline=timings['baseline']['step_median_ms']['median']
    checks['crate_step_at_most_15_ms']=current<=15
    checks['crate_ftft4b_overhead_at_most_10_percent']=current<=1.1*baseline
    mixed={}
    for fixture in ['rotating_boxes','offset_compounds','near_parallel_and_exact_touch','moving_static_support']:
        mixed[fixture]={}
        for label in variants:
            rr=[x for r in records if r['label']==label and r['kind']=='mixed' for x in r['rows'] if x['kind']=='workload' and x['fixture']==fixture]
            mixed[fixture][label]={k:summary([r[k] for r in rr]) for k in ['setup_ms','external_frame_ms','total_ms','broadphase_ms','narrowphase_ms','solver_ms','cache_allocations','peak_cache_bytes']}
            checks[label+'_'+fixture]=len(rr)==7 and all(r['pass'] for r in rr)
    return {'overall_pass':all(checks.values()),'checks':checks,'versions':variants,'records':records,'timings':timings,'mixed':mixed,
            'crate_ratio':current/baseline,'trials':7,'crate_steps':420,'mixed_steps':6720,'flags':FLAGS,
            'timer_scope':'Outer world.Step includes all internal preparation. First step, construction and teardown reported. Whole fixture also includes exhaustive oracle; it is not production physics cost.',
            'mixed_note':'Timing is reported with current changed impact physics. Trajectory identity is not claimed after timing repair.'}

def regressions(run,jobs):
    P=module('ftft4_closure_regressions',ROOT/'scripts/ftft4p_validation.py')
    return P.regressions(run.out,jobs)

def main():
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['correctness','performance','regressions']);p.add_argument('--output',type=Path,required=True);p.add_argument('--jobs',type=int,default=4);a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);BUILD.mkdir(parents=True,exist_ok=True)
    start=time.monotonic();before=fingerprints();save(out/'source-sha256.json',before)
    P=module('ftft4_closure_protection',ROOT/'scripts/ftft4p_validation.py');protected=P.checked_protection();run=Run(out)
    result={}
    try:
        result=correctness(run,a.jobs) if a.mode=='correctness' else benchmark(run) if a.mode=='performance' else regressions(run,a.jobs)
    except Exception as exc:
        result={'overall_pass':False,'failure':str(exc)}
    result.update(mode=a.mode,seconds=time.monotonic()-start,head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                  source_unchanged=before==fingerprints(),protected_unchanged=protected==P.checked_protection(),human_visual_validation='NOT RUN')
    result['overall_pass'] &= result['source_unchanged'] and result['protected_unchanged'];save(out/'results.json',result)
    print(json.dumps({k:result[k] for k in ['mode','overall_pass','seconds']}));return 0 if result['overall_pass'] else 1
if __name__=='__main__':raise SystemExit(main())
