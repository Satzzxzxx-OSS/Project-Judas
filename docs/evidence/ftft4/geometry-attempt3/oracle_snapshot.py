#!/usr/bin/env python3
"""FTFT4A represented-input rational oracle; never obtains expected geometry from Judas.

The imported supplied Fraction predicates define closed touching and normalize quaternion
orientations rationally. The probe only emits production outputs. Surface-witness checks
use independent coordinate transforms, with binary64 operation-scale allowances declared
before observing outputs. These are engineering error allowances, not a universal proof.
"""
import argparse, hashlib, json, math, pathlib, random, shutil, struct, subprocess, sys, time
from fractions import Fraction as F
ROOT = pathlib.Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT/'handoff/reference/code'))
import geometry_reference as ref


def f32(x): return struct.unpack('f', struct.pack('f',float(x)))[0]
def v32(v): return [f32(x) for x in v]
def next32(x, direction):
    x=f32(x)
    if x==0: return math.copysign(2**-149,direction)
    bits=struct.unpack('I',struct.pack('f',x))[0]
    bits += 1 if (direction>x)==(x>0) else -1
    return struct.unpack('f',struct.pack('I',bits))[0]
def shape(kind,c=(0,0,0),q=(1,0,0,0),h=(.5,.5,.5),r=.5,kids=()):
    return dict(kind=kind,c=v32(c),q=v32(q),h=v32(h),r=f32(r),kids=[dict(c=v32(k[0]),h=v32(k[1])) for k in kids])
def exact_children(s):
    c=ref.vec(s['c']);R=ref.rotation(s['q'])
    if s['kind']=='sphere': return [ref.Sphere(c,F(s['r']))]
    if s['kind']=='box': return [ref.Box(c,ref.vec(s['h']),R)]
    return [ref.Box(ref.add(c,ref.matvec(R,ref.vec(k['c']))),ref.vec(k['h']),R) for k in s['kids']]
def transform_shape(s,q,t):
    # This intentionally rounds the complete authored inputs before the oracle sees them.
    out=dict(s); R=ref.rotation(q)
    out['c']=v32(ref.add(ref.matvec(R,ref.vec(s['c'])),ref.vec(t)))
    # Fixtures transform identity-orientation primitives only, avoiding a separate quaternion product.
    out['q']=v32(q)
    return out

