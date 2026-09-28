from geometry_reference import *
import json,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def run():
    start=time.perf_counter();rows=[]
    R=rotation((4,1,2,2));T=vec((137,-64,32));n=0;toi_checks=0;lbchecks=0
    for gap in [F(0),F(1,2**24),F(1,1000),F(1,4)]:
      for off in [(gap,0,0),(gap,gap,0),(gap,gap,gap)]:
        a=Box(vec((0,0,0)),vec((F(1,2),)*3))
        b=Box(vec(tuple(1+o if o else 0 for o in off)),a.h)
        # gap zero makes both boxes coincide, nonzero witnesses have face/edge/corner.
        expected=sum((z*z for z in off),F(0))
        got=box_distance_sq(a,b);assert got==expected;n+=1
        ar=transform(a,R,T);br=transform(b,R,T)
        got2=box_distance_sq(ar,br);assert got2==expected;n+=1
        rows.append({'gap':str(gap),'offset':list(map(str,off)),'exact_distance_squared':str(got)})
    # Different orientations: exact feature distance vs SAT projected lower bound.
    for q,c in [((3,2,-1,2),(2,1,1)),((1,F(1,2**30),0,0),(0,2,1)),((4,1,2,2),(3,-2,1))]:
        a=Box(vec((0,0,0)),vec((1,F(1,3),F(1,5))))
        b=Box(vec(c),vec((F(2,5),F(1,2),F(1,3))),rotation(q))
        D=box_distance_sq(a,b);assert D>0
        for s,axis in sat_separations(a,b):
            if s>0:assert s*s<=D*dot(axis,axis);lbchecks+=1
        n+=1
    # Fixed-orientation translational sweep, exact rational TOI.
    for g in [F(1,1000),F(1,10),F(1)]:
      for speed in [F(1,2),F(1),F(100)]:
        a=Box(vec((0,0,0)),vec((F(1,2),)*3));b=Box(vec((1+g,0,0)),a.h)
        va=vec((speed,0,0));vb=vec((0,0,0));expected=g/speed
        for rot,shift in [(I,vec((0,0,0))),(R,T)]:
            got=translation_box_toi(transform(a,rot,shift),transform(b,rot,shift),matvec(rot,va),matvec(rot,vb),F(3))
            assert got==expected;toi_checks+=1
    # Compound can have a central opening: no convex-hull substitute.
    children=[Box(vec((-1,0,0)),vec((F(1,5),1,1))),Box(vec((1,0,0)),vec((F(1,5),1,1)))]
    mover=Box(vec((0,-3,0)),vec((F(1,5),)*3))
    assert all(translation_box_toi(mover,b,vec((0,1,0)),vec((0,0,0)),F(6)) is None for b in children)
    result={'status':'PASS_EXACT_REFERENCE_ONLY','exact_distance_checks':n,'exact_box_translation_toi_checks':toi_checks,'sat_lower_bound_checks':lbchecks,'compound_opening_sweep':True,'seconds':time.perf_counter()-start}
    (ROOT/'results/distance_results.json').write_text(json.dumps(result,indent=2));(ROOT/'results/exact_distance_witnesses.json').write_text(json.dumps(rows,indent=2))
    print(json.dumps(result,indent=2))
if __name__=='__main__':run()
