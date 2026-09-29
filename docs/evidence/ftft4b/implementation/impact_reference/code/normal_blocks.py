"""Independent research: simultaneous normal blocks on approaching/resting rows.
No production code changes. No friction in this file. Small exhaustive active sets
are an oracle, not an engine algorithm or performance recommendation.
"""
from __future__ import annotations
import itertools, json, math, time
from pathlib import Path
import numpy as np

def lcp(G, q):
    n=len(q); scale=max(1.,np.max(np.abs(q),initial=0.))
    for k in range(n+1):
        for ids0 in itertools.combinations(range(n),k):
            ids=list(ids0); p=np.zeros(n)
            if ids:
                g=G[np.ix_(ids,ids)]
                x,*_=np.linalg.lstsq(g,-q[ids],rcond=1e-13)
                if np.max(np.abs(g@x+q[ids]))>1e-9*scale: continue
                if np.min(x)<-1e-10*max(1.,np.max(np.abs(x))):continue
                p[ids]=x
            z=G@p+q
            if np.min(z,initial=0)<-1e-9*scale:continue
            if abs(p@z)>1e-8*max(1.,np.linalg.norm(p)*scale):continue
            return p,z
    raise ArithmeticError('No feasible active set to declared tolerance')

def skew(x):
    a,b,c=x;return np.array([[0,-c,b],[c,0,-a],[-b,a,0.]])

def world(rng,n):
    positions=rng.normal(size=(n,3))*2
    masses=10.**rng.uniform(-2,4,n)
    H=np.zeros((6*n,6*n)); Hi=H.copy()
    for i,m in enumerate(masses):
        z=rng.normal(size=(3,3));Q,_=np.linalg.qr(z)
        I=Q@np.diag(m*10.**rng.uniform(-1,1,3))@Q.T
        H[6*i:6*i+3,6*i:6*i+3]=m*np.eye(3)
        H[6*i+3:6*i+6,6*i+3:6*i+6]=I
        Hi[6*i:6*i+6,6*i:6*i+6]=np.linalg.inv(H[6*i:6*i+6,6*i:6*i+6])
    pairs=[(i,i+1) for i in range(n-1)]
    if n>2 and rng.random()<.5:pairs.append((0,n-1))
    rows=[];points=[];normals=[]
    for a,b in pairs:
        p=.5*(positions[a]+positions[b])+rng.normal(size=3)
        normal=positions[b]-positions[a];normal/=np.linalg.norm(normal)
        j=np.zeros(6*n);ra=p-positions[a];rb=p-positions[b]
        j[6*a:6*a+3]=-normal;j[6*a+3:6*a+6]=-np.cross(ra,normal)
        j[6*b:6*b+3]=normal;j[6*b+3:6*b+6]=np.cross(rb,normal)
        rows.append(j);points.append(p);normals.append(normal)
    return H,Hi,np.array(rows),masses,positions,pairs,points,normals

def budgets(H,positions,masses,v):
    k=float(.5*np.longdouble(v)@np.longdouble(H)@np.longdouble(v))
    p=np.zeros(3);L=np.zeros(3)
    for i,m in enumerate(masses):
        vv=v[i*6:i*6+3];w=v[i*6+3:i*6+6];pi=m*vv
        p+=pi;L+=np.cross(positions[i],pi)+H[i*6+3:i*6+6,i*6+3:i*6+6]@w
    return k,p,L

