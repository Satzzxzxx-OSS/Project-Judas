"""Exploratory sequential composition of the tested energetic point operator.
Finite-cluster results only. Selection is an explicit model choice, not unique
physical simultaneity. A budget exit is an incomplete result, never acceptance.
"""
import json, time
from pathlib import Path
import numpy as np
from normal_blocks import world, budgets
from spatial_point import spatial_impact

def contact_jacobian(n,a,b,point,normal,pos):
    seed=np.eye(3)[np.argmin(np.abs(normal))]
    t=np.cross(normal,seed);t/=np.linalg.norm(t);s=np.cross(normal,t)
    rows=[]
    for d in [normal,t,s]:
        row=np.zeros(6*n);row[6*a:6*a+3]=-d;row[6*a+3:6*a+6]=-np.cross(point-pos[a],d)
        row[6*b:6*b+3]=d;row[6*b+3:6*b+6]=np.cross(point-pos[b],d);rows.append(row)
    return np.array(rows)

def solve(H,Hi,Jlist,v,e,mu,max_events=64):
    vv=v.copy();maxv=max(1.,np.linalg.norm(v));trace=[];initial=.5*v@H@v
    for k in range(max_events):
        us=[J@vv for J in Jlist];closing=[i for i,u in enumerate(us)if u[0]<-1e-8*maxv]
        if not closing:return dict(status='COMPLETE_TO_DECLARED_VELOCITY_TOLERANCE',v=vv,trace=trace)
        # Explicit greedy sequence; not a continuum-material impact-order oracle.
        i=max(closing,key=lambda i:us[i][0]**2/(Jlist[i]@Hi@Jlist[i].T)[0,0])
        A=Jlist[i]@Hi@Jlist[i].T;ei=e[i]if -us[i][0]>.5 else 0.
        try:z=spatial_impact(A,us[i],float(ei),float(mu[i]),tol=1e-6)
        except Exception as ex:return dict(status='POINT_SUBPROBLEM_UNRESOLVED',reason=str(ex),v=vv,trace=trace)
        before=.5*vv@H@vv;vv+=Hi@Jlist[i].T@z['impulse'];after=.5*vv@H@vv
        error=abs(after-before-z['energy_change'])/max(1.,initial)
        if after-before>1e-9*max(1.,initial):raise AssertionError('energy gain in point composition')
        trace.append(dict(contact=i,effective_e=float(ei),normal_before=float(us[i][0]),point_work=z['energy_change'],energy_before=before,energy_after=after,work_residual=error,subproblems=z['subproblems']))
    return dict(status='UNRESOLVED_CLUSTER_BUDGET',v=vv,trace=trace)

def main():
    rng=np.random.default_rng(831903);start=time.perf_counter();rows=[];unresolved=[]
    for case in range(12):
        n=int(rng.integers(2,5));H,Hi,J,m,pos,pairs,points,normals=world(rng,n)
        Js=[contact_jacobian(n,a,b,p,normal,pos)for(a,b),p,normal in zip(pairs,points,normals)]
        v=rng.normal(size=6*n)
        # Manufacture genuinely approaching simultaneous contacts, rather than
        # accepting empty/open contact graphs as useful impact evidence.
        Jn=np.array([j[0]for j in Js]);desired=-rng.uniform(.5,3,len(Js))
        v+=Hi@Jn.T@np.linalg.solve(Jn@Hi@Jn.T,desired-Jn@v)
        assert np.max(Jn@v)<-.49
        e=rng.choice([0.,.2,.5,1.],len(Js));mu=rng.choice([0.,.2,.5,.8],len(Js))
        out=solve(H,Hi,Js,v,e,mu);k0,p0,l0=budgets(H,pos,m,v);k1,p1,l1=budgets(H,pos,m,out['v'])
        scale=max(1.,sum(m)*max(1.,np.linalg.norm(v)));p_err=float(np.linalg.norm(p1-p0)/scale);l_err=float(np.linalg.norm(l1-l0)/(scale*(1+max(np.linalg.norm(pos,axis=1)))))
        row=dict(case=case,bodies=n,contacts=len(Js),status=out['status'],events=len(out['trace']),energy_before=k0,energy_after=k1,initial_normal_velocities=(Jn@v).tolist(),linear_error=p_err,angular_error=l_err,trace=out['trace'])
        if out['status']!='COMPLETE_TO_DECLARED_VELOCITY_TOLERANCE':unresolved.append(dict(case=case,status=out['status'],reason=out.get('reason')))
        rows.append(row)
    result=dict(status='EXPLORATORY_COUPLED_IMPACTS',cases=len(rows),complete=len(rows)-len(unresolved),unresolved=unresolved,
      max_events=max(r['events']for r in rows),total_events=sum(r['events']for r in rows),
      max_linear_error=max(r['linear_error']for r in rows),max_angular_error=max(r['angular_error']for r in rows),
      runtime_s=time.perf_counter()-start,scope='Small fixed-geometry contact graphs, sequential energetic single-point composition; supplied contact geometry, not engine detection. Does not settle simultaneous symmetry, general progress, costs or persistent supports.',rows=rows)
    root=Path(__file__).resolve().parents[1];(root/'results/coupled_spatial_experiment.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items()if k!='rows'},indent=2))
if __name__=='__main__':main()
