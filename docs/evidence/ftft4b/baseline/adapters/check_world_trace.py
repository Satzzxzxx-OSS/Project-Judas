"""Turn the diagnostic PhysicsWorld CSV into explicit pre/post evidence.
Not executed by the reviewer because the real-GLM build is unavailable.
Modes: baseline = demonstrate known defects; geometry = FTFT4A gap witness;
full = also enforce isolated temporal and chain-of-impact oracles for FTFT4B.
"""
import argparse,csv,math,json
p=argparse.ArgumentParser();p.add_argument('csv');p.add_argument('--mode',choices=['baseline','geometry','full'],required=True);a=p.parse_args()
rows=list(csv.reader(open(a.csv)));imp=[r for r in rows if r[0]=='impact'];gap=[r for r in rows if r[0]=='gap'];chain=[r for r in rows if r[0]=='chain'];fail=[];checks=0

def check(ok,what):
 global checks
 checks+=1
 if not ok:fail.append(what)

check(len(imp)==8,'8 impact rows');check(len(gap)==3,'3 signed-gap rows');check(len(chain)==3,'3 chain rows')
for r in rows:
 check(all(math.isfinite(float(x)) for x in r[1:]),'finite '+r[0])
h=float.fromhex('0x1.111112p-6') # exact binary32 1/60 used by C++
for r in gap:
 T,g,v0,v1=float(r[1]),float(r[2]),float(r[3]),float(r[4]);n=int(r[5]);pens=[float(x) for x in r[6:]]
 if a.mode=='baseline':
  check(abs(v1)<1e-10,f'baseline premature support T={T}')
  check(n>0 and any(x==0 for x in pens),f'baseline zeroed gap T={T}')
 else:
  check(abs(v1-v0)<=1e-6*abs(v0)+1e-11,f'preserve no-impact velocity T={T}')
  check(all(x<0 for x in pens),f'no false touching T={T}')
for r in imp:
 e,step,g,actual,v=float(r[1]),int(r[2]),float(r[3]),float(r[4]),float(r[5]);first=float(r[7])
 if a.mode=='baseline':
  expected=g+e*h*step
  check(abs(actual-expected)<2e-6,f'baseline early velocity drift e={e} step={step}')
  check(abs(v-e)<2e-6,f'baseline target e={e}')
 elif a.mode=='full':
  expected=first+e*h*(step-1)
  check(abs(actual-expected)<2e-6,f'event end position e={e} step={step}')
  check(abs(v-e)<2e-6,f'event end velocity e={e} step={step}')
if a.mode=='full':
 for r,(x,v) in zip(chain,[(.8,0),(2.6,0),(4.,10)]):
  check(abs(float(r[1])-x)<1e-5,'chain end position')
  check(abs(float(r[2])-v)<1e-5,'chain end velocity')
print(json.dumps({'mode':a.mode,'checks':checks,'failures':fail,'result':'PASS' if not fail else 'FAIL','scope':{'baseline':'Reproduction of existing defects, NOT correct-physics acceptance','geometry':'FTFT4A no-ghost-gap witness ONLY; temporal cases not accepted','full':'Selected central impact/chain witnesses only; not full-engine certification'}[a.mode]},indent=2))
raise SystemExit(bool(fail))
