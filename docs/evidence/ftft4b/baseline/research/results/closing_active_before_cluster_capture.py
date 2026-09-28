#!/usr/bin/env python3
"""Exact 1-D head-on rigid-impact reference. NOT a Judas replacement.
Fraction arithmetic; closed contacts solved together via active-set LCP.
No friction/rotation in this driver. Shared-source geometry is not imported.
"""
from fractions import Fraction as F
from itertools import combinations
from dataclasses import dataclass
from typing import Optional

def dot(a,b): return sum((x*y for x,y in zip(a,b)), F(0))

def solve_linear(A,b):
    n=len(b); a=[list(row)+[y] for row,y in zip(A,b)]
    for col in range(n):
        pivot=next((r for r in range(col,n) if a[r][col]),None)
        if pivot is None:return None
        a[col],a[pivot]=a[pivot],a[col]
        scale=a[col][col]; a[col]=[x/scale for x in a[col]]
        for r in range(n):
            if r!=col and a[r][col]:
                s=a[r][col]; a[r]=[x-s*y for x,y in zip(a[r],a[col])]
    return [a[r][-1] for r in range(n)]

def impact(m,v,J,offsets,e,threshold=F(0)):
    """Normal Newton complementarity at coincident contacts, e per row.
    Exact small active-set reference. Does not claim unique physical law for
    genuinely simultaneous real material impacts.
    """
    u=[dot(j,v)+b for j,b in zip(J,offsets)]
    target=[-ei*ui if ui < -threshold else F(0) for ei,ui in zip(e,u)]
    G=[[sum((ja[k]*jb[k]/m[k] for k in range(len(m))),F(0)) for jb in J] for ja in J]
    q=[ui-ti for ui,ti in zip(u,target)]
    for size in range(len(J)+1):
        for chosen in combinations(range(len(J)),size):
            lam=[F(0)]*len(J)
            sol=solve_linear([[G[i][j] for j in chosen] for i in chosen],[-q[i] for i in chosen])
            if sol is None or any(x<0 for x in sol):continue
            for i,x in zip(chosen,sol):lam[i]=x
            y=[qi+dot(row,lam) for qi,row in zip(q,G)]
            if any(x<0 for x in y):continue
            assert all(l*y0==0 for l,y0 in zip(lam,y))
            out=[vi+sum((J[j][i]*lam[j] for j in range(len(J))),F(0))/m[i] for i,vi in enumerate(v)]
            return out,lam,u,target
    raise ArithmeticError('No exact feasible LCP active set in supplied fixture')

@dataclass
class State:
    x:list
    v:list
    m:list
    r:list
    def copy(self):return State(self.x[:],self.v[:],self.m[:],self.r[:])

def constraints(s,left=None,right=None):
    n=len(s.x);c=[]
    if left is not None:
        j=[F(0)]*n;j[0]=F(1);c.append(('left',s.x[0]-s.r[0]-left,j,F(0)))
    for i in range(n-1):
        j=[F(0)]*n;j[i]=F(-1);j[i+1]=F(1)
        c.append((f'{i}:{i+1}',s.x[i+1]-s.x[i]-s.r[i]-s.r[i+1],j,F(0)))
    if right is not None:
        j=[F(0)]*n;j[-1]=F(-1);c.append(('right',right-s.x[-1]-s.r[-1],j,F(0)))
    return c

def step(state,h,e=F(1),left=None,right=None,accel=None,max_events=128,
         frozen_pairs=False, row_reverse=False, model="newton", capture=F(0), pair_restitution=None):
    if model not in ("newton", "projection_rebound_candidate", "closing_active"): raise ValueError(model)
    s=state.copy();elapsed=F(0);events=[];external=F(0);kick=F(0)
    if accel is not None:
        for i,a in enumerate(accel):s.v[i]+=a*h;kick+=s.m[i]*a*h
    allowed=None
    if frozen_pairs:
        allowed={name for name,g,j,b in constraints(s,left,right)
                 if dot(j,s.v)+b<0 and -g/(dot(j,s.v)+b)<=h}
    initial_energy=sum((m*v*v/F(2) for m,v in zip(s.m,s.v)),F(0))
    while elapsed<h or not events:
        if len(events)>=max_events:
            return s,dict(status='UNRESOLVED_EVENT_BUDGET',consumed=elapsed,remaining=h-elapsed,
                          events=events,external=external,kick=kick,initial_kicked_energy=initial_energy)
        cs=constraints(s,left,right)
        if any(g<0 for _,g,_,_ in cs):
            return s,dict(status='PENETRATION',consumed=elapsed,remaining=h-elapsed,events=events,
                          external=external,kick=kick,initial_kicked_energy=initial_energy)
        times=[]
        for name,g,j,b in cs:
            u=dot(j,s.v)+b
            if u<0 and (allowed is None or name in allowed):
                tau=-g/u
                if tau<=h-elapsed: times.append((tau,name))
        if not times:
            rem=h-elapsed
            s.x=[x+v*rem for x,v in zip(s.x,s.v)];elapsed=h
            break
        tau=min(times)[0]
        s.x=[x+v*tau for x,v in zip(s.x,s.v)];elapsed+=tau
        rows=[q for q in constraints(s,left,right) if q[1]==0]
        if model=='closing_active': rows=[q for q in rows if dot(q[2],s.v)+q[3]<=0]
        if row_reverse:rows.reverse()
        J=[q[2] for q in rows]; b=[q[3] for q in rows]
        before=s.v[:];p0=dot(s.m,before);K0=dot(s.m,[v*v/F(2) for v in before])
        if model in ("newton","closing_active"):
            es=[pair_restitution.get(row[0],e) if pair_restitution else e for row in rows]
            s.v,lam,u,targ=impact(s.m,s.v,J,b,es,threshold=capture)
        else:
            # Research candidate, common e and stationary homogeneous constraints only.
            # Compress by M-projection, rebound toward -normal component, reproject.
            # This is NOT Judas's current impact law.
            assert all(x==0 for x in b)
            vc,jc,u,_=impact(s.m,s.v,J,b,[F(0)]*len(J))
            event_e=e if any(dot(j,s.v)+bb < -capture for j,bb in zip(J,b)) else F(0)
            vr=[a+event_e*(a-b0) for a,b0 in zip(vc,s.v)]
            s.v,jr,_,targ=impact(s.m,vr,J,b,[F(0)]*len(J))
            lam=[(1+event_e)*a+b0 for a,b0 in zip(jc,jr)]
        K1=dot(s.m,[v*v/F(2) for v in s.v]);p1=dot(s.m,s.v)
        if left is not None or right is not None:external+=p1-p0
        assert any(x for x in lam),'non-progressing zero impulse event'
        events.append(dict(time=elapsed,rows=[q[0] for q in rows],before=before,after=s.v[:],
                           lambda_=lam,energy_change=K1-K0,momentum_change=p1-p0))
        if elapsed==h:break
    # frozen-pair negative control can finish overlapped; expose it rather than hide.
    overlap=any(g<0 for _,g,_,_ in constraints(s,left,right))
    return s,dict(status='PENETRATION' if overlap else 'COMPLETE',consumed=elapsed,remaining=h-elapsed,
                  events=events,external=external,kick=kick,initial_kicked_energy=initial_energy)
