from event_reference import *
from event_checks import enc
import random,json
from pathlib import Path
rng=random.Random(426220)
for trial in range(180):
 n=rng.randrange(2,6);r=[F(rng.randrange(1,4),10) for _ in range(n)];x=[F(0)]
 for i in range(1,n):x.append(x[-1]+r[i-1]+r[i]+F(rng.randrange(0,5),10))
 v=[F(rng.randrange(-6,7)) for _ in range(n)];m=[rng.choice([F(1,10),F(1),F(10)]) for _ in range(n)]
 e=rng.choice([F(0),F(1,2),F(1)]);h=F(1,5);s=State(x,v,m,r);t,z=step(s,h,e)
 if any(q['energy_change']>0 for q in z['events']):
  out=dict(trial=trial,initial=s.__dict__,e=e,h=h,final=t.__dict__,log=z)
  p=Path(__file__).resolve().parents[1]/'results/newton_energy_counterexample.json';p.write_text(json.dumps(enc(out),indent=2));print(json.dumps(enc(out),indent=2));break
