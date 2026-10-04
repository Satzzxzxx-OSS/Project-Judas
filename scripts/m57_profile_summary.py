"""Summarize retained M56 profiles; exclude known screenshot-adjacent intervals.
Usage: python3 scripts/m57_profile_summary.py PROFILE_DIRECTORY
"""
from pathlib import Path
import json,statistics as st,sys
root=Path(sys.argv[1]);summary={}
def stats(values):
 values=sorted(values)
 return {'median':round(st.median(values),4),'p95':round(values[int(.95*(len(values)-1))],4),'max':round(max(values),4)} if values else None
for mode in range(3):
 data=json.loads((root/f'range-{mode}-profile.json').read_text());frames=data['frames'];warm=frames[15:-10];allwarm=warm;warm=[f for f in warm if not 89<=f['id']-data['startup']['id']<=92]
 gpu={}
 for f in warm:
  for g in f['gpu']['passes']:
   if not g['pending']:gpu.setdefault(g['pass'],[]).append(g['milliseconds'])
 def scope(name):return stats([sum(s['inclusive_ns'] for s in f['scopes'] if s['name']==name)/1e6 for f in warm])
 summary[str(mode)]={'frames':len(warm),'screenshot_interval_max_ms':max(f['interval_ns'] for f in allwarm)/1e6,'frame_ms':stats([f['interval_ns']/1e6 for f in warm]),'CPU_active_ms':stats([(f['covered_ns']-f['waiting_ns'])/1e6 for f in warm]),'wait_ms':stats([f['waiting_ns']/1e6 for f in warm]),'unattributed_ms':stats([f['unattributed_ns']/1e6 for f in warm]),'gpu_pass_ms':{k:stats(v) for k,v in gpu.items()},'fixed_ms':stats([sum(step['end_ns']-step['start_ns'] for step in f['fixed']['steps'])/1e6 for f in warm]),'script_callback_ms':scope('Script lifecycle callback'),'draws':sorted(set(c['value'] for f in warm for c in f['counters'] if c['name']=='Submitted draws all passes')),'diagnostics':data['diagnostics'],'startup_ms':(data['startup']['end_ns']-data['startup']['start_ns'])/1e6,'memory_bytes':frames[-1]['memory']}
print(json.dumps(summary,indent=2))
