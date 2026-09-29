"""Own experimental work-consistent point-impact discretization.
Backward Coulomb cone subproblem, dissipative linear-impulse intervals, and energetic normal return. No claim of a
published verbatim algorithm, multipoint coupling, finite production cost or
production integration. Compare to the exact planar interval reference.
"""
from __future__ import annotations
import numpy as np, math, json, time
from pathlib import Path
from energetic_point import energetic_point, plain

class Unresolved(Exception): pass

def spatial_impact(A,u0,e,mu,tol=1e-6,max_work=20000):
    A=np.asarray(A,float);u0=np.asarray(u0,float);p=np.zeros(3);u=u0.copy()
    C=A[1:,1:];b=A[1:,0];d,V=np.linalg.eigh(C)
    if np.min(np.linalg.eigvalsh(A))<=0 or u0[0]>=0 or not 0<=e<=1 or mu<0:
        raise ValueError('positive mobility, approaching velocity and admissible material parameters required')
    vscale=max(1e-12,float(np.linalg.norm(u0)));pscale=vscale/np.linalg.eigvalsh(A)[0];escale=vscale*pscale
    work=0
    def increment(uu,q):
        nonlocal work
        work+=1
        if work>max_work:raise Unresolved('reference work budget')
        h=V.T@(uu[1:]+q*b);limit=mu*q
        if limit==0:r=np.zeros(2)
        else:
            rloc=-h/d
            if np.linalg.norm(rloc)>limit:
                lo=0.;hi=max(float(np.linalg.norm(h))/limit,float(np.max(d)))
                while np.linalg.norm(h/(d+hi))>limit:hi*=2
                for _ in range(52):
                    mid=(lo+hi)/2
                    if np.linalg.norm(h/(d+mid))>limit:lo=mid
                    else:hi=mid
                rloc=-h/(d+hi)
            r=V@rloc
        dp=np.r_[q,r];unew=uu+A@dp;W=dp*(uu+unew)/2
        bad=max(float(r@uu[1:]),float(r@unew[1:]),0.)/escale
        return unew,dp,float(W[0]),float(sum(W[1:])),bad
    def two(uu,q):
        v,p1,n1,t1,b1=increment(uu,q/2);v,p2,n2,t2,b2=increment(v,q/2)
        return v,p1+p2,n1+n2,t1+t2,max(b1,b2)
    Wn=Wt=0.;Ec=None;phase='compression';steps=rejects=0;q=pscale/32;max_cone_excess=0.
    for _ in range(12000):
        if phase=='release' and (e==0 or Wn+Ec>=e*e*Ec-tol*escale*1e-4):break
        full=increment(u,q);half=two(u,q)
        error=max(np.linalg.norm(full[0]-half[0])/vscale,abs(full[2]-half[2])/escale)
        if half[4]>1e-12 or error>tol:
            q*=.5 if half[4]>1e-12 else max(.1,.8*math.sqrt(tol/error));rejects+=1;continue
        event=(phase=='compression' and half[0][0]>=0) or (phase=='release' and Wn+Ec+half[2]>=e*e*Ec)
        if event:
            lo=0.;hi=q
            for _ in range(52):
                mid=(lo+hi)/2;trial=two(u,mid)
                val=trial[0][0] if phase=='compression' else Wn+Ec+trial[2]-e*e*Ec
                if val>=0:hi=mid
                else:lo=mid
            q=(lo+hi)/2;half=two(u,q)
            # An event shortens the interval and may change its slip modes.
            # Re-check the local error on the interval ACTUALLY accepted.
            full=increment(u,q)
            error=max(np.linalg.norm(full[0]-half[0])/vscale,abs(full[2]-half[2])/escale)
            if half[4]>1e-12 or error>tol:
                q*=.5 if half[4]>1e-12 else max(.1,.8*math.sqrt(tol/error));rejects+=1;continue
        u=half[0];p+=half[1];Wn+=half[2];Wt+=half[3];steps+=1
        max_cone_excess=max(max_cone_excess,float(np.linalg.norm(half[1][1:])-mu*half[1][0]))
        if phase=='compression' and event:
            Ec=-Wn;phase='release'
            if Ec<=0:raise Unresolved('nonpositive compression energy')
        elif phase=='release' and event:break
        elif phase=='release' and u[0]<-tol*vscale:raise Unresolved('renewed normal compression')
        if q<=np.finfo(float).eps*pscale:raise Unresolved('impulse progress below representation')
        q*=min(2.,max(.5,.9*math.sqrt(tol/max(error,1e-30))))
    else:raise Unresolved('step budget')
    delta=float(p@u0+.5*p@A@p)
    expected=-(1-e*e)*Ec+Wt
    if abs(delta-expected)>20*tol*escale:raise Unresolved('energy budget')
    if delta>20*tol*escale:raise Unresolved('energy gain')
    return dict(velocity=u,impulse=p,energy_change=delta,expected_change=expected,energy_identity_error=abs(delta-expected)/escale,
      friction_work=Wt,normal_work=Wn,compression_energy=Ec,accepted_steps=steps,rejected_steps=rejects,subproblems=work,max_cone_excess=max_cone_excess)

