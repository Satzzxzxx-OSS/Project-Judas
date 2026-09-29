"""Independent FTFT4 impact arithmetic. Not a Judas runtime or stack test.
Analytical kinematics are piecewise constant-velocity drift (after any force kick).
"""
import numpy as np, mpmath as mp, json, math, time
from pathlib import Path
from itertools import product
mp.mp.dps=90
ROOT=Path(__file__).resolve().parents[1]

def toi_target(s,q,dt,e):
    if s<0 or q<0 or dt<=0 or not 0<=e<=1:raise ValueError('invalid')
    if q==0 or s>q*dt:return s-q*dt,-q,None
    t=s/q
    return e*q*(dt-t),e*q,t

def direct_event(s,q,dt,e):
    x=s;v=-q;elapsed=0.
    if v>=0 or x+dt*v>0:return x+dt*v,v,None
    t=-x/v;x+=v*t;elapsed+=t
    impulse=-(1+e)*v;v+=impulse
    x+=v*(dt-elapsed)
    return x,v,t

def sphere_toi(ca,cb,va,vb,ra,rb,dt):
    d=np.asarray(cb)-np.asarray(ca);v=np.asarray(vb)-np.asarray(va);r=ra+rb
    c=float(d@d-r*r);a=float(v@v);b=float(d@v)
    if c<=0:return 0.
    if a==0 or b>=0:return None
    disc=b*b-a*c
    if disc<0:return None
    # Avoid cancellation for early root.
    t=c/(-b+math.sqrt(disc))
    return t if 0<=t<=dt else None

def sphere_toi_mp(ca,cb,va,vb,ra,rb,dt):
    d=[mp.mpf(float(y))-mp.mpf(float(x)) for x,y in zip(ca,cb)]
    v=[mp.mpf(float(y))-mp.mpf(float(x)) for x,y in zip(va,vb)]
    r=mp.mpf(float(ra))+mp.mpf(float(rb));c=sum(x*x for x in d)-r*r
    a=sum(x*x for x in v);b=sum(x*y for x,y in zip(d,v))
    if c<=0:return mp.mpf(0)
    if a==0 or b>=0:return None
    disc=b*b-a*c
    if disc<0:return None
    t=c/(-b+mp.sqrt(disc))
    return t if 0<=t<=mp.mpf(float(dt)) else None

def sphere_box_toi(p,v,h,r,dt):
    """Exact-feature piecewise quadratic for translating sphere vs stationary OBB
    in box-local frame. No angular sweep; no one-off expanded-box corner proxy.
    """
    p=np.asarray(p,float);v=np.asarray(v,float);h=np.asarray(h,float)
    breaks=[0.,dt]
    for k in range(3):
        if v[k]!=0:
            for s in (-1,1):
                t=(s*h[k]-p[k])/v[k]
                if 0<t<dt:breaks.append(float(t))
    breaks=sorted(set(breaks))
    for lo,hi in zip(breaks[:-1],breaks[1:]):
        xm=p+v*(lo+hi)/2
        idx=[k for k in range(3) if abs(xm[k])>h[k]]
        bvec=np.zeros(3);av=np.zeros(3)
        for k in idx:bvec[k]=p[k]-math.copysign(h[k],xm[k]);av[k]=v[k]
        a=float(av@av);b=float(av@bvec);c=float(bvec@bvec-r*r)
        flo=a*lo*lo+2*b*lo+c
        if flo<=1e-13:return lo
        if a==0:continue
        disc=b*b-a*c
        if disc<0:continue
        roots=[(-b-math.sqrt(disc))/a,(-b+math.sqrt(disc))/a]
        for t in roots:
            if lo-1e-13<=t<=hi+1e-13:return max(lo,float(t))
    return None

def impulse(m1,m2,I1,I2,c1,c2,v1,v2,w1,w2,c,n,e,mu=0.):
    r1=c-c1;r2=c-c2
    d=(v2+np.cross(w2,r2))-(v1+np.cross(w1,r1))
    inv1=np.linalg.inv(I1);inv2=np.linalg.inv(I2)
    rn1=np.cross(r1,n);rn2=np.cross(r2,n)
    k=1/m1+1/m2+rn1@inv1@rn1+rn2@inv2@rn2
    vn=d@n
    jn=max(0.,-(1+e)*vn/k)
    J=jn*n
    vv1=v1-J/m1;vv2=v2+J/m2;ww1=w1-inv1@np.cross(r1,J);ww2=w2+inv2@np.cross(r2,J)
    # Optional single tangent direction friction only for dedicated sphere-plane check elsewhere.
    return vv1,vv2,ww1,ww2,jn,k,vn

def energy(m1,m2,I1,I2,v1,v2,w1,w2):return .5*(m1*(v1@v1)+m2*(v2@v2)+w1@I1@w1+w2@I2@w2)
def L(m1,m2,I1,I2,c1,c2,v1,v2,w1,w2):return np.cross(c1,m1*v1)+np.cross(c2,m2*v2)+I1@w1+I2@w2

