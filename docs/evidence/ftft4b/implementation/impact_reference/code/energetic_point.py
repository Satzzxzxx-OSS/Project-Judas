"""Independent planar single-point impulse-space research.
Normal energetic restitution (Stronge-type) with exact sliding/sticking interval
end conditions in a 2x2 contact mobility. Not the whole PLUS method, not multi-
contact friction, not a replacement for contact support or current production.
"""
from __future__ import annotations
import math, json,time
from pathlib import Path
import numpy as np

class Unresolved(Exception):pass

def energetic_point(A,u0,e,mu,max_intervals=32):
    A=np.asarray(A,float); u0=np.asarray(u0,float)
    a,b,c=A[0,0],A[0,1],A[1,1]
    if not (a>0 and c>0 and a*c>b*b and u0[0]<0 and 0<=e<=1 and mu>=0):raise ValueError('invalid point problem')
    u=u0.copy();p=np.zeros(2);Wn=0.;Wt=0.;log=[];phase='compression';Ec=None
    at_slip_zero=(u[1]==0.)
    def add(q,k,why):
        nonlocal u,p,Wn,Wt,at_slip_zero
        before=u.copy(); dp=q*np.asarray(k);du=A@dp
        wn=dp[0]*(u[0]+.5*du[0]);wt=dp[1]*(u[1]+.5*du[1])
        p+=dp;u=u0+A@p;Wn+=wn;Wt+=wt
        scale=max(1.,abs(wn),abs(wt),np.linalg.norm(p)*np.linalg.norm(u0))
        if wt>1e-10*scale:raise Unresolved('positive friction work in interval')
        if why=='slip_zero':at_slip_zero=True
        log.append(dict(phase=phase,reason=why,dnormal=q,impulse=dp.tolist(),before=before.tolist(),after=u.tolist(),normal_work=wn,friction_work=wt))
    for count in range(max_intervals):
        if phase=='expansion' and e==0:break
        if at_slip_zero:
            ratio=-b/c
            if abs(ratio)<=mu:
                k=np.array([1.,ratio]);mode='sticking'
            else:
                k=np.array([1.,-mu*np.sign(b)]);mode='impending_slip';at_slip_zero=False
        else:
            k=np.array([1.,-mu*np.sign(u[1])]);mode='sliding'
        du=A@k
        qs=math.inf
        if not at_slip_zero and u[1]*du[1]<0:qs=-u[1]/du[1]
        if phase=='compression':
            qend=-u[0]/du[0] if du[0]>0 else math.inf
            if qend>=0 and qend<=qs:
                add(qend,k,'compression_end');Ec=-Wn
                if Ec<0:raise Unresolved('negative stored compression work')
                phase='expansion';continue
        else:
            remaining=e*e*Ec-(Wn+Ec)
            if remaining<=1e-13*max(1.,Ec):break
            disc=u[0]*u[0]+2*du[0]*remaining
            qend=math.inf
            if disc>=0:
                den=u[0]+math.sqrt(disc)
                if den>0:qend=2*remaining/den
            if qend>=0 and qend<=qs:
                add(qend,k,'expansion_end');break
        if not math.isfinite(qs) or qs<0:raise Unresolved('no valid interval endpoint')
        add(qs,k,'slip_zero')
    else:raise Unresolved('interval budget')
    uactual=u0+A@p
    delta=float(p@u0+.5*p@A@p)
    return dict(impulse=p,velocity=uactual,normal_work=Wn,friction_work=Wt,compression_energy=Ec,energy_change=delta,intervals=log)

def source_single(A,u,e,mu,niter):
    # Same accumulated normal/tangent structure, scalar tangent case.
    w=np.array(u,float);p=np.zeros(2);target=-e*u[0]
    for it in range(niter):
        pn=max(0.,p[0]+(target-w[0])/A[0,0]);d=pn-p[0];p[0]=pn;w+=A[:,0]*d
        pt=np.clip(p[1]-w[1]/A[1,1],-mu*p[0],mu*p[0]);d=pt-p[1];p[1]=pt;w+=A[:,1]*d
    return p,w,float(p@u+.5*p@A@p)

def plain(x):
 if isinstance(x,np.ndarray):return x.tolist()
 if isinstance(x,dict):return {k:plain(v) for k,v in x.items()}
 if isinstance(x,list):return [plain(v) for v in x]
 if isinstance(x,np.generic):return x.item()
 return x