def run():
    t=time.perf_counter();rng=np.random.default_rng(432715)
    maxes={k:0. for k in ['normal_lcp','energy_identity','linear_momentum','angular_momentum','row_order','rotation','boost']}
    records=[];closures=0
    for trial in range(600):
        n=int(rng.integers(2,6));H,Hi,J,m,pos,pairs,points,normals=world(rng,n)
        # Make an arbitrary realized generalized velocity. Select only rows
        # approaching or at rest; opening rows get no stale Newton target.
        v=rng.normal(size=6*n)*3;u=J@v;ids=np.where(u<=0.)[0]
        if not len(ids):v=-v;u=-u;ids=np.where(u<=0.)[0]
        A=J[ids];un=u[ids];e=rng.choice([0.,.2,.5,1.],len(ids))
        G=A@Hi@A.T;p,z=lcp(G,(1+e)*un);dv=Hi@A.T@p;vp=v+dv
        k0,P0,L0=budgets(H,pos,m,v);k1,P1,L1=budgets(H,pos,m,vp)
        W=float(np.longdouble(p)@(np.longdouble(un)+np.longdouble(A@vp))/2)
        certified=float(.5*np.longdouble(p)@((1-e)*np.longdouble(un)))
        scale=max(1.,k0,np.linalg.norm(v)*np.linalg.norm(H@dv))
        err=abs((k1-k0)-W)/scale
        assert k1-k0 <= 2e-9*scale
        assert abs(W-certified)<2e-8*scale
        perr=np.linalg.norm(P1-P0)/max(1.,sum(m)*max(np.linalg.norm(v),1))
        lerr=np.linalg.norm(L1-L0)/max(1.,sum(m)*(np.max(np.linalg.norm(pos,axis=1))+1)*max(np.linalg.norm(v),1))
        assert perr<1e-10 and lerr<1e-10
        residual=max(0.,-float(np.min(z,initial=0)))/max(1.,np.linalg.norm(un))
        order=np.arange(len(ids))[::-1];pr,zr=lcp(G[np.ix_(order,order)],((1+e)*un)[order]);vr=v+Hi@A[order].T@pr
        oerr=np.linalg.norm(vr-vp)/max(1.,np.linalg.norm(vp));assert oerr<1e-8
        # Rigid rotation of EVERY vector/tensor/operator, independent of row order.
        qr,_=np.linalg.qr(rng.normal(size=(3,3)));qr[:,0]*=np.linalg.det(qr)
        S=np.kron(np.eye(2*n),qr)
        Ar=A@S.T;Hr=S@H@S.T;Hir=S@Hi@S.T;vrot=S@v
        p2,z2=lcp(Ar@Hir@Ar.T,(1+e)*(Ar@vrot));vp2=vrot+Hir@Ar.T@p2
        rerr=np.linalg.norm(vp2-S@vp)/max(1.,np.linalg.norm(vp));assert rerr<1e-8
        b=np.zeros(6*n);b.reshape(n,6)[:,:3]=[2,-1,3]
        pb,zb=lcp(G,(1+e)*(A@(v+b)));vpb=v+b+Hi@A.T@pb
        berr=np.linalg.norm(vpb-vp-b)/max(1.,np.linalg.norm(vp));assert berr<1e-8
        for key,x in [('normal_lcp',residual),('energy_identity',err),('linear_momentum',perr),('angular_momentum',lerr),('row_order',oerr),('rotation',rerr),('boost',berr)]:maxes[key]=max(maxes[key],float(x))
        newclosing=int(np.count_nonzero((J@vp)<-1e-8));closures+=newclosing>0
        records.append(dict(trial=trial,bodies=n,contacts=len(J),active_rows=len(ids),e=e.tolist(),K_before=k0,K_after=k1,normal_work=W,predicted_work=certified,new_closing_rows=newclosing))
    out=dict(status='PASS_RESTRICTED_NORMAL_BLOCKS',cases=len(records),variant_evaluations=4*len(records),max_errors=maxes,blocks_inducing_other_contacts=closures,
      scope='Fixed-geometry frictionless normal impulse blocks. Actual event closure and friction are NOT certified by these block tests; new closing contacts are explicitly counted.',runtime_s=time.perf_counter()-t,cases_detail=records)
    root=Path(__file__).resolve().parents[1];(root/'results/normal_blocks.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:v for k,v in out.items() if k!='cases_detail'},indent=2))
if __name__=='__main__':run()