def chain_events():
    x=np.array([0.,1.,2.]);v=np.array([10.,0.,0.]);r=.1;dt=.2;t=0.;history=[]
    while t<dt-1e-14:
        hits=[]
        for i in range(2):
            closing=v[i]-v[i+1];gap=x[i+1]-x[i]-2*r
            if closing>1e-12:
                tau=max(0.,gap/closing)
                if tau<=dt-t:hits.append((tau,i))
        if not hits:x+=v*(dt-t);t=dt;break
        tau,i=min(hits);x+=v*tau;t+=tau;v[i],v[i+1]=v[i+1],v[i]
        history.append({'t':t,'pair':[i,i+1]})
        if len(history)>20:raise RuntimeError('event loop did not progress')
    assert np.allclose(x,[.8,1.8,2.4],atol=1e-14)
    assert np.array_equal(v,[0.,0.,10.])
    return {'events':history,'positions':x.tolist(),'velocities':v.tolist(),'momentum':float(v.sum()),'energy':float(.5*(v@v))}

def run():
    start=time.perf_counter();rng=np.random.default_rng(53719)
    counters={'scalar_impact_cases':0,'sphere_toi_cases':0,'sphere_box_cases':0,'single_contact_impulse_cases':0,'rotated_impulse_covariance':0,'galilean_impulse_covariance':0,'frictional_sphere_impacts':0}
    scalar_max=0.
    for q,dt,e,frac in product([.01,.49,.51,1.,6.38,100.],[1/30,1/60,1/240],[0.,.2,.5,1.],[0.,.001,.1,.5,.999,1.1]):
        s=q*dt*frac;a=toi_target(s,q,dt,e);b=direct_event(s,q,dt,e)
        er=max(abs(a[0]-b[0]),abs(a[1]-b[1]));scalar_max=max(scalar_max,er)
        assert er<1e-12;counters['scalar_impact_cases']+=1
    # Analytical witnesses for the exact limitation and failed formula, not source re-execution.
    witnesses=[]
    for s,q,dt,e in [(.001,1.,1/60,0.),(.001,1.,1/60,.5),(.1,6.38,1/60,.5)]:
        x,v,t=toi_target(s,q,dt,e)
        early_v=e*q;early_x=s+early_v*dt
        wrong_v=e*q-s/dt;wrong_x=s+wrong_v*dt
        witnesses.append({'gap':s,'closing':q,'dt':dt,'e':e,'toi':t,'physical_end_gap':x,'physical_end_velocity':v,'early_bounce_end_gap':early_x,'rejected_formula_velocity':wrong_v,'rejected_formula_end_gap':wrong_x,'correct_average_drift_velocity':(x-s)/dt})
    max_toi=0.
    for i in range(300):
        # Well-conditioned crossing and miss cases; near tangencies separately logged, not certified.
        ca=rng.normal(size=3);n=rng.normal(size=3);n/=np.linalg.norm(n)
        v=rng.normal(size=3);ra=.2;rb=.3
        cb=ca+n*(.6+rng.random()*3)
        vb=v-n*(.3+rng.random()*10)+rng.normal(size=3)*(.2 if i%2 else 2)
        t=sphere_toi(ca,cb,v,vb,ra,rb,1.);exact=sphere_toi_mp(ca,cb,v,vb,ra,rb,1.)
        assert (t is None)==(exact is None)
        if t is not None:max_toi=max(max_toi,abs(t-float(exact)))
        counters['sphere_toi_cases']+=1
    assert max_toi<1e-11
    # Face, edge and corner linear sweeps have independent symmetric exact distances.
    sb=[]
    for k in (1,2,3):
      for r in (.05,.5):
       for speed in (.5,5.,50.):
        p=[2. if i<k else 0. for i in range(3)];v=[-speed if i<k else 0. for i in range(3)]
        expected=(1-r/math.sqrt(k))/speed
        got=sphere_box_toi(p,v,[1,1,1],r,4)
        assert got is not None and abs(got-expected)<2e-12
        counters['sphere_box_cases']+=1
        sb.append({'feature_dimension':k,'radius':r,'speed':speed,'toi':got,'expected':expected})
    # Expanded AABB has a false corner hit: path has y,z excess .4,.4 > sphere radius .5.
    assert sphere_box_toi([3,1.4,1.4],[-3,0,0],[1,1,1],.5,2.) is None
    counters['sphere_box_cases']+=1
    # Two dynamic generalized bodies, arbitrary off-centre normal contact.
    maxima={'linear':0.,'angular':0.,'energy_law':0.,'restitution':0.,'covariance':0.}
    Q=np.array([[.36,-.48,.8],[.8,.6,0],[-.48,.64,.6]])
    assert np.max(abs(Q.T@Q-np.eye(3)))<1e-15
    for i in range(300):
        m1=10**rng.uniform(-2,4);m2=10**rng.uniform(-2,4)
        I1=np.diag(m1*(.2+rng.random(3)));I2=np.diag(m2*(.2+rng.random(3)))
        c1=rng.normal(size=3);c2=c1+rng.normal(size=3);c=(c1+c2)/2+rng.normal(size=3)*.3
        n=rng.normal(size=3);n/=np.linalg.norm(n)
        v1=rng.normal(size=3);v2=rng.normal(size=3);w1=rng.normal(size=3);w2=rng.normal(size=3)
        vn=((v2+np.cross(w2,c-c2))-(v1+np.cross(w1,c-c1)))@n
        if vn>0:n=-n
        e=[0.,.5,1.][i%3]
        out=impulse(m1,m2,I1,I2,c1,c2,v1,v2,w1,w2,c,n,e);z1,z2,o1,o2,jn,k,vn=out
        E0=energy(m1,m2,I1,I2,v1,v2,w1,w2);E1=energy(m1,m2,I1,I2,z1,z2,o1,o2)
        scale=max(1.,E0)
        le=np.linalg.norm(m1*z1+m2*z2-m1*v1-m2*v2)/max(1.,m1*np.linalg.norm(v1)+m2*np.linalg.norm(v2))
        ae=np.linalg.norm(L(m1,m2,I1,I2,c1,c2,z1,z2,o1,o2)-L(m1,m2,I1,I2,c1,c2,v1,v2,w1,w2))/max(1.,np.linalg.norm(L(m1,m2,I1,I2,c1,c2,v1,v2,w1,w2)))
        expected_loss=-(1-e*e)*vn*vn/(2*k)
        ee=abs(E1-E0-expected_loss)/scale
        re=abs(((z2+np.cross(o2,c-c2))-(z1+np.cross(o1,c-c1)))@n+e*vn)/max(1.,abs(vn))
        qr=impulse(m1,m2,Q@I1@Q.T,Q@I2@Q.T,Q@c1,Q@c2,Q@v1,Q@v2,Q@w1,Q@w2,Q@c,Q@n,e)
        cov=max(np.linalg.norm(qr[k]-Q@out[k]) for k in range(4))
        boost=rng.normal(size=3)*3
        br=impulse(m1,m2,I1,I2,c1,c2,v1+boost,v2+boost,w1,w2,c,n,e)
        cov=max(cov,np.linalg.norm(br[0]-z1-boost),np.linalg.norm(br[1]-z2-boost),np.linalg.norm(br[2]-o1),np.linalg.norm(br[3]-o2))
        for key,value in zip(maxima,[le,ae,ee,re,cov]):maxima[key]=max(maxima[key],value)
        assert max(le,ae,ee,re)<1e-11 and cov<1e-10
        counters['single_contact_impulse_cases']+=1;counters['rotated_impulse_covariance']+=1;counters['galilean_impulse_covariance']+=1
    # Sphere vs fixed plane: tangent impulse from actual contact-point velocity,
    # no friction on a speculative pre-contact gap.
    max_friction_growth=0.
    for m,r,mu,e in product([.1,80.,1e4],[.1,.7],[0.,.2,1.],[0.,.5,1.]):
        n=np.array([0.,1.,0.]);v=np.array([3.,-4.,1.]);w=np.array([0.,0.,2.]);arm=-r*n;I=.4*m*r*r
        E0=.5*m*(v@v)+.5*I*(w@w);vn=v@n;jn=-(1+e)*vn*m
        v=v+jn*n/m
        vt=v+np.cross(w,arm);vt-=n*(vt@n);kt=1/m+r*r/I
        jt=-vt/kt
        if np.linalg.norm(jt)>mu*jn:jt*=mu*jn/np.linalg.norm(jt)
        v+=jt/m;w+=np.cross(arm,jt)/I
        E1=.5*m*(v@v)+.5*I*(w@w)
        assert E1<=E0+1e-9*max(1.,E0)
        assert abs(v@n+e*vn)<1e-11
        max_friction_growth=max(max_friction_growth,(E1-E0)/max(1.,E0))
        counters['frictional_sphere_impacts']+=1
    result={'status':'PASS_INDEPENDENT_REFERENCE_ONLY','counts':counters,'max_scalar_error':scalar_max,'max_sphere_toi_error_s':max_toi,'impulse_normalized_errors':maxima,'max_friction_energy_growth':max_friction_growth,'documented_policy_witnesses':witnesses,'chain_impact':chain_events(),'seconds':time.perf_counter()-start}
    (ROOT/'results/impact_results.json').write_text(json.dumps(result,indent=2));(ROOT/'results/sphere_box_toi_fixtures.json').write_text(json.dumps(sb,indent=2))
    print(json.dumps(result,indent=2))
if __name__=='__main__':run()
