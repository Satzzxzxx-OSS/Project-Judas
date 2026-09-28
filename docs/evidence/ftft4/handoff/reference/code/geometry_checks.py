from geometry_reference import *
import numpy as np, json, random, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def encode(x):
    if isinstance(x,F):return str(x)
    if isinstance(x,tuple):return [encode(z) for z in x]
    if isinstance(x,Box):return {'kind':'box','c':encode(x.c),'h':encode(x.h),'R':encode(x.R)}
    if isinstance(x,Sphere):return {'kind':'sphere','c':encode(x.c),'r':encode(x.r)}
    raise TypeError(type(x))

def run():
    start=time.perf_counter();rng=random.Random(719251)
    pairs=[]
    # Axis-separations across common exact global transformations.
    R=rotation((4,1,2,2))
    for T in [0,137,1024,32768,1048576]:
      for g in [F(-1,1024),F(0),F(1,2**24),F(1,2**16),F(1,1024)]:
        t=vec((T,-T,2*T))
        a=Box(vec((0,0,0)),vec((F(1,2),F(1,3),F(2,5))))
        b=Box(vec((1+g,0,0)),a.h)
        s=Sphere(vec((1+g,0,0)),F(1,2))
        pairs.append(('box_box_gap',transform(a,R,t),transform(b,R,t),g<=0))
        pairs.append(('sphere_box_gap',transform(s,R,t),transform(a,R,t),g<=0))
        pairs.append(('sphere_sphere_gap',transform(Sphere(vec((0,0,0)),F(1,2)),R,t),transform(s,R,t),g<=0))
    # Random rational oriented boxes/spheres; compare exact predicates and
    # interval filter; then check rigid covariance and AABB superset.
    for k in range(200):
        c=vec([F(rng.randint(-20,20),8) for _ in range(3)])
        h=vec([F(rng.randint(1,8),10) for _ in range(3)])
        q=[rng.randint(-5,5) for _ in range(4)]
        if not any(q):q=[1,0,0,0]
        a=Box(c,h,rotation(q))
        d=vec([F(rng.randint(-12,12),10) for _ in range(3)])
        if k%2:b=Sphere(add(c,d),F(rng.randint(1,8),10))
        else:b=Box(add(c,d),h,rotation((3,2,-1,2)))
        pairs.append(('random',a,b,None))
    # Nearly parallel: never delete a separating axis merely for being small.
    for e in [F(1,2**k) for k in (10,20,30,40)]:
        a=Box(vec((0,0,0)),vec((2,F(1,100),F(1,100))))
        for sep in [F(0),F(1,10),F(5)]:
            b=Box(vec((0,sep,0)),a.h,rotation((1,e,e/2,0)))
            pairs.append(('nearly_parallel',a,b,None))
    counts={'pairs':len(pairs),'exact_expected':0,'filter_resolved':0,'exact_fallback':0,'covariance':0,'aabb_superset':0,'interval_enclosures':0}
    out=[]
    Q=rotation((2,3,1,-2));off=vec((200,-137,31))
    for name,a,b,expected in pairs:
        truth=overlap(a,b)
        if expected is not None:assert truth==expected;counts['exact_expected']+=1
        got,fb,ivs=filtered_overlap(a,b)
        assert got==truth;counts['exact_fallback' if fb else 'filter_resolved']+=1
        exacts=[s for s,_ in sat_separations(a,b)] if isinstance(a,Box) and isinstance(b,Box) else [witness(a,b)]
        for s,v in zip(exacts,ivs):
            assert F(v.lo)<=s<=F(v.hi),(name,float(s),v)
            counts['interval_enclosures']+=1
        assert overlap(transform(a,Q,off),transform(b,Q,off))==truth
        counts['covariance']+=1
        assert not truth or aabb_overlap(aabb(a),aabb(b));counts['aabb_superset']+=1
        out.append({'name':name,'a':encode(a),'b':encode(b),'overlap':truth,'filter_fallback':fb,'exact_witness':str(witness(a,b))})
    # Exact compound union must NOT be replaced with its convex hull.
    kids=[Box(vec((-1,0,0)),vec((F(1,5),1,1))),Box(vec((1,0,0)),vec((F(1,5),1,1)))]
    empty=Sphere(vec((0,0,0)),F(1,5));hit=Sphere(vec((F(9,10),0,0)),F(1,5))
    assert not compound_overlap(kids,[empty]);assert compound_overlap(kids,[hit])
    assert compound_overlap(compound_children(off,Q,kids),[transform(hit,Q,off)])
    counts['compound_checks']=3
    # Exact representable binary32 gaps vs the reported gamma8 policy.
    gamma=float(8*2**-24/(1-8*2**-24));rows=[]
    for T in [0,137,1024,32768,1048576]:
        t=np.float32(T);r=np.float32(.5)
        bp=np.float32(T+1);u=np.spacing(bp)
        bp=np.float32(bp+np.float32(2)*u)
        exact=F(float(bp))-F(float(t))-F(1)
        measured=np.float32(np.float32(bp-t)-np.float32(1))
        c=(float(t)+.5+float(bp)-.5)/2
        bound=gamma*(abs(float(t))+abs(c-float(t))+abs(float(bp))+abs(c-float(bp)))
        assert exact>0 and F(float(measured))==exact and float(measured)<=bound
        rows.append({'translation':T,'stored_gap_m':float(exact),'gap_exact_fraction':str(exact),'float32_computed_gap_m':float(measured),'arithmetic_error_m':0.,'gamma8_bound_m':bound,'policy_collapses_real_gap':True})
    # Prior authored detail can already be lost before any contact arithmetic.
    ideal=F(1,1000);rounded=F(float(np.float32(1e6+1+.001)))-F(1000000)-1
    assert rounded==0
    results={'status':'PASS_INDEPENDENT_REFERENCE_ONLY','counts':counts,'gamma8_witnesses':rows,'input_quantization_control':{'requested_gap_m':float(ideal),'stored_gap_m':float(rounded),'translation':1e6},'seconds':time.perf_counter()-start}
    (ROOT/'results/geometry_results.json').write_text(json.dumps(results,indent=2))
    (ROOT/'results/geometry_fixtures.json').write_text(json.dumps(out,indent=2))
    print(json.dumps(results,indent=2))
if __name__=='__main__':run()
