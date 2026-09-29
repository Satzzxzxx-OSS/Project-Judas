"""Extend, do not overwrite, the previous exact event experiment.
A larger research work budget is NOT a production termination strategy.
"""
from pathlib import Path
import sys,json,random,time
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reference'))
from event_reference_original import *

def enc(v):
 if isinstance(v,F):return {'exact':str(v),'float':float(v)}
 if isinstance(v,dict):return {k:enc(x) for k,x in v.items()}
 if isinstance(v,(list,tuple)):return [enc(x) for x in v]
 return v

def fixtures():
 rng=random.Random(128422)
 for trial in range(360):
  n=rng.randrange(2,7);r=[F(rng.randrange(1,4),10) for _ in range(n)];x=[F(0)]
  for i in range(1,n):x.append(x[-1]+r[i-1]+r[i]+F(rng.randrange(0,4),10))
  v=[F(rng.randrange(-10,11)) for _ in range(n)];m=[rng.choice([F(1,10),F(1),F(10)]) for _ in range(n)]
  mat=[rng.choice([F(0),F(1,5),F(1,2),F(1)]) for _ in range(n)]
  yield trial,State(x,v,m,r),{f'{i}:{i+1}':max(mat[i],mat[i+1]) for i in range(n-1)}

def main():
 ts=time.perf_counter();rows=[];fails=[];checks=0;old=[]
 for trial,s,es in fixtures():
  opts=dict(model='closing_active',capture=F(1,2),pair_restitution=es,max_events=1024)
  t,z=step(s,F(1,5),**opts)
  assert dot(s.m,s.v)==dot(s.m,t.v);checks+=1
  assert all(q['energy_change']<=0 for q in z['events']);checks+=1
  if z['status']!='COMPLETE':fails.append(dict(trial=trial,status=z['status'],remaining=z['remaining']));continue
  rev,rz=step(s,F(1,5),row_reverse=True,**opts);assert rz['status']=='COMPLETE' and rev.x==t.x and rev.v==t.v;checks+=1
  t1,z1=step(s,F(2,25),**opts);t2,z2=step(t1,F(3,25),**opts);assert z1['status']==z2['status']=='COMPLETE' and t2.x==t.x and t2.v==t.v;checks+=1
  boost=F(23,7);shift=F(12345,8);sb=State([x+shift for x in s.x],[v+boost for v in s.v],s.m,s.r)
  tb,zb=step(sb,F(1,5),**opts);assert zb['status']=='COMPLETE' and tb.x==[x+shift+boost*F(1,5) for x in t.x] and tb.v==[v+boost for v in t.v];checks+=1
  row=dict(trial=trial,events=len(z['events']),K_before=dot(s.m,[v*v/F(2) for v in s.v]),K_after=dot(s.m,[v*v/F(2) for v in t.v]))
  rows.append(row)
  if trial in (116,180,250):
   r128,z128=step(s,F(1,5),**dict(opts,max_events=128));assert z128['status']=='UNRESOLVED_EVENT_BUDGET'
   old.append(dict(trial=trial,initial=s.__dict__,restitution=es,events=len(z['events']),consumed=z['consumed'],remaining=z['remaining'],final_velocity=t.v,
      prior128_status=z128['status'],prior128_consumed=z128['consumed'],min_positive_prior128_gap=min([x[1] for x in constraints(r128) if x[1]>0],default=F(0))))
 # Exact repaired normal-only actual-witness reference: no physical current-engine result.
 s=State([F(0),F(1),F(2)],[F(1),F(-3),F(4)],[F(10),F(1,8),F(10)],[F(1,2)]*3)
 t,z=step(s,F(1,60),e=F(1),model='closing_active',capture=F(1,2),max_events=1024)
 assert z['status']=='COMPLETE';assert dot(s.m,[v*v for v in s.v])==dot(s.m,[v*v for v in t.v]);checks+=2
 witness=dict(initial=s.__dict__,final=t.__dict__,events=z['events'],K_before=dot(s.m,[v*v/F(2) for v in s.v]),K_after=dot(s.m,[v*v/F(2) for v in t.v]))
 # A diagnostic without the source's 0.5 m/s capture; never count budget exit as PASS.
 trial,s,es=next(x for x in fixtures() if x[0]==116)
 unc,zu=step(s,F(1,5),model='closing_active',capture=F(0),pair_restitution=es,max_events=512)
 no_capture=dict(trial=trial,status=zu['status'],events=len(zu['events']),consumed=float(zu['consumed']),remaining=float(zu['remaining']),
   last_closing_velocity=max(0.,-min(float(dot(j,unc.v)) for _,_,j,_ in constraints(unc))),
   interpretation='Finite work cap reached, not a proof of an infinite sequence. Neither cap exit nor increased budget is accepted as a production progress policy.')
 # Independent analytically summable gravitational bouncing-ball collapse.
 v0=2.;g=9.81;e=.5;tlimit=2*e*v0/(g*(1-e))
 bouncer={'initial_downward_speed':v0,'gravity':g,'e':e,'time_after_first_impact_to_infinite_event_accumulation':tlimit,
  'partial_40_round_trip_times':sum(2*v0*e**k/g for k in range(1,41)),
  'note':'Ideal constant restitution, exact gravity. Requires a sustained-contact continuation after the limit; differs from the engine low-speed capture law.'}
 out=dict(status='ALL_PREVIOUS_360_CASES_COMPLETE_WITH_LARGER_RESEARCH_BUDGET',cases=len(rows),checks=checks,max_events=max(r['events'] for r in rows),total_events=sum(r['events'] for r in rows),
  unresolved=fails,old_failures_resolved=old,normal_witness=witness,without_capture=no_capture,analytic_zeno=bouncer,
  capture_speed=F(1,2),event_budget=1024,scope='Exact 1D frictionless reference. Per-contact e and original 0.5 capture unchanged. No production run or general finite-cost proof.',runtime_s=time.perf_counter()-ts,rows=rows)
 root=Path(__file__).resolve().parents[1];(root/'results/event_progress.json').write_text(json.dumps(enc(out),indent=2));print(json.dumps(enc({k:v for k,v in out.items() if k not in ('rows','normal_witness','old_failures_resolved')}),indent=2))
 print('old event counts',[(x['trial'],x['events']) for x in old]);print('witness',list(map(float,t.v)) if False else enc(witness))
if __name__=='__main__':main()
