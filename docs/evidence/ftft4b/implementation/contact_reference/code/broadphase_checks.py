"""Independent spatial-bound checks and deterministic tree-adapter fixtures.
NO Judas tree is imported/executed. Expected query sets are exhaustive AABB sets.
"""
from geometry_reference import *
import numpy as np, json, random, time, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def run():
    t0=time.perf_counter();rng=random.Random(919027)
    registry={};ops=[];queries=0;updates=0;creates=0;destroys=0
    nextid=1
    def newbox():
        c=vec([F(rng.randint(-64,64),8) for _ in range(3)])
        h=vec([F(rng.randint(1,12),16) for _ in range(3)])
        return sub(c,h),add(c,h)
    def serial(b):return [[float(x) for x in v] for v in b]
    for frame in range(80):
        # Repeated logical slot reuse is represented with distinct logical ids.
        if len(registry)<24 or frame%4==0:
            b=newbox();registry[nextid]=b
            ops.append({'op':'create','logical_id':nextid,'aabb':serial(b)});creates+=1;nextid+=1
        if registry and frame%3==0:
            k=rng.choice(list(registry));b=newbox();registry[k]=b
            ops.append({'op':'move','logical_id':k,'aabb':serial(b)});updates+=1
        if len(registry)>8 and frame%5==0:
            k=rng.choice(list(registry));del registry[k]
            ops.append({'op':'destroy','logical_id':k});destroys+=1
        for _ in range(5):
            b=newbox();expect=sorted(k for k,v in registry.items() if aabb_overlap(b,v))
            ops.append({'op':'query','aabb':serial(b),'must_include_live_ids':expect,'must_exclude_destroyed_ids':True});queries+=1
    # Query exact face touching as closed overlap.
    a=(vec((0,0,0)),vec((1,1,1)));b=(vec((1,0,0)),vec((2,1,1)))
    assert aabb_overlap(a,b)
    # Endpoint-only AABBs miss a rotating rod's intermediate occupancy.
    rod=Box(vec((0,0,0)),vec((2,F(1,20),F(1,20))))
    mid=transform(rod,rotation((1,0,0,1))) # 90 degrees about z
    end=transform(rod,rotation((0,0,0,1))) # 180 degrees
    obstacle=Box(vec((0,F(3,2),0)),vec((F(1,10),)*3))
    end_union=(tuple(min(aabb(rod)[0][i],aabb(end)[0][i]) for i in range(3)),tuple(max(aabb(rod)[1][i],aabb(end)[1][i]) for i in range(3)))
    assert not aabb_overlap(end_union,aabb(obstacle));assert overlap(mid,obstacle)
    # Full revolution endpoints coincide; sign-change-only TOI logic misses entry+exit.
    radius=math.sqrt(sum(float(h*h) for h in rod.h))
    dt=1/60;omega=2*math.pi/dt;sphere_r=.1;cy=1.5
    expected_t=math.acos((.05+sphere_r)/cy)/omega
    def gap(t):
        theta=omega*t
        x=cy*math.sin(theta);y=cy*math.cos(theta)
        return math.hypot(max(abs(x)-2,0),max(abs(y)-.05,0))-.1
    assert gap(0)>0 and gap(dt)>0 and gap(dt/4)<0
    # Conservative advancement uses separation/speed bound. Dedicated example,
    # error-tolerance stop; NOT certified arbitrary-rotation CCD implementation.
    t=0.;iters=0;vmax=omega*radius
    while gap(t)>1e-11 and iters<200:
        t+=gap(t)/vmax;iters+=1
    assert iters<200 and abs(t-expected_t)<1e-11
    # Conservative COM-segment plus circumsphere envelope holds for ANY orientation
    # along a linear COM path. Verify samples for random rational rotations.
    contain=0
    for k in range(100):
        p0=np.array([rng.randint(-100,100)/8 for _ in range(3)],float)
        p1=p0+np.array([rng.randint(-10,10)/4 for _ in range(3)],float)
        h=vec([F(rng.randint(1,10),10) for _ in range(3)])
        Rrad=math.sqrt(sum(float(z*z) for z in h));lo=np.minimum(p0,p1)-Rrad;hi=np.maximum(p0,p1)+Rrad
        for n in range(9):
            q=tuple(rng.randint(-10,10) for _ in range(4))
            if not any(q):q=(1,0,0,0)
            c=vec(p0+(p1-p0)*(n/8));shape=Box(c,h,rotation(q))
            for vx in vertices(shape):
                vf=np.array([float(x) for x in vx]);assert np.all(vf>=lo-1e-12) and np.all(vf<=hi+1e-12);contain+=1
    # Child-offset radius matters for compounds.
    child=Box(vec((3,0,0)),vec((F(1,10),)*3))
    max_corner=max(math.sqrt(float(dot(v,v))) for v in vertices(child))
    wrong=math.sqrt(.03);correct=3+wrong
    assert max_corner>wrong and max_corner<=correct
    result={'status':'PASS_REFERENCE_BOUNDS_ONLY_JUDAS_ADAPTER_NOT_RUN','tree_fixture_ops':{'creates':creates,'moves':updates,'destroys':destroys,'queries':queries},'closed_aabb_touch':True,'rotating_rod_endpoint_hull_false_negative_detected':True,'full_rotation_endpoint_only_toi_false_negative_detected':True,'full_rotation_first_toi_analytic':expected_t,'full_rotation_first_toi_CA':t,'CA_iterations':iters,'swept_bound_vertex_checks':contain,'compound_child_offset_control_detected':True,'seconds':time.perf_counter()-t0}
    (ROOT/'results/tree_adapter_fixtures.json').write_text(json.dumps({'scope':'No Judas execution. Compare production query output as conservative superset; fat-proxy extras allowed. For exact supplied proxy AABBs equality may be checked before production fattening. Destroyed logical ids must never reappear.','operations':ops},indent=2))
    (ROOT/'results/broadphase_results.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result,indent=2))
if __name__=='__main__':run()