def generate(out):
    cases=[]
    def add(name,a,b,margin=0,category='primitive',intended=None):
        cases.append(dict(id=name,a=a,b=b,margin=f32(margin),category=category,intended=intended))
    # Explicit known-positive represented gamma witnesses, at retained stored inputs.
    for T in [0,137,1024,32768,1048576]:
        x=next32(next32(T+1,math.inf),math.inf)
        for kind in ['sphere','box']:
            add(f'gamma_{T}_{kind}',shape(kind,(x,0,0)),shape('box',(T,0,0)),.01 if T<10000 else 1,
                'gamma' if T<=1024 else 'representation_limit')
    # Quantization witness: requested millimetre gap erased before geometry, truth is touching.
    add('lost_input_gap',shape('sphere',(1e6+1+.001,0,0)),shape('sphere',(1e6,0,0)),0,
        'representation_limit',dict(requested_gap=.001))
    rotations=[(1,0,0,0),(4,1,2,2)]
    for scale in [.001,1,1000]:
      for T in [0,10,137,1000]:
       for ri,q in enumerate(rotations):
        for si,g in enumerate([-scale/1024,0,scale/1024]):
         for kind in ['ss','sb','bb']:
          a=shape('sphere' if kind!='bb' else 'box',(scale+g,0,0),h=(scale/2,)*3,r=scale/2)
          b=shape('sphere' if kind=='ss' else 'box',h=(scale/2,)*3,r=scale/2)
          aa=transform_shape(a,q,(T,-T,2*T));bb=transform_shape(b,q,(T,-T,2*T))
          for near in [False,True]:
           add(f'{kind}_s{scale}_t{T}_r{ri}_g{si}_m{int(near)}',aa,bb,scale/128 if near else 0)
    # Every signed axis plus face/edge/corner sphere-box feature pairs.
    for axis in range(3):
     for sign in [-1,1]:
      for features in [1,2,3]:
       for g in [-2**-14,0,2**-14]:
        p=[0.,0.,0.]
        for k in range(features): p[(axis+k)%3]=sign*(.5+(.25+g)/math.sqrt(features))
        for ri,q in enumerate(rotations):
         add(f'sb_feature_{axis}_{sign}_{features}_{g}_{ri}',transform_shape(shape('sphere',p,r=.25),q,(137,-137,274)),
             transform_shape(shape('box'),q,(137,-137,274)),.001,'sphere_feature')
    # Deterministic nonparallel SAT configurations, including edge-only separation.
    rng=random.Random(0x4A719251)
    edge_only=0
    for k in range(180):
        qa=[rng.randint(-5,5) for _ in range(4)]; qb=[rng.randint(-5,5) for _ in range(4)]
        if not any(qa):qa[0]=1
        if not any(qb):qb[0]=1
        a=shape('box',q=qa,h=[rng.uniform(.05,1.2) for _ in range(3)])
        b=shape('box',c=[rng.uniform(-1.6,1.6) for _ in range(3)],q=qb,h=[rng.uniform(.05,1.2) for _ in range(3)])
        axes=ref.sat_separations(exact_children(a)[0],exact_children(b)[0])
        edge=not any(s>0 for s,_ in axes[:6]) and any(s>0 for s,_ in axes[6:])
        edge_only+=edge
        add(f'random_sat_{k}',a,b,.015,'edge_only' if edge else 'random_sat')
    for exponent in [10,20,30,40]:
     for offset in [0,.019,.0200001,.1]:
      for T in [0,137,1000]:
       add(f'near_parallel_{exponent}_{offset}_{T}',shape('box',(T,0,0),h=(2,.01,.01)),
           shape('box',(T,offset,0),q=(1,2**-exponent,2**(-exponent-1),0),h=(2,.01,.01)),.002,'near_parallel')
    # A real opening and nonzero child offsets. Parent identity, no convex hull oracle.
    kids=[((-1,0,0),(.2,1,1)),((1,0,0),(.2,1,1)),((0,-1.2,0),(1.2,.2,1))]
    for T in [0,10,137,1000]:
     for ri,q in enumerate(rotations):
      for p,label in [((0,0,0),'opening'),((.9,0,0),'right'),((-.9,0,0),'left'),((0,-1.05,0),'bottom')]:
       add(f'compound_{T}_{ri}_{label}',transform_shape(shape('sphere',p,r=.15),q,(T,-T,2*T)),
           transform_shape(shape('compound',kids=kids),q,(T,-T,2*T)),.001,'compound')
    # Parallel congruent faces have one uniform exact gap, unlike generic tilted
    # manifolds. Child offsets remain local represented inputs across world translations.
    for T in [0,137]:
      for offset,label in [(next32(2.,-math.inf),'negative'),(2.,'touch'),(next32(2.,math.inf),'positive')]:
        q=(.7,.1,.3,.4)
        add(f'rotated_child_face_{T}_{label}',shape('compound',(T,-T,2*T),q=q,kids=[((0,0,0),(1,1,1))]),
            shape('compound',(T,-T,2*T),q=q,kids=[((offset,0,0),(1,1,1))]),.01,'parallel_face')
    # Exact parallel cross axes of dense binary32 quaternion orientations need
    # expansion fallback when interval products cannot resolve the algebraic zero.
    for T in [0,137,1000]:
        q=(.71357,.23711,-.41397,.13931)
        add(f'dense_parallel_fallback_{T}',shape('box',(T,0,0),q=q),
            shape('box',(T+.1,0,0),q=q),.01,'exact_fallback')
    # Quaternion sign/scale changes preserve represented orientation exactly for dyadic inputs.
    for q in [(4,1,2,2),(-4,-1,-2,-2),(16,4,8,8)]:
        add('quaternion_'+'_'.join(map(str,q)),shape('box',(0,0,0),q=q),shape('sphere',(.5,.5,.5),r=.2),.1,'quaternion')
    out.mkdir(parents=True,exist_ok=True)
    (out/'fixtures.json').write_text(json.dumps(cases,indent=2)+'\n')
    with (out/'fixtures.txt').open('w') as f:
      for case in cases:
        pa,pb=exact_children(case['a']),exact_children(case['b'])
        expected=[int(expected_margin(a,b,case['margin'])) for a in pa for b in pb]
        signs=[(ref.witness(a,b)>0)-(ref.witness(a,b)<0) for a in pa for b in pb]
        row=[case['id'],str(case['margin']),str(int(any(ref.overlap(a,b) for a in pa for b in pb))),str(len(expected))]+list(map(str,expected))+list(map(str,signs))
        for s in [case['a'],case['b']]:
          row += [s['kind']]+[format(x,'.17g') for x in s['c']+s['q']+[s['r']]+s['h']]+[str(len(s['kids']))]
          for k in s['kids']:row +=[format(x,'.17g') for x in k['c']+k['h']]
        f.write(' '.join(row)+'\n')
    summary=dict(cases=len(cases),edge_only=edge_only,categories={c:sum(x['category']==c for x in cases) for c in sorted({x['category'] for x in cases})})
    (out/'fixture_summary.json').write_text(json.dumps(summary,indent=2)+'\n'); print(json.dumps(summary))
    return cases