def main():
    rng=np.random.default_rng(626715);start=time.perf_counter();planar=[];spatial=[];fail=[]
    for k in range(12):
        m=10**rng.uniform(-1,1);r=rng.normal(size=2);I=m*10**rng.uniform(-.4,.4)
        J=np.array([[0,1,r[0]],[1,0,-r[1]]]);Ap=J@np.diag([1/m,1/m,1/I])@J.T
        A=np.eye(3);A[:2,:2]=Ap;u=np.r_[-rng.uniform(.5,3),rng.uniform(-3,3),0.];e=float(rng.choice([.2,.5,1.]));mu=float(rng.choice([.2,.5,.8]))
        ep=energetic_point(Ap,u[:2],e,mu);runs=[]
        for tol in [1e-4,1e-5,1e-6]:
            try:
                z=spatial_impact(A,u,e,mu,tol);err=float(np.linalg.norm(z['velocity'][:2]-ep['velocity'])/max(1,np.linalg.norm(ep['velocity'])))
                runs.append(dict(tolerance=tol,error_vs_exact_planar=err,**plain(z)))
            except Exception as ex:fail.append(dict(kind='planar',case=k,tol=tol,reason=str(ex)));break
        planar.append(dict(case=k,A=A.tolist(),u=u.tolist(),e=e,mu=mu,runs=runs))
    for k in range(48):
        m=10**rng.uniform(-2,3);r=rng.normal(size=3);z=rng.normal(size=(3,3));Q,_=np.linalg.qr(z);Hi=Q@np.diag(1/(m*10**rng.uniform(-.5,.5,3)))@Q.T
        rx=np.array([[0,-r[2],r[1]],[r[2],0,-r[0]],[-r[1],r[0],0.]])
        A=np.eye(3)/m-rx@Hi@rx;u=np.r_[-rng.uniform(.5,3),rng.normal(size=2)*2];e=float(rng.choice([0,.2,.5,1.]));mu=float(rng.choice([0,.2,.5,.8,1.]))
        try:
            s=spatial_impact(A,u,e,mu,1e-6)
            theta=rng.uniform(-math.pi,math.pi);R=np.eye(3);R[1:,1:]=[[math.cos(theta),-math.sin(theta)],[math.sin(theta),math.cos(theta)]]
            t=spatial_impact(R@A@R.T,R@u,e,mu,1e-6)
            cov=float(np.linalg.norm(t['velocity']-R@s['velocity'])/max(1,np.linalg.norm(s['velocity'])))
            spatial.append(dict(case=k,e=e,mu=mu,tangent_rotation_error=cov,**plain(s)))
        except Exception as ex:fail.append(dict(kind='spatial',case=k,reason=str(ex)))
    out=dict(status='EXPERIMENTAL_RESULTS_NOT_PRODUCTION_ACCEPTANCE',planar_cases=len(planar),spatial_cases=len(spatial),failures=fail,
      max_planar_errors_by_tolerance=[max([r['error_vs_exact_planar'] for c in planar for r in c['runs'] if r['tolerance']==tol],default=0) for tol in [1e-4,1e-5,1e-6]],
      max_tangent_rotation_error=max([s['tangent_rotation_error'] for s in spatial],default=0),
      max_subproblems=max([s['subproblems'] for s in spatial]+[r['subproblems'] for c in planar for r in c['runs']],default=0),
      max_energy_identity_error=max([s['energy_identity_error'] for s in spatial],default=0),runtime_s=time.perf_counter()-start,
      scope='Own adaptive backward-cone impulse discretization with nonpositive endpoint friction work: single 3D contact, stationary support/equivalent SPD mobility. No multiple frictional contact or event-progress proof. Parameter e is energetic, not universally a normal velocity ratio.',
      planar=planar,spatial=spatial)
    root=Path(__file__).resolve().parents[1];(root/'results/spatial_point.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:v for k,v in out.items() if k not in ['planar','spatial']},indent=2))
if __name__=='__main__':main()
