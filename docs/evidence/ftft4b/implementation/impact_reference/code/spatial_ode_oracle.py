"""Independent continuous impulse-ODE comparison on strictly sliding 3D cases.
DOP853 is a supporting numerical oracle, not an exact closed-form reference.
The strictly positive slip bound is constructed before running either method.
"""
import json,time
from pathlib import Path
import numpy as np
from scipy.integrate import solve_ivp
from spatial_point import spatial_impact

def oracle(A,u,e,mu):
    a=A[0,0];b=A[1:,0];C=A[1:,1:]
    lower=a-mu*np.linalg.norm(b)
    assert lower>0
    limit=10*(1+e)*abs(u[0])/lower
    def fun(s,y):
        vel=u+A@y[:3];speed=np.linalg.norm(vel[1:]);dp=np.r_[1.,-mu*vel[1:]/speed]
        return np.r_[dp,vel[0],-mu*speed]
    def comp(s,y):return (u+A@y[:3])[0]
    comp.terminal=True;comp.direction=1
    sol=solve_ivp(fun,[0,limit],np.zeros(5),method='DOP853',events=comp,rtol=1e-12,atol=1e-13)
    assert sol.success and len(sol.t_events[0])==1
    yc=sol.y_events[0][0];Ec=-yc[3]
    if e==0: yf=yc;duration=sol.t_events[0][0]
    else:
        def release(s,y):return y[3]+Ec-e*e*Ec
        release.terminal=True;release.direction=1
        sol2=solve_ivp(fun,[sol.t_events[0][0],limit],yc,method='DOP853',events=release,rtol=1e-12,atol=1e-13)
        assert sol2.success and len(sol2.t_events[0])==1
        yf=sol2.y_events[0][0];duration=sol2.t_events[0][0]
    return dict(velocity=u+A@yf[:3],impulse=yf[:3],normal_work=yf[3],friction_work=yf[4],compression_energy=Ec,impulse_duration=duration)

def main():
    rng=np.random.default_rng(702806);rows=[];start=time.perf_counter();failed=[]
    for k in range(12):
        z=rng.normal(size=(3,3));A=z@z.T+.5*np.eye(3)
        mu=min(.1,A[0,0]/(4*max(1e-8,np.linalg.norm(A[1:,0]))));un=-rng.uniform(.2,2);e=float(rng.choice([.2,.5,1.]))
        lower=A[0,0]-mu*np.linalg.norm(A[1:,0]);qbound=10*(1+e)*abs(un)/lower
        variation=qbound*(np.linalg.norm(A[1:,0])+mu*np.linalg.norm(A[1:,1:],2))
        t=rng.normal(size=2);t/=np.linalg.norm(t);u=np.r_[un,t*(2*variation+5)]
        ref=oracle(A,u,e,mu);runs=[]
        for tol in [1e-4,1e-5,1e-6]:
            try:
                z=spatial_impact(A,u,e,mu,tol,max_work=20000)
                err=float(np.linalg.norm(z['velocity']-ref['velocity'])/max(1,np.linalg.norm(ref['velocity'])))
                impulse_err=float(np.linalg.norm(z['impulse']-ref['impulse'])/max(1,np.linalg.norm(ref['impulse'])))
                runs.append(dict(tol=tol,velocity_error=err,impulse_error=impulse_err,steps=z['accepted_steps'],work=z['subproblems']))
            except Exception as ex:failed.append(dict(case=k,tol=tol,error=str(ex)))
        rows.append(dict(case=k,A=A.tolist(),u=u.tolist(),e=e,mu=mu,guaranteed_slip_lower=float(np.linalg.norm(u[1:])-variation),oracle_velocity=ref['velocity'].tolist(),runs=runs))
    maxima=[max(r['velocity_error'] for row in rows for r in row['runs'] if r['tol']==t) for t in [1e-4,1e-5,1e-6]]
    assert not failed and maxima[2]<maxima[0]
    out=dict(status='PASS_RESTRICTED_SLIDING_ODE_COMPARISON',cases=len(rows),comparisons=sum(len(r['runs'])for r in rows),max_velocity_errors=maxima,failed=failed,rows=rows,runtime_s=time.perf_counter()-start,
      scope='Strictly sliding single 3D contact; continuous Routh ODE via independent DOP853 vs adaptive discrete candidate. No exact/production/multi-contact claim.')
    root=Path(__file__).resolve().parents[1];(root/'results/spatial_ode_oracle.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:v for k,v in out.items()if k!='rows'},indent=2))
if __name__=='__main__':main()