def vecerr(a,b):return math.sqrt(sum(float(F(x)-F(y))**2 for x,y in zip(a,b)))
def sf(s):
    if s['kind']=='sphere':return s['r']
    if s['kind']=='box':return max(s['h'])
    return max([abs(x) for k in s['kids'] for x in k['c']+k['h']]+[1e-300])
def rational_world(s,local):return ref.add(ref.vec(s['c']),ref.matvec(ref.rotation(s['q']),ref.vec(local)))
def surface_error(p,primitive):
    if isinstance(primitive,ref.Sphere):return abs(math.sqrt(float(ref.dot(ref.sub(p,primitive.c),ref.sub(p,primitive.c))))-float(primitive.r))
    local=[ref.dot(a,ref.sub(p,primitive.c)) for a in ref.columns(primitive.R)]
    outside=max([float(abs(x)-h) for x,h in zip(local,primitive.h)]+[0.])
    feature=min(abs(float(abs(x)-h)) for x,h in zip(local,primitive.h))
    return max(outside,feature)
def exact_gap(a,b):
    if isinstance(a,ref.Box) and isinstance(b,ref.Sphere):a,b=b,a
    if isinstance(a,ref.Sphere) and isinstance(b,ref.Sphere):return math.sqrt(float(ref.dot(ref.sub(a.c,b.c),ref.sub(a.c,b.c))))-float(a.r+b.r)
    if isinstance(a,ref.Sphere):
        local=[ref.dot(x,ref.sub(a.c,b.c)) for x in ref.columns(b.R)]
        ext=[max(abs(x)-h,F(0)) for x,h in zip(local,b.h)]
        if any(ext):return math.sqrt(float(ref.dot(ext,ext)))-float(a.r)
        return -min(float(h-abs(x)) for x,h in zip(local,b.h))-float(a.r)
    return max(float(s)/math.sqrt(float(ref.dot(ax,ax))) for s,ax in ref.sat_separations(a,b))
