#!/usr/bin/env python3
"""Research alternate contact ACTIVATION schedule, not a current-engine pass.
Resolve closing/resting coincident contacts, then process contacts made closing
by the impulse at the SAME clock time. Existing normal restitution law retained.
No friction. Fraction-exact small LCPs. No energy clipping.
"""
from event_reference import *
from event_checks import enc
import random,json,time
from pathlib import Path

def main():
 start=time.perf_counter();rng=random.Random(128422);runs=[];nchecks=0;total_events=0;incomplete=[]
 for trial in range(360):
  n=rng.randrange(2,7);r=[F(rng.randrange(1,4),10) for _ in range(n)];x=[F(0)]
  for i in range(1,n):x.append(x[-1]+r[i-1]+r[i]+F(rng.randrange(0,4),10))
  v=[F(rng.randrange(-10,11)) for _ in range(n)];m=[rng.choice([F(1,10),F(1),F(10)]) for _ in range(n)]
  material=[rng.choice([F(0),F(1,5),F(1,2),F(1)]) for _ in range(n)]
  es={f'{i}:{i+1}':max(material[i],material[i+1]) for i in range(n-1)}
  s=State(x,v,m,r);h=F(1,5)
  opts=dict(model='closing_active',capture=F(1,2),pair_restitution=es)
  t,z=step(s,h,**opts)
  assert dot(m,t.v)==dot(m,v);nchecks+=1
  assert all(q['energy_change']<=0 for q in z['events']);nchecks+=1
  if z['status']!='COMPLETE':incomplete.append(dict(trial=trial,status=z['status'],remaining=z['remaining']));continue
  total_events+=len(z['events'])
  rev,rz=step(s,h,row_reverse=True,**opts);assert rev.x==t.x and rev.v==t.v;nchecks+=1
  t1,z1=step(s,h*F(2,5),**opts);t2,z2=step(t1,h*F(3,5),**opts)
  assert z1['status']==z2['status']=='COMPLETE' and t2.x==t.x and t2.v==t.v;nchecks+=1
  boost=F(23,7);shift=F(12345,8);sb=State([a+shift for a in x],[a+boost for a in v],m,r)
  tb,zb=step(sb,h,**opts);assert tb.x==[a+shift+boost*h for a in t.x] and tb.v==[a+boost for a in t.v];nchecks+=1
  runs.append(dict(trial=trial,n=n,events=len(z['events'])))
 # Dedicated counterexample with all binary-exact input dimensions/mass.
 s=State([F(0),F(1),F(2)],[F(1),F(-3),F(4)],[F(10),F(1,8),F(10)],[F(1,2)]*3)
 t,z=step(s,F(1,60),F(1),model='closing_active',capture=F(1,2));assert z['status']=='COMPLETE';nchecks+=1
 witness=dict(initial=s.__dict__,final=t.__dict__,energy_before=dot(s.m,[x*x/F(2) for x in s.v]),energy_after=dot(s.m,[x*x/F(2) for x in t.v]),log=z)
 out=dict(status='PASS_TESTED_NORMAL_EVENT_REFERENCE' if not incomplete else 'INCOMPLETE',cases=360,completed=len(runs),incomplete=incomplete,events=total_events,assertions=nchecks,witness=witness,
      scope='1D normal impacts only; activation sequencing is a deliberate research change, not an already-approved production change; no 3D/friction/performance theorem',runtime_seconds=time.perf_counter()-start)
 (Path(__file__).resolve().parents[1]/'results/induced_impact_results.json').write_text(json.dumps(enc(out),indent=2));print(json.dumps(enc({k:v for k,v in out.items() if k!='witness'}),indent=2))
if __name__=='__main__':main()
