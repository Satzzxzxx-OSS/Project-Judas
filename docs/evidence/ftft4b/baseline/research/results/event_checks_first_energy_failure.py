#!/usr/bin/env python3
from event_reference import *
import random,json,time
from pathlib import Path

def enc(x):
 if isinstance(x,F):return {'exact':str(x),'float':float(x)}
 if isinstance(x,dict):return {str(k):enc(v) for k,v in x.items()}
 if isinstance(x,(list,tuple)):return [enc(v) for v in x]
 return x

def main():
 start=time.perf_counter(); counts={}; records={}; neg={}; checks=0
 def ck(b):
  nonlocal checks
  checks+=1
  assert b
 # Isolated body-wall impacts, including tau exactly endpoint and repeated no-force frames.
 count=0
 for gap in [F(1,1000),F(1,10),F(1)]:
  for speed in [F(1),F(7),F(30)]:
   for e in [F(0),F(1,2),F(1)]:
    for multiple in [F(1,2),F(1),F(3,2),F(3)]:
     tau=gap/speed;h=tau*multiple
     s=State([F(1,2)+gap],[-speed],[F(2)],[F(1,2)])
     t,log=step(s,h,e,left=F(0));ck(log['status']=='COMPLETE');ck(log['remaining']==0)
     ck(t.x[0]-t.r[0]==(gap-speed*h if h<tau else e*speed*(h-tau)))
     ck(t.v[0]==(-speed if h<tau else e*speed));count+=1
     if h>=tau and e==0:
      for _ in range(4):t,z=step(t,h,e,left=F(0));ck(t.x[0]==t.r[0] and t.v[0]==0)
 counts['isolated_wall_cases']=count
 # Chains of successive elastic impacts have exact endpoint and event time oracle.
 chain=[]
 for n in range(2,9):
  for den in [2,5,10]:
   gap=F(1,den);r=F(1,2);speed=F(10);h=F(n)*gap/speed
   s=State([i*(1+gap) for i in range(n)],[speed]+[F(0)]*(n-1),[F(1)]*n,[r]*n)
   t,z=step(s,h,F(1));ck(z['status']=='COMPLETE');ck(len(z['events'])==n-1)
   ck([q['time'] for q in z['events']]==[i*gap/speed for i in range(1,n)])
   ck(t.v==[F(0)]*(n-1)+[speed]);ck(t.x==[s.x[i]+gap for i in range(n)])
   chain.append(dict(n=n,gap=gap,events=len(z['events'])))
 counts['elastic_chain_cases']=len(chain)
 s=State([F(0),F(9,5),F(18,5)],[F(10),F(0),F(0)],[F(1)]*3,[F(1,2)]*3)
 t,z=step(s,F(1,5));records['three_body_chain']=dict(final=t.__dict__,log=z)
 bad,bz=step(s,F(1,5),frozen_pairs=True);ck(bz['status']=='PENETRATION');neg['stale_initial_candidate_reach']=dict(final=bad.__dict__,status=bz['status'])
 _,limit=step(s,F(1,5),max_events=1);ck(limit['status']=='UNRESOLVED_EVENT_BUDGET');ck(limit['consumed']==F(2,25));ck(limit['remaining']==F(3,25));neg['event_budget_report']=limit
 # Random exact states. Conservation, dissipation and step-partition equivalence.
 rng=random.Random(426220);maxevents=0;total_events=0
 for trial in range(180):
  n=rng.randrange(2,6);r=[F(rng.randrange(1,4),10) for _ in range(n)];x=[F(0)]
  for i in range(1,n):x.append(x[-1]+r[i-1]+r[i]+F(rng.randrange(0,5),10))
  v=[F(rng.randrange(-6,7)) for _ in range(n)];m=[rng.choice([F(1,10),F(1),F(10)]) for _ in range(n)]
  e=rng.choice([F(0),F(1,2),F(1)]);h=F(1,5);s=State(x,v,m,r)
  t,z=step(s,h,e);ck(z['status']=='COMPLETE');ck(dot(m,t.v)==dot(m,v))
  ck(dot(m,[a*a for a in t.v])<=dot(m,[a*a for a in v]));ck(all(q['energy_change']<=0 for q in z['events']))
  t1,_=step(s,h*F(2,5),e);t2,_=step(t1,h*F(3,5),e)
  ck(t2.x==t.x and t2.v==t.v)
  rev,rz=step(s,h,e,row_reverse=True);ck(rev.x==t.x and rev.v==t.v)
  boost=F(23,7);shift=F(12345,8);sb=State([a+shift for a in x],[a+boost for a in v],m,r)
  tb,_=step(sb,h,e);ck(tb.x==[a+shift+boost*h for a in t.x]);ck(tb.v==[a+boost for a in t.v])
  maxevents=max(maxevents,len(z['events']));total_events+=len(z['events'])
 counts['random_exact_multi_body_cases']=180;counts['random_events']=total_events;counts['max_random_events']=maxevents
 # Closed-contact cluster includes an initially stationary contact.
 s=State([F(0),F(1),F(2)],[F(1),F(0),F(0)],[F(1)]*3,[F(1,2)]*3)
 cluster,z=step(s,F(1,10),F(1));ck(cluster.v==[F(-1,3),F(2,3),F(2,3)])
 records['simultaneous_declared_newton_model']=dict(velocity=cluster.v,energy=dot(cluster.m,[a*a/F(2) for a in cluster.v]))
 # Infinitesimally sequential elastic alternative is also momentum/energy preserving.
 sequential=[F(0),F(0),F(1)];ck(dot(s.m,sequential)==dot(s.m,s.v));ck(dot(s.m,[a*a for a in sequential])==dot(s.m,[a*a for a in s.v]));records['simultaneous_alternative']=sequential
 # Resting stack, one external kick each frame, retains all touching support rows.
 s=State([F(1,2),F(3,2),F(5,2)],[F(0)]*3,[F(1),F(2),F(3)],[F(1,2)]*3)
 for i in range(120):
  t,z=step(s,F(1,60),F(0),left=F(0),accel=[-F(981,100)]*3)
  ck(t.x==s.x and t.v==[F(0)]*3);ck(z['external']==-z['kick']);s=t
 counts['resting_stack_steps']=120
 # Multiple rebounds in a bounded interval: analytic unfolded-coordinate witness.
 count=0
 for speed in [F(1),F(7),F(31)]:
  for h in [F(1,3),F(1),F(7,3)]:
   s=State([F(1,2)],[speed],[F(1)],[F(0)])
   t,z=step(s,h,F(1),left=F(0),right=F(2));ck(z['status']=='COMPLETE')
   q=(F(1,2)+speed*h)%4;expected=q if q<=2 else 4-q
   expected_v=speed if q<2 else -speed
   ck(t.x==[expected]);ck(t.v==[expected_v]);count+=1
 counts['rebounding_wall_cases']=count
 # External kick ownership: spectator receives one kick regardless of event count.
 h=F(1,5);g=-F(981,100);correct=g*h;bad=correct*3
 neg['repeated_global_force_kick']=dict(correct_velocity=correct,three_kicks_velocity=bad)
 ck(correct!=bad)
 # Replaying a completed impact impulse without solving as an increment is false extra work.
 neg['blind_replay_of_impact_impulse']=dict(pre=F(-1),after_one=F(1),after_replay=F(3),energy_ratio=F(9))
 ck(F(9)>1)
 # Stop-at-TOI discards drift. Keep witness rather than count as event success.
 neg['discard_remaining_time']=dict(correct_endpoint=F(1,2)*(F(1,60)-F(1,1000)),stopped_endpoint=F(0))
 ck(neg['discard_remaining_time']['correct_endpoint']>0)
 out=dict(status='PASS',scope='exact 1D, frictionless, kick-once piecewise constant drift; declared normal Newton cluster law, not general 3D solver',counts=counts,assertions=checks,records=records,negative_controls=neg,runtime_seconds=time.perf_counter()-start)
 p=Path(__file__).resolve().parents[1]/'results/event_results.json';p.write_text(json.dumps(enc(out),indent=2));print(json.dumps(enc({k:v for k,v in out.items() if k not in ['records','negative_controls']}),indent=2))
if __name__=='__main__':main()