def main():
    start=time.perf_counter();A=np.array([[2.5,1.5],[1.5,2.5]]);u=np.array([-1.,-.1])
    witness=dict(mass=1.,inertia=2/3,lever_arm=[1,-1],normal_velocity=-1.,tangent_velocity=-.1,mu=.8,e=1.,initial_energy=.505)
    for n in [10,100]:
        p,v,dk=source_single(A,u,1.,.8,n);witness[f'source_style_{n}']=dict(impulse=p.tolist(),velocity=v.tolist(),energy_change=dk,final_energy=.505+dk)
    ep=energetic_point(A,u,1.,.8);witness['energetic_reference']=plain(ep);witness['energetic_reference']['final_energy']=.505+ep['energy_change']
    assert ep['energy_change']<=0 and ep['friction_work']<0
    rng=np.random.default_rng(715260);rows=[];fails=[];maxes={k:0. for k in ['energy_identity','normal_budget','reflection','friction_positive','cone_excess']};gainers=0
    for trial in range(2400):
        m=10.**rng.uniform(-2,3);size=10.**rng.uniform(-1,1,2);I=m*np.dot(size,size)/3
        phi=rng.uniform(-1.35,-.15);r=np.array([size[0]*math.cos(phi)+size[1]*math.sin(phi),size[0]*math.sin(phi)-size[1]*math.cos(phi)])
        # normal is +y, tangent +x, point lever r.
        J=np.array([[0,1,r[0]],[1,0,-r[1]]]);Hi=np.diag([1/m,1/m,1/I]);A=J@Hi@J.T
        u=np.array([-10.**rng.uniform(-1,1),rng.uniform(-5,5)]);e=float(rng.choice([0,.2,.5,1.]));mu=float(rng.choice([0,.1,.4,.8,1.2]))
        initial_ke=.5*u@np.linalg.solve(A,u)
        try: ep=energetic_point(A,u,e,mu)
        except Exception as ex:fails.append(dict(trial=trial,error=str(ex),A=A.tolist(),u=u.tolist(),e=e,mu=mu));continue
        scale=max(1.,initial_ke,ep['compression_energy']);expected=-(1-e*e)*ep['compression_energy']+ep['friction_work']
        err=abs(ep['energy_change']-expected)/scale
        assert err<2e-10 and ep['energy_change']<=2e-10*scale
        assert ep['velocity'][0]>=-1e-9*max(1.,abs(u[0]))
        for segment in ep['intervals']:
            pn,pt=segment['impulse'];assert pn>=0 and abs(pt)<=mu*pn+1e-10*max(1.,pn)
        F=np.diag([1.,-1.]);ref=energetic_point(F@A@F,F@u,e,mu)
        rev=np.linalg.norm(ref['velocity']-F@ep['velocity'])/max(1.,np.linalg.norm(ep['velocity']));assert rev<1e-9
        pp,vv,dkk=source_single(A,u,e,mu,100)
        gain=dkk>1e-8*max(1.,initial_ke);gainers+=gain
        for k,x in [('energy_identity',err),('normal_budget',abs(ep['normal_work']+(1-e*e)*ep['compression_energy'])/scale),('reflection',rev),('friction_positive',max(0.,ep['friction_work'])/scale),('cone_excess',max(0.,abs(ep['impulse'][1])-mu*ep['impulse'][0]))]:maxes[k]=max(maxes[k],float(x))
        rows.append(dict(trial=trial,e=e,mu=mu,intervals=len(ep['intervals']),energy_initial=float(initial_ke),energy_change=ep['energy_change'],source_style_change=dkk,source_style_energy_gain=bool(gain),velocity=ep['velocity'].tolist(),normal_work=ep['normal_work'],friction_work=ep['friction_work']))
    out=dict(status='PASS_RESTRICTED_PLANAR_IMPACT_REFERENCE' if not fails else 'INCOMPLETE',cases=len(rows),failed_cases=fails,source_style_energy_gain_cases=gainers,
      max_errors=maxes,max_intervals=max(r['intervals'] for r in rows),witness=witness,
      scope='Single planar off-centre contact, stationary boundary or equivalent SPD 2x2 mobility. Energetic restitution is a constitutive change; no general 3D tangential turn, multi-contact friction, production or event-progress claim.',
      runtime_s=time.perf_counter()-start,cases_detail=rows)
    root=Path(__file__).resolve().parents[1];(root/'results/energetic_point.json').write_text(json.dumps(plain(out),indent=2));print(json.dumps(plain({k:v for k,v in out.items() if k!='cases_detail'}),indent=2))
if __name__=='__main__':main()
