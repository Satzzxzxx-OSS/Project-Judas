#!/usr/bin/env python3
from event_reference import *
from event_checks import enc
from pathlib import Path
import json,time

def segments(initial,log,h):
    x=initial.x[:];v=initial.v[:];last=F(0);out=[]
    for event in log['events']:
        t=event['time']
        if t>last:out.append(dict(start=last,end=t,x=x[:],v=v[:]))
        x=[a+b*(t-last) for a,b in zip(x,v)];v=event['after'][:];last=t
    if h>last:out.append(dict(start=last,end=h,x=x[:],v=v[:]))
    return out

def evaluate(segs,t):
    for seg in segs:
        if seg['start']<=t<=seg['end']:
            return [x+v*(t-seg['start']) for x,v in zip(seg['x'],seg['v'])]
    raise ValueError(t)

def main():
    start=time.perf_counter();cases=[];checks=0
    for speed in [F(2),F(10),F(30)]:
        for bounce_count in [1,2]:
            s=State([F(1,2)],[speed],[F(1)],[F(1,10)])
            # first wall hit is 7/(5*speed); additional wall crossing length 9/5.
            h=(F(14,5)+F(18,5)*(bounce_count-1))/speed
            out,log=step(s,h,F(1),left=F(0),right=F(2));assert log['status']=='COMPLETE'
            segs=segments(s,log,h)
            for k in range(101):
                t=h*F(k,100);x=evaluate(segs,t)[0]
                # unfolded centre coordinate in interval [.1,1.9]
                q=(F(2,5)+speed*t)%F(18,5)
                oracle=F(1,10)+(q if q<=F(9,5) else F(18,5)-q)
                assert x==oracle;checks+=1
            lo=min(seg['x'][0]+min(F(0),seg['v'][0]*(seg['end']-seg['start'])) for seg in segs)-s.r[0]
            hi=max(seg['x'][0]+max(F(0),seg['v'][0]*(seg['end']-seg['start'])) for seg in segs)+s.r[0]
            endlo=min(s.x[0],out.x[0])-s.r[0];endhi=max(s.x[0],out.x[0])+s.r[0]
            assert hi>endhi;checks+=1
            at=log['events'][0]['time'];actual=evaluate(segs,at)[0];lerp=s.x[0]+(out.x[0]-s.x[0])*at/h
            assert actual!=lerp;checks+=1
            cases.append(dict(speed=speed,h=h,events=len(log['events']),actual_at_first_hit=actual,
              endpoint_lerp_at_first_hit=lerp,history_bound=[lo,hi],endpoint_bound=[endlo,endhi],final=out.x))
    result=dict(status='PASS',cases=len(cases),assertions=checks,records=cases,
        scope='Exact 1D analytic motion-history oracle; current player/renderer/PhysicsWorld not executed',runtime_seconds=time.perf_counter()-start)
    (Path(__file__).resolve().parents[1]/'results/motion_ledger_results.json').write_text(json.dumps(enc(result),indent=2));print(json.dumps(enc(result),indent=2))
if __name__=='__main__':main()
