#!/usr/bin/env python3
"""One bounded FTFT4A-P storage pass. Sequential, source-matched measurements.
--build, --measure, --regressions; no mode overwrites existing evidence.
Timings exclude global-new instrumentation; separate diagnostic runs include it.
"""
import sys
sys.dont_write_bytecode=True
import argparse,importlib.util,json,re,statistics,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def module(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
P=module('allocation_previous',ROOT/'scripts/ftft4p_validation.py')
OUT=ROOT/'docs/evidence/ftft4/allocation';BUILD=ROOT/'build/ftft4alloc'
LABELS=['before','after','slow']
def marked(code):return '\n/*ALLOCATION_BEGIN*/\n'+code+'\n/*ALLOCATION_END*/\n'
def generate(source):
 original=source.read_text();text=original
 text=text.replace('Comparison Compare(const PhysicsWorld& world) {','Comparison Compare(const PhysicsWorld& world) {'+marked('allocation_evidence::Scope all(allocation_evidence::oracle);\nconst allocation_evidence::Point oracleStart{};'),1)
 text=text.replace('    const PairSet reported = BroadphasePairs(world);',marked('allocation_evidence::exhaustive=allocation_evidence::between(oracleStart,allocation_evidence::Point{});\nconst allocation_evidence::Point treeStart{};')+'    const PairSet reported = BroadphasePairs(world);'+marked('allocation_evidence::tree=allocation_evidence::between(treeStart,allocation_evidence::Point{});'),1)
 start=text.index('void TestScaleStatistics() {');end=text.index('\n}\n',start)+3
 body=text[start:end]
 body=body.replace('void TestScaleStatistics() {','void TestScaleStatistics() {'+marked('allocation_evidence::Meter<PhysicsWorld::StepStats> meter;'),1)
 body=body.replace('    for (int step = 0; step < 30; ++step) {',marked('meter.built();')+'    for (int step = 0; step < 30; ++step) {'+marked('meter.start();'),1)
 body=body.replace('        world.Step(1.0f / 60.0f);','        world.Step(1.0f / 60.0f);'+marked('meter.finish(step,world.LastStepStats());'),1)
 body=body[:-3]+marked('meter.teardown();')+body[-3:]
 text=text[:start]+body+text[end:]
 assert re.sub(r'\n/\*ALLOCATION_BEGIN\*/.*?/\*ALLOCATION_END\*/\n','',text,flags=re.S)==original
 return '#include "ContactAllocationInstrumentation.h"\n#define main original_broadphase_main\n'+text+'\n#undef main\nint main(){TestScaleStatistics();return g_failures?1:0;}\n'
def build(directory):
 P.BUILD=BUILD
 P.extract(OUT/'before-source.zip',BUILD/'before-source')
 P.extract(ROOT/'docs/evidence/ftft4/performance/pre-optimization-source.zip',BUILD/'slow-source')
 versions={}
 for name in LABELS:
  root=ROOT if name=='after' else BUILD/(name+'-source')
  v=P.build_variant(name,root,directory)
  driver=directory/(name+'-partition.cpp');driver.write_text(generate(root/'tests/BroadphaseTests.cpp'))
  objects=[BUILD/name/(u+'.o') for u in P.UNITS]
  for instrumented in [False,True]:
   suffix='allocations' if instrumented else 'partition';binary=BUILD/name/suffix
   extra=['-DFTFT4_ALLOC_TRACKING',ROOT/'tests/ContactAllocationTracking.cpp'] if instrumented else []
   record,_=P.required(name+'-'+suffix,[P.COMPILER,*P.FLAGS,'-I'+str(root/'src'),'-I'+str(ROOT/'tests'),*extra,driver,*objects,'-o',binary],directory)
   v['commands'].append(record);v['binaries'][suffix]={'path':str(binary),'sha256':P.sha(binary)}
  versions[name]=v
 assert len({P.scale_body((Path(v['source_root'])/'tests/BroadphaseTests.cpp').read_text()) for v in versions.values()})==1
 return {'versions':versions,'original_fixture_unchanged':True,'overall_pass':True}
def lifetime_build(directory):
 versions=P.load(OUT/'build/results.json')['versions']
 original=(ROOT/'tests/ContactPerformanceTests.cpp').read_text();text=original
 text=text.replace('bool Workload(const char* name,int kind,bool trace) {','bool Workload(const char* name,int kind,bool trace) {'+marked('allocation_evidence::MixedMeter<PhysicsWorld::StepStats> meter(name);'),1)
 text=text.replace('    const double setupMs=Milliseconds(setupStart);','    const double setupMs=Milliseconds(setupStart);'+marked('meter.built();'),1)
 text=text.replace('        const auto begin=Clock::now();','        const auto begin=Clock::now();'+marked('meter.start();'),1)
 text=text.replace('        world.Step(dt);','        world.Step(dt);'+marked('meter.finish(step,world.LastStepStats());'),1)
 text=text.replace('    return pass;','    '+marked('meter.teardown();')+'return pass;',1)
 # Removing insertions must recover every original physics/fixture statement.
 assert re.sub(r'\n/\*ALLOCATION_BEGIN\*/.*?/\*ALLOCATION_END\*/\n','',text,flags=re.S)==original
 assert text.count('meter.start()')==1 and text.count('meter.finish(')==1
 driver=directory/'mixed-partition.cpp';driver.write_text('#include "ContactMixedLifetime.h"\n'+text)
 for label,v in versions.items():
  root=Path(v['source_root']);assert v['source_sha256']==P.source_hashes(root)
  objects=[BUILD/label/(u+'.o') for u in P.UNITS]
  for tracked in [False,True]:
   name='mixed_allocations' if tracked else 'mixed';binary=BUILD/label/('lifetime-'+name)
   extra=['-DFTFT4_ALLOC_TRACKING',ROOT/'tests/ContactAllocationTracking.cpp'] if tracked else []
   record,_=P.required(label+'-'+name,[P.COMPILER,*P.FLAGS,'-I'+str(root/'src'),'-I'+str(ROOT/'tests'),*extra,driver,*objects,'-o',binary],directory)
   v['binaries'][name]={'path':str(binary),'sha256':P.sha(binary)};v['commands'].append(record)
 return {'versions':versions,'unchanged_original_fixture':True,'overall_pass':True}

def rows(text):return [json.loads(x) for x in text.splitlines() if x.startswith('{')]
def summarize(a):return {'median':statistics.median(a),'min':min(a),'max':max(a),'samples':a}
def measure(directory):
 versions=P.load(OUT/'lifetime-build/results.json')['versions'];records=[]
 for label,v in versions.items():
  assert v['source_sha256']==P.source_hashes(Path(v['source_root']))
  for b in v['binaries'].values():assert P.sha(b['path'])==b['sha256']
 for trial in range(7):
  for label in LABELS[trial%3:]+LABELS[:trial%3]:
   for kind in ['partition','mixed']:
    args=[versions[label]['binaries'][kind]['path']]
    if kind=='mixed' and trial==0:args.append('--trace')
    command,text=P.required(f'{trial}-{label}-{kind}',args,directory)
    records.append({'trial':trial,'label':label,'kind':kind,'command':command,'rows':rows(text)})
    print(trial,label,kind,flush=True)
 diagnostics=[]
 for label,v in versions.items():
  command,text=P.required(label+'-allocation-diagnostic',[v['binaries']['allocations']['path']],directory)
  diagnostics.append({'label':label,'kind':'crate','command':command,'rows':rows(text)})
  command,text=P.required(label+'-mixed-allocation-diagnostic',[v['binaries']['mixed_allocations']['path']],directory)
  diagnostics.append({'label':label,'kind':'mixed','command':command,'rows':rows(text)})
 timing={};checks={}
 for label in LABELS:
  rr=[r['rows'][0] for r in records if r['label']==label and r['kind']=='partition'];summary={}
  for k in ['whole','construction','teardown','oracle','exhaustive','tree_check']:summary[k+'_ms']=summarize([r[k]['ms'] for r in rr])
  for k in ['all_steps_ms','other_instrumentation_ms']:summary[k]=summarize([r[k] for r in rr])
  summary['first_step_ms']=summarize([r['steps'][0]['outer']['ms'] for r in rr])
  summary['steady_step_median_ms']=summarize([statistics.median(s['outer']['ms'] for s in r['steps'][1:]) for r in rr])
  summary['steady_step_range_ms']=[min(s['outer']['ms'] for r in rr for s in r['steps'][1:]),max(s['outer']['ms'] for r in rr for s in r['steps'][1:])]
  summary['final_engine_step_ms']=summarize([r['steps'][-1]['engine_ms'] for r in rr]);timing[label]=summary
  checks[label+'_crate_contacts']=all(s['pairs']==1500 and s['points']==6000 and s['unresolved']==0 for r in rr for s in r['steps'])
 mixed={}
 for fixture in ['rotating_boxes','offset_compounds','near_parallel_and_exact_touch','moving_static_support']:
  mixed[fixture]={};sums={}
  for label in LABELS:
   rr=[x for r in records if r['label']==label and r['kind']=='mixed' for x in r['rows'] if x['kind']=='workload' and x['fixture']==fixture]
   mixed[fixture][label]={k:summarize([x[k] for x in rr]) for k in ['setup_ms','external_frame_ms','total_ms','narrowphase_ms','solver_ms','cache_allocations','peak_cache_bytes'] if k in rr[0]}
   sums[label]=set(x['state_checksum'] for x in rr);checks[label+'_'+fixture]=len(rr)==7 and all(x['pass'] for x in rr)
  checks[fixture+'_identical_trajectories']=len(set.union(*sums.values()))==1
 checks['after_steady_cache_zero_allocations']=all(s['cache_calls']==0 and s['cache_allocated_bytes']==0 for r in records if r['label']=='after' and r['kind']=='partition' for s in r['rows'][0]['steps'][1:])
 return {'records':records,'diagnostics':diagnostics,'timings':timing,'mixed':mixed,'checks':checks,'overall_pass':all(checks.values()),'trials':7,'timed_crate_steps':630,'timed_mixed_steps':10080,'diagnostic_crate_steps':90,'timer_note':'setup, first/all 30 steps, implicit destructors included; oracle includes actual exhaustive narrowphase; other includes diagnostics/assertions/counter copies. Allocation diagnostics are separate, not timing acceptance.'}
def main():
 a=argparse.ArgumentParser();a.add_argument('mode',choices=['build','lifetime-build','measure','regressions']);a.add_argument('--jobs',type=int,default=4);args=a.parse_args();d=OUT/args.mode
 if d.exists():raise RuntimeError('Refusing to overwrite '+str(d))
 d.mkdir(parents=True);start=time.monotonic();protected=P.checked_protection();before=P.BASE.fingerprints()
 result=build(d) if args.mode=='build' else lifetime_build(d) if args.mode=='lifetime-build' else measure(d) if args.mode=='measure' else P.regressions(d,args.jobs)
 result.update(seconds=time.monotonic()-start,source_sha256=before,source_unchanged=before==P.BASE.fingerprints(),protection_unchanged=protected==P.checked_protection(),head=P.BASE.git('rev-parse','HEAD'))
 result['overall_pass'] &= result['source_unchanged'] and result['protection_unchanged'];P.save(d/'results.json',result);print(json.dumps({k:result[k] for k in ['overall_pass','seconds']}));return 0 if result['overall_pass'] else 1
if __name__=='__main__':raise SystemExit(main())
