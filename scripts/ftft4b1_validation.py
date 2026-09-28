#!/usr/bin/env python3
"""FTFT4B-1 production defect measurements. No solver substitute or repair.
Default --output is a fresh replay; --observations-only continues the initial
energy witness already recorded at that output. Requires numpy for exact reuse
of the supplied seeded rotation fixture generation, not for engine answers.
"""
import sys
sys.dont_write_bytecode=True
import argparse,hashlib,json,math,subprocess,time,importlib.util
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'docs/evidence/ftft4b/baseline'
UNITS=['RigidBody','RigidBodyGravity','Contacts','ContactSolver','RadialTerrain','Broadphase','Narrowphase','PhysicsWorld']
TARGETS=['judas_rigid_contact_tests','judas_contact_geometry_tests','judas_contact_lifecycle_tests','judas_broadphase_tests','judas_contact_cache_tests','judas_contact_storage_tests','judas_physics_tests','judas_lifecycle_tests']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,x):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(x,indent=2)+'\n')
def load(p):return json.loads(p.read_text())
def source():return {str(p.relative_to(ROOT)):sha(p) for p in sorted((ROOT/'src').rglob('*')) if p.is_file()}
def run(name,cmd,out,allowed=(0,),timeout=600):
 start=time.monotonic();p=subprocess.run([str(x) for x in cmd],cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout)
 (out/(name+'.log')).write_text(p.stdout);rec={'command':[str(x) for x in cmd],'exit_code':p.returncode,'seconds':time.monotonic()-start,'log':name+'.log'};save(out/(name+'-run.json'),rec)
 print(name,'exit',p.returncode,flush=True)
 if p.returncode not in allowed:raise RuntimeError(name+' unexpected execution failure; see '+rec['log'])
 return rec,p.stdout
def rows(t):return [json.loads(l) for l in t.splitlines() if l.startswith('{')]
def qmul(a,b):
 w,x,y,z=a;v,i,j,k=b
 return np.array([w*v-x*i-y*j-z*k,w*i+x*v+y*k-z*j,w*j-x*k+y*v+z*i,w*k+x*j-y*i+z*v])
def normalized(a):return a/np.linalg.norm(a)
def distance(a,b):return min(np.linalg.norm(a-b),np.linalg.norm(a+b))
def rotation_cases(out):
 # Same RNG, draw order and choices as supplied rotation_checks.py. No candidate integrator run.
 rng=np.random.default_rng(422);cases=[]
 for case in range(240):
  q=normalized(rng.normal(size=4));axis=normalized(rng.normal(size=3));speed=float(rng.choice([.1,1,10,100,500]));w=axis*speed
  h=float(rng.choice([1/240,1/60,.2]));n=int(rng.integers(2,17));parts=rng.random(n);parts*=h/parts.sum()
  cases.append((q,w,h,parts))
 for speed,h,n in [(10.,1/60,2),(100.,1/60,2),(10.,.2,2),(1000.,1/60,8)]:cases.append((np.array([1.,0,0,0]),np.array([0.,0,speed]),h,np.full(n,h/n)))
 text=''
 for i,(q,w,h,parts) in enumerate(cases):
  # Independently stored binary32 inputs, same represented values consumed by C++.
  q=np.asarray(q,dtype=np.float32);w=np.asarray(w,dtype=np.float32);h=np.float32(h);parts=np.asarray(parts,dtype=np.float32)
  text+=' '.join([str(i),*[format(float(x),'.17g') for x in q],*[format(float(x),'.17g') for x in w],format(float(h),'.17g'),str(len(parts)),*[format(float(x),'.17g') for x in parts]])+'\n'
 (out/'angular-input.txt').write_text(text)
