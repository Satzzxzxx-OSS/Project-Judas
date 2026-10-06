#!/usr/bin/env python3
"""Modest M56 warmed M65 workloads; same content/cadence, sleep-only reference."""
import gzip,hashlib,json,math,os,statistics,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/evidence/m65/performance-final'
def summarize(data):
 frames=data['frames'][-120:]
 def stats(values):
  values=sorted(values);return {'median_ms':statistics.median(values),'p95_ms':values[max(0,math.ceil(.95*len(values))-1)],'max_ms':max(values)}
 result={'frames':len(frames),'frame':stats([(f['end_ns']-f['start_ns'])/1e6 for f in frames]),'fixed':stats([sum((x['end_ns']-x['start_ns'])/1e6 for x in f['fixed']['steps']) for f in frames]),'scopes':{},'last_counters':{c['name']:c['value'] for c in frames[-1]['counters']}}
 for name in ['Rigid physics','Physics broadphase','Physics contacts','Physics contact and joint solver','Character motors','Final pose resolution','Animation instances','JavaScript fixedUpdate']:
  values=[sum(s['inclusive_ns']/1e6 for s in f['scopes'] if s['name']==name and s['thread']==f['main_thread']) for f in frames]
  if any(values):result['scopes'][name]=stats(values)
 for name in ['M65 IK end error metres','Sleeping physics bodies','Awake physics bodies']:
  values=[c['value'] for f in frames for c in f['counters'] if c['name']==name]
  if values:result[name]={'min':min(values),'max':max(values)}
 return result

def main():
 assert not OUT.exists(),'Preserve earlier timings.'
 OUT.mkdir(parents=True);(OUT/'machine-load.txt').write_text(subprocess.check_output(['uptime'],text=True)+Path('/proc/loadavg').read_text())
 env=dict(os.environ,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_DATA_HOME=str(ROOT/'.cache/m65-perf-final'));env.pop('LIBGL_ALWAYS_SOFTWARE',None)
 runs=[];work={}
 for count in [1,10,20]:
  for suffix in ['-reference','']:
   name=f'bench-{count}{suffix}';directory=OUT/name;start=time.perf_counter()
   with (OUT/(name+'.log')).open('w') as f:r=subprocess.run([str(ROOT/'build/judas_developer_application_tests'),str(ROOT/'projects/m65_integration/m65_integration.judasproj'),name,str(directory)],cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=120)
   runs.append({'name':name,'exit_code':r.returncode,'seconds':time.perf_counter()-start});assert r.returncode==0
   work[name]=summarize(json.loads((directory/'profile.json').read_text()));print(name,work[name]['fixed'],flush=True)
 name='pose-20';directory=OUT/name
 with (OUT/(name+'.log')).open('w') as f:r=subprocess.run([str(ROOT/'build/judas_developer_application_tests'),str(ROOT/'projects/m65_integration/m65_integration.judasproj'),name,str(directory)],cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=120)
 runs.append({'name':name,'exit_code':r.returncode});assert r.returncode==0;work[name]=summarize(json.loads((directory/'profile.json').read_text()))
 for count in [1,10,100]:work[f'motor-{count}']=summarize(json.loads((ROOT/f'docs/evidence/m65/late-followup/core-run/motor-{count}.json').read_text()))
 manifest=[]
 for p in OUT.rglob('profile.json'):
  raw=p.read_bytes();z=p.with_suffix('.json.gz');z.write_bytes(gzip.compress(raw,mtime=0));manifest.append({'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'gzip':str(z.relative_to(ROOT))});p.unlink()
 result={'method':'M56 final 120 warmed frames, 60 Hz. Reference changes only sleep enable. Concurrent user applications remain running. Elapsed CPU times, not achieved FPS; corrected source after the broad gate.','runs':runs,'workloads':work,'profiles':manifest}
 (OUT/'RESULTS.json').write_text(json.dumps(result,indent=2)+'\n');(ROOT/'docs/evidence/m65/PERFORMANCE_FINAL.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