def expected_margin(a,b,margin):
    if margin==0:return ref.overlap(a,b)
    if isinstance(a,ref.Box) and isinstance(b,ref.Box):
        return all(s<=0 or s*s<=F(margin)**2*ref.dot(axis,axis) for s,axis in ref.sat_separations(a,b))
    if isinstance(a,ref.Box):a,b=b,a
    if isinstance(b,ref.Sphere):return ref.dot(ref.sub(a.c,b.c),ref.sub(a.c,b.c))<=(a.r+b.r+F(margin))**2
    local=[ref.dot(x,ref.sub(a.c,b.c)) for x in ref.columns(b.R)];ext=[max(abs(x)-h,F(0)) for x,h in zip(local,b.h)]
    return ref.dot(ext,ext)<=(a.r+F(margin))**2

def check(cases,observations,out):
    failures=[];rows=[];counts=dict(cases=0,primitive_pairs=0,contacts=0,exact_touching=0,positive_gap_contacts=0,bounds=0,world_pairs=0,query_candidates=0)
    maxima=dict(surface_error=0.,anchor_midpoint_error=0.,anchor_witness_gap_error=0.,normal_length_error=0.,gap_error=0.)
    def chk(ok,case,label,detail=None):
        if not ok: failures.append(dict(id=case,label=label,detail=detail))
    for c,o in zip(cases,observations):
      cid=c['id'];chk(cid==o['id'],cid,'case identity')
      for source,stored in zip([c['a'],c['b']],o['inputs']):
        chk(source['c']==stored['c'] and source['q']==stored['q'],cid,'actual stored pose equals oracle binary32 inputs')
        if source['kind']=='sphere':chk(source['r']==stored['r'],cid,'actual stored sphere radius')
        elif source['kind']=='box':chk(source['h']==stored['h'],cid,'actual stored box extents')
        else:chk(source['kids']==stored['children'],cid,'actual stored compound child offsets/extents')
      pa=exact_children(c['a']);pb=exact_children(c['b']);scale=max(sf(c['a']),sf(c['b']))
      # Deliberate fixed engineering allowance: local binary64 geometry, independently derived features.
      tol=4096*sys.float_info.epsilon*max(scale,max(abs(float(v)) for a in pa for b in pb for v in ref.sub(a.c,b.c)))
      truth=any(ref.overlap(a,b) for a in pa for b in pb)
      counts['cases']+=1;counts['world_pairs']+=1;counts['query_candidates']+=2
      chk(o['world_overlap']==truth,cid,'PhysicsWorld independently expected pair',dict(expected=truth,actual=o['world_overlap']))
      if truth:chk(all(o['query_candidates']),cid,'PhysicsWorld broadphase conservative query superset')
      for si,s in enumerate([c['a'],c['b']]):
        prims=[pa,pb][si];bounds=o['bounds'][si]
        lo=tuple(min(ref.aabb(p)[0][k] for p in prims) for k in range(3));hi=tuple(max(ref.aabb(p)[1][k] for p in prims) for k in range(3))
        for k in range(3):chk(F(bounds[0][k])<=lo[k] and F(bounds[1][k])>=hi[k],cid,'outward AABB',dict(shape=si,axis=k,exact=[str(lo[k]),str(hi[k])],observed=[bounds[0][k],bounds[1][k]]))
        counts['bounds']+=1
      contacts=0
      for po in o['pairs']:
        a=pa[po['a']];b=pb[po['b']];expected=expected_margin(a,b,c['margin']);got=bool(po['contacts']);counts['primitive_pairs']+=1
        witness=ref.witness(a,b);counts['exact_touching']+=witness==0
        chk(not po['uncertain'],cid,'resolved supported input',dict(pair=[po['a'],po['b']]))
        chk(expected==got,cid,'closed primitive margin classification',dict(expected=expected,actual=got,witness=str(witness),pair=[po['a'],po['b']]))
        for p in po['contacts']:
          counts['contacts']+=1;contacts+=1
          chk(p['has_anchors'],cid,'precise anchors present')
          chk(all(math.isfinite(x) for key in ['normal','anchor_a','anchor_b','witness_a','witness_b'] for x in p[key]) and math.isfinite(p['gap']),cid,'finite contact')
          # Geometry state is whole primitive truth; positive gap cannot silently become touching.
          if witness>0:
            counts['positive_gap_contacts']+=1;chk(p['gap']>0 and p['state']==0,cid,'known positive preserved',p['gap'])
          if c['category']=='parallel_face':
              chk((p['gap']>0)-(p['gap']<0)==(witness>0)-(witness<0),cid,'parallel-face uniform exact gap sign',dict(exact_sign=(witness>0)-(witness<0),observed=p['gap']))
          # Point separation has its own sign: an overlapping/touching box pair may
          # contain separated clipped points within its candidate margin. Never
          # substitute the whole-pair SAT sign for every manifold point's sign.
          chk(p['state']==(0 if p['gap']>0 else 2 if p['gap']<0 else 1),cid,'per-point separation state matches sign',p['gap'])
          if witness==0 and (isinstance(a,ref.Sphere) or isinstance(b,ref.Sphere)):
              chk(p['state']==1 and p['gap']==0,cid,'exact sphere touching classification',p['gap'])
          n=p['normal'];ne=abs(math.sqrt(sum(x*x for x in n))-1);maxima['normal_length_error']=max(maxima['normal_length_error'],ne);chk(ne<=4096*sys.float_info.epsilon,cid,'unit precise normal',ne)
          wa=rational_world(c['a'],p['witness_a']);wb=rational_world(c['b'],p['witness_b'])
          aa=rational_world(c['a'],p['anchor_a']);ab=rational_world(c['b'],p['anchor_b'])
          se=max(surface_error(wa,a),surface_error(wb,b));maxima['surface_error']=max(maxima['surface_error'],se);chk(se<=tol,cid,'surface witnesses on actual features',dict(error=se,tol=tol))
          mid=ref.mul(ref.add(wa,wb),F(1,2));ae=max(vecerr(aa,mid),vecerr(ab,mid));maxima['anchor_midpoint_error']=max(maxima['anchor_midpoint_error'],ae);chk(ae<=tol,cid,'same physical midpoint parent anchors',dict(error=ae,tol=tol))
          wg=float(ref.dot(ref.sub(wa,wb),ref.vec(n)));ge=abs(wg-p['gap']);maxima['anchor_witness_gap_error']=max(maxima['anchor_witness_gap_error'],ge);chk(ge<=tol,cid,'witness gap along normal',dict(error=ge,tol=tol))
          # Sphere normals are geometrically unique outside the box / noncoincident pair.
          if isinstance(a,ref.Sphere) and isinstance(b,ref.Sphere) and a.c!=b.c:
              d=ref.sub(a.c,b.c); norm=math.sqrt(float(ref.dot(d,d)));chk(vecerr(n,[float(v)/norm for v in d])<=4096*sys.float_info.epsilon,cid,'sphere normal B toward A')
          if isinstance(a,ref.Sphere)!=isinstance(b,ref.Sphere):
              sphere,box=(a,b) if isinstance(a,ref.Sphere) else (b,a)
              local=[ref.dot(u,ref.sub(sphere.c,box.c)) for u in ref.columns(box.R)]
              closest=ref.add(box.c,ref.matvec(box.R,tuple(max(-h,min(h,x)) for x,h in zip(local,box.h))))
              delta=ref.sub(sphere.c,closest);distance=math.sqrt(float(ref.dot(delta,delta)))
              if distance>0:
                  sign=1 if isinstance(a,ref.Sphere) else -1
                  chk(vecerr(n,[sign*float(v)/distance for v in delta])<=4096*sys.float_info.epsilon,cid,'sphere-box normal from exact closest feature')
          # A returned box normal may prefer a stable face among contacts, but must be a SAT axis.
          if isinstance(a,ref.Box) and isinstance(b,ref.Box):
              axes=ref.sat_separations(a,b);alignment=max(abs(sum(float(ax[k])*n[k] for k in range(3)))/math.sqrt(float(ref.dot(ax,ax))) for _,ax in axes)
              chk(abs(alignment-1)<=4096*sys.float_info.epsilon,cid,'box normal aligns actual SAT feature')
              chk(float(ref.dot(ref.sub(a.c,b.c),ref.vec(n)))>=-tol,cid,'box normal B toward A')
          else:
              gap=exact_gap(a,b);err=abs(gap-p['gap']);maxima['gap_error']=max(maxima['gap_error'],err);chk(err<=tol,cid,'sphere exact signed gap',dict(expected=gap,actual=p['gap'],error=err,tol=tol))
      rows.append(dict(id=cid,category=c['category'],actual_overlap=truth,contacts=contacts,tolerance=tol))
    chk(len(cases)==len(observations),'all','row count',dict(expected=len(cases),actual=len(observations)))
    result=dict(status='PASS' if not failures else 'FAIL',counts=counts,maxima=maxima,failures=failures,diagnostics=observations[-1].get('diagnostics',{}))
    out.mkdir(parents=True,exist_ok=True);(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');(out/'case_summary.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='failures'},indent=2)); print('failures',len(failures))
    return not failures