def angular_analysis(data):
 result=[]
 for r in data:
  q=normalized(np.array(r['q0']));omega=np.array(r['omega']);speed=float(np.linalg.norm(omega));axis=omega/speed
  one_angle=2*math.atan(speed*r['h']/2);split_angle=sum(2*math.atan(speed*t/2) for t in r['parts'])
  def expected(angle):return normalized(qmul(np.r_[math.cos(angle/2),axis*math.sin(angle/2)],q))
  a=np.array(r['single']);b=np.array(r['split']);n=len(r['parts']);tol=64*np.finfo(np.float32).eps*(1+n)
  e1=distance(a,expected(one_angle));e2=distance(b,expected(split_angle));delta=distance(a,b)
  result.append(dict(case=r['case'],segments=n,whole_interval=r['h'],partition_interval=sum(r['parts']),one_analytic_angle=one_angle,split_analytic_angle=split_angle,actual_quaternion_partition_distance=float(delta),one_formula_error=float(e1),split_formula_error=float(e2),formula_tolerance=float(tol),formula_pass=bool(e1<=tol and e2<=tol),partition_changes_endpoint_beyond_roundoff=bool(delta>2*tol),omega_unchanged=r['omega_after_single']==r['omega_after_split']==r['omega']))
 return {'cases':len(result),'position_integrator_calls':sum(1+r['segments'] for r in result),'rows':result,'formula_checks_pass':all(r['formula_pass'] and r['omega_unchanged'] for r in result),'scope':'Current actual IntegrateRigidBodyPosition on copied states; no contact/event loop or torque-free-asymmetric-dynamics claim.'}
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=ROOT/'docs/evidence/ftft4b/replay');parser.add_argument('--observations-only',action='store_true');args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
 d=out/'execution';d.mkdir(exist_ok=False);before=source();commands=[]
 if not args.observations_only:
  binary=ROOT/'build/ftft4b/research-energy';binary.parent.mkdir(parents=True,exist_ok=True)
  cmd=['/usr/bin/c++','-std=c++17','-O3','-DNDEBUG','-ffp-contract=off','-Isrc',BASE/'research/code/production_energy_witness.cpp',*['src/'+u+'.cpp' for u in UNITS],'-o',binary]
  c,_=run('compile-energy',cmd,out);commands.append(c);c,_=run('original-energy',[binary],out,allowed=(0,1));commands.append(c)
 else:
  assert (out/'original-energy-run.json').exists()
 c,_=run('configure',['cmake','-S','.','-B','build','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_CXX_FLAGS=-ffp-contract=off'],d);commands.append(c)
 c,_=run('build',['cmake','--build','build','--target','judas_timing_defect_probe',*TARGETS,'-j4'],d);commands.append(c)
 c,t=run('energy-variants',[ROOT/'build/judas_timing_defect_probe','energy'],d,allowed=(0,1));commands.append(c);energy=rows(t);assert len(energy)==36
 save(d/'energy.json',{'rows':energy,'energy_failures':sum(x['energy_failure'] for x in energy),'momentum_failures':sum(x['momentum_failure'] for x in energy),'scope':'36 actual world steps; nominal mass and sphere inertia; all arithmetic promoted before budget dot products. Original unchanged witness separately retains its float-dot output.'})
 c,t=run('extended-temporal',[ROOT/'build/judas_timing_defect_probe','extended'],d);commands.append(c);extended=rows(t);assert len(extended)==240
 for x in extended:
  s,q,h,e=x['initial_gap'],1.,x['dt'],x['e'];elapsed=x['step']*h;tau=s/q
  expected=s-q*elapsed if elapsed<tau else e*q*(elapsed-tau)
  expectedv=-q if elapsed<tau else e*q
  x.update(expected_gap=expected,expected_velocity=expectedv,position_error=x['gap']-expected,position_requirement_pass=abs(x['gap']-expected)<=2e-6,velocity_requirement_pass=abs(x['velocity'][1]-expectedv)<=2e-6)
 save(d/'extended-temporal.json',{'rows':extended,'position_failures':sum(not x['position_requirement_pass'] for x in extended),'velocity_failures':sum(not x['velocity_requirement_pass'] for x in extended),'oracle':'s/q event time; post-impact drift over elapsed-tau; unchanged 2e-6 m allowance from supplied temporal checker'})
 # Compile and run the unchanged actual-engine temporal adapter and checker.
 probe=BASE/'adapters/world_probe.cpp';binary=ROOT/'build/ftft4b/temporal'
 cmd=['/usr/bin/c++','-std=c++17','-O3','-DNDEBUG','-ffp-contract=off','-Isrc',probe,*['src/'+u+'.cpp' for u in UNITS],'-o',binary]
 c,_=run('compile-temporal',cmd,d);commands.append(c);c,t=run('temporal',[binary],d);commands.append(c);(d/'temporal.csv').write_text(t)
 for mode in ['geometry','full']:
  c,_=run('temporal-'+mode,[sys.executable,BASE/'adapters/check_world_trace.py',d/'temporal.csv','--mode',mode],d,allowed=(0,1));commands.append(c)
 rotation_cases(d);c,t=run('angular',[ROOT/'build/judas_timing_defect_probe','angular',d/'angular-input.txt'],d);commands.append(c);ang=angular_analysis(rows(t));save(d/'angular.json',ang);assert len(ang['rows'])==244 and ang['formula_checks_pass']
 regression={}
 for target in TARGETS:
  c,_=run(target,[ROOT/'build'/target],d);commands.append(c);regression[target]=c['exit_code']==0
 for flag in ['--player','--invalid']:
  c,_=run('geometry-'+flag[2:],[ROOT/'build/judas_contact_geometry_tests',flag],d);commands.append(c)
 c,_=run('independent-oracle',[sys.executable,ROOT/'docs/evidence/ftft4/geometry_oracle.py','--binary',ROOT/'build/judas_contact_geometry_tests','--output',d/'geometry-oracle'],d);commands.append(c)
 assert before==source(),'Production changed during verification'
 result={'scope':'FTFT4B-1 measurement complete; physical failures preserved; NOT FTFT4B acceptance','verification_complete':True,'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'commands':commands,'regressions':regression,'energy_cases':36,'energy_failures':sum(x['energy_failure'] for x in energy),'momentum_failures':sum(x['momentum_failure'] for x in energy),'angular_cases':244,'angular_formula_checks_pass':ang['formula_checks_pass'],'angular_partition_failures_beyond_roundoff':sum(x['partition_changes_endpoint_beyond_roundoff'] for x in ang['rows']),'production_sha256':before,'probe_sha256':sha(ROOT/'tests/TimingDefectProbe.cpp'),'runner_sha256':sha(__file__),'numpy':np.__version__,'no_production_source_changes':True,'new_hooks':'none; existing public measurements suffice','not_run':['research experimental multi-contact algorithms','new rotational TOI implementation','full application/editor/async suite','fluid prototypes','performance campaign','human visual validation']}
 save(d/'results.json',result);print(json.dumps({k:result[k] for k in ['verification_complete','energy_cases','energy_failures','momentum_failures','angular_partition_failures_beyond_roundoff']}))
if __name__=='__main__':main()
