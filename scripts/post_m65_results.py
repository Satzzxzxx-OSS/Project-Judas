#!/usr/bin/env python3
"""Summarize bounded M56 observations; never turn known defects into passing fixes."""
from pathlib import Path
import collections,gzip,json,statistics
R=Path(__file__).resolve().parents[1];E=R/'docs/evidence/post_m65_consumers'
def read(p):return json.loads(gzip.open(p,'rt').read() if p.suffix=='.gz' else p.read_text())
def stats(v):
    if not v:return None
    v=sorted(v);return {'samples':len(v),'median':statistics.median(v),'p95':v[min(len(v)-1,int((len(v)-1)*.95))],'max':v[-1]}
def main():
 results={}
 for p in sorted(E.rglob('profile.json'))+sorted(E.rglob('profile.json.gz')):
    if 'scaling' in p.parts:continue
    d=read(p);frames=d['frames'][10:];label=str(p.parent.relative_to(E));groups=collections.defaultdict(list);counters=collections.defaultdict(list)
    for f in frames:
      groups['frame_interval_ms'].append(f['interval_ns']/1e6)
      for s in f['fixed']['steps']:groups['fixed_step_ms'].append((s['end_ns']-s['start_ns'])/1e6)
      sums=collections.defaultdict(float)
      for s in f['scopes']:
        if s['thread']==f['main_thread']:sums[s['name']]+=s['inclusive_ns']/1e6
      for name,v in sums.items():groups[name+'_ms_per_frame'].append(v)
      groups['script_phases_ms_per_frame'].append(sum(sums[n] for n in ['JavaScript UI update','JavaScript UI events','JavaScript update','JavaScript fixedUpdate','JavaScript presentation','JavaScript contact events']))
      for c in f['counters']:counters[c['name']].append(c['value'])
    results[label]={'policy':'Last 120 M56 frames, first 10 discarded; inclusive named scopes overlap. Observer audio mix/sleep and occasional readback/export affect outer intervals. Fixed-step and Play frame scopes exclude observer callbacks. Controlled-clock runs are not real-time FPS evidence.','frames':len(frames),'diagnostics':d['diagnostics'],'fixed_cap_frames':sum(f['fixed']['cap_reached'] for f in frames),'ms':{k:stats(v) for k,v in groups.items()},'counters':{k:stats(v) for k,v in counters.items()}}
 (E/'profile-summary.json').write_text(json.dumps(results,indent=2)+'\n')
 for name in ['realtime-skate','realtime-rooftop','realtime-void','skate-ragdolls']:
    if name not in results:continue
    v=results[name];print(name, {k:v['ms'].get(k) for k in ['frame_interval_ms','Play frame_ms_per_frame','fixed_step_ms','Rigid physics_ms_per_frame','script_phases_ms_per_frame']})
if __name__=='__main__':main()