def main():
    ap=argparse.ArgumentParser();ap.add_argument('--generate',action='store_true');ap.add_argument('--binary');ap.add_argument('--output',type=pathlib.Path,default=ROOT/'geometry');args=ap.parse_args()
    fixtures=ROOT/'geometry';cases=generate(fixtures) if args.generate else json.loads((fixtures/'fixtures.json').read_text())
    if not args.binary:return 0
    args.output.mkdir(parents=True,exist_ok=True)
    repository=ROOT.parents[2]
    tracked_sources=[repository/'tests/ContactGeometryProbe.cpp',pathlib.Path(__file__).resolve(),fixtures/'fixtures.txt',fixtures/'fixtures.json']
    tracked_sources += [repository/'src'/name for name in ['Contacts.cpp','Contacts.h','ContactGeometryInternal.h','Narrowphase.cpp','Narrowphase.h','PhysicsWorld.cpp','ContactSolver.cpp','ContactSolver.h','RigidBody.cpp','RigidBody.h','Broadphase.cpp','Broadphase.h']]
    fingerprint=lambda:{str(path.relative_to(repository)):hashlib.sha256(path.read_bytes()).hexdigest() for path in tracked_sources}
    before=fingerprint();binary_hash=hashlib.sha256(pathlib.Path(args.binary).read_bytes()).hexdigest()
    shutil.copyfile(__file__,args.output/'oracle_snapshot.py');shutil.copyfile(repository/'tests/ContactGeometryProbe.cpp',args.output/'probe_snapshot.cpp')
    shutil.copyfile(fixtures/'fixtures.json',args.output/'input_fixtures.json');shutil.copyfile(fixtures/'fixtures.txt',args.output/'input_fixtures.txt')
    start=time.perf_counter();p=subprocess.run([args.binary,str(fixtures/'fixtures.txt')],text=True,capture_output=True)
    (args.output/'fingerprints.json').write_text(json.dumps(dict(before=before,after=fingerprint(),binary_sha256=binary_hash),indent=2)+'\n')
    (args.output/'probe.jsonl').write_text(p.stdout);(args.output/'probe.stderr').write_text(p.stderr)
    (args.output/'run.json').write_text(json.dumps(dict(command=[args.binary,str(fixtures/'fixtures.txt')],returncode=p.returncode,seconds=time.perf_counter()-start),indent=2)+'\n')
    if p.returncode:print(p.stderr)
    if p.returncode not in (0,1):return p.returncode
    observations=[json.loads(x) for x in p.stdout.splitlines() if x.strip()]
    return 0 if check(cases,observations,args.output) and p.returncode==0 else 1
if __name__=='__main__':sys.exit(main())
