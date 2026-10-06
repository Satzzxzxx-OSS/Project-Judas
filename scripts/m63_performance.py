#!/usr/bin/env python3
"""Identical-setting bounded CPU/application comparison. Never starts load workers."""
import os,json,time,subprocess,sys,importlib.util
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
def main():
 out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=False);native='--native' in sys.argv
 spec=importlib.util.spec_from_file_location('measures',ROOT/'scripts/m63_focused.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 scratch=ROOT/'.cache'/('m63-'+out.name);env={k:v for k,v in os.environ.items() if not k.startswith('JUDAS_')};env.update(SDL_VIDEODRIVER='x11' if native else 'offscreen',SDL_AUDIODRIVER='dummy',JUDAS_WORLD_STATE='none',XDG_DATA_HOME=str(scratch/'data'))
 if not native:env['LIBGL_ALWAYS_SOFTWARE']='1'
 result={'graphics':'native desktop X11' if native else 'offscreen software requested','load_average_before':Path('/proc/loadavg').read_text().strip(),'cpu':subprocess.check_output(['lscpu'],text=True),'cases':{}}
 for mode in ('perf-baseline','perf-intact','perf-failing','perf-settled'):
  start=time.monotonic();p=subprocess.run([str(ROOT/'build/judas_fracture_application_tests'),str(ROOT/'projects/fracture_lab/fracture_lab.judasproj'),mode,str(scratch/mode)],env=env,cwd=ROOT,capture_output=True,text=True,timeout=180)
  (out/(mode+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,mode;path=scratch/mode/'profile.json';d=json.loads(path.read_text());r=m.measurements(path)
  def stats(v):
   v=sorted(v);return {'samples':len(v),'median_ms':v[len(v)//2] if v else 0,'p95_ms':v[min(len(v)-1,int(len(v)*.95))] if v else 0,'maximum_ms':max(v,default=0)}
  r['windows']={}
  for label,lo,hi in [('intact',60,99),('failure',100,160),('settled',250,359)]:
   frames=[f for f in d['frames'] if lo<=f['id']<=hi];scopes={}
   for f in frames:
    totals={}
    for s in f['scopes']:totals[s['name']]=totals.get(s['name'],0)+s['inclusive_ns']/1e6
    for n,v in totals.items():
     if 'Fracture' in n or n in ('World deformables','World render submission','Physics contact and joint solver','Physics contacts','Physics broadphase','Deformable render mapping'):scopes.setdefault(n,[]).append(v)
   r['windows'][label]={'frame':stats([(f['end_ns']-f['start_ns'])/1e6 for f in frames]),'fixed':stats([(step['end_ns']-step['start_ns'])/1e6 for f in frames for step in f['fixed']['steps']]),'scopes':{n:stats(v) for n,v in scopes.items()}}
  r['largest_retained_fracture_scope_ms']=max([s['inclusive_ns']/1e6 for f in d['frames'] for s in f['scopes'] if 'Fracture' in s['name']],default=0)
  r['first_failure_frame_ms']=max([(f['end_ns']-f['start_ns'])/1e6 for f in d['frames'] if 100<=f['id']<=102],default=0)
  r['command_seconds']=time.monotonic()-start;result['cases'][mode]=r
  (out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n')
 result['load_average_after']=Path('/proc/loadavg').read_text().strip();result['pass']=True;(out/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({n:r['windows'] for n,r in result['cases'].items()},indent=2))
if __name__=='__main__':main()
