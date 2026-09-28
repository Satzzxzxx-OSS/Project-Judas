from event_reference import *
from event_checks import enc
import random,json,time
from pathlib import Path

def main():
 start=time.perf_counter();rng=random.Random(426220);summary={}
 states=[]
 for trial in range(180):
  n=rng.randrange(2,6);r=[F(rng.randrange(1,4),10) for _ in range(n)];x=[F(0)]
  for i in range(1,n):x.append(x[-1]+r[i-1]+r[i]+F(rng.randrange(0,5),10))
  v=[F(rng.randrange(-6,7)) for _ in range(n)];m=[rng.choice([F(1,10),F(1),F(10)]) for _ in range(n)]
  e=rng.choice([F(0),F(1,2),F(1)]);states.append((State(x,v,m,r),e))
 for model in ['newton','projection_rebound_candidate']:
  complete=0;badenergy=[];events=0;incomplete=[]
  for i,(s,e) in enumerate(states):
   t,z=step(s,F(1,5),e,model=model,capture=F(1,2))
   assert dot(s.m,t.v)==dot(s.m,s.v)
   if z['status']=='COMPLETE':complete+=1
   else:incomplete.append(i)
   events+=len(z['events'])
   if any(q['energy_change']>0 for q in z['events']):badenergy.append(i)
  summary[model]=dict(cases=180,completed=complete,incomplete=incomplete,events=events,energy_counterexamples=badenergy)
 assert summary['projection_rebound_candidate']['completed']==180
 assert not summary['projection_rebound_candidate']['energy_counterexamples']
 # The source-style law still has an energy witness; low-speed threshold does not fix it.
 assert summary['newton']['energy_counterexamples']
 result=dict(status='RESEARCH_COMPLETE',models=summary,capture_speed=.5,
   caveat='The research projection-rebound candidate selects common event restitution using max closing speed. This is not equivalent to the production per-contact restitution policy and has no friction/3D approval.',runtime_seconds=time.perf_counter()-start)
 (Path(__file__).resolve().parents[1]/'results/capture_results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
if __name__=='__main__':main()
