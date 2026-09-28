#!/usr/bin/env python3
"""Constant-world-angular-velocity drift and event partition research.
Current-source normalized Euler expression vs explicit exponential alternative.
The alternative is NOT a production change or general torque-free asymmetric-body integrator.
"""
import numpy as np, math,json,random,time
from pathlib import Path

def mul(a,b):
 s,u=a[0],a[1:];t,v=b[0],b[1:]
 return np.r_[s*t-u@v,s*v+t*u+np.cross(u,v)]
def norm(q):return q/np.linalg.norm(q)
def euler(q,w,t):return norm(q+0.5*t*mul(np.r_[0.,w],q))
def exponential(q,w,t):
 a=np.linalg.norm(w);half=a*t/2
 if a==0:return q.copy()
 return norm(mul(np.r_[math.cos(half),w*(math.sin(half)/a)],q))
def difference(a,b):return min(np.linalg.norm(a-b),np.linalg.norm(a+b))
def main():
 start=time.perf_counter();rng=np.random.default_rng(422);records=[];max_euler=0;max_exp=0;max_anchor=0;checks=0
 for case in range(240):
  q=norm(rng.normal(size=4));axis=norm(rng.normal(size=3));speed=float(rng.choice([.1,1,10,100,500]));w=axis*speed
  h=float(rng.choice([1/240,1/60,.2]));n=int(rng.integers(2,17));parts=rng.random(n);parts*=h/parts.sum()
  qe=q.copy();qx=q.copy()
  for dt in parts:qe=euler(qe,w,dt);qx=exponential(qx,w,dt)
  de=difference(qe,euler(q,w,h));dx=difference(qx,exponential(q,w,h))
  # Anchor-preserving legacy evaluation at final elapsed time, not repeated advancing.
  qa=euler(q,w,float(sum(parts)));da=difference(qa,euler(q,w,h))
  max_euler=max(max_euler,de);max_exp=max(max_exp,dx);max_anchor=max(max_anchor,da)
  assert dx<1e-12 and da<1e-12;checks+=2
 for speed,h,n in [(10.,1/60,2),(100.,1/60,2),(10.,.2,2),(1000.,1/60,8)]:
  old=2*math.atan(speed*h/2);split=2*n*math.atan(speed*h/(2*n));exact=speed*h
  deriv=speed/(1+(speed*h/2)**2)
  records.append(dict(speed=speed,h=h,segments=n,legacy_angle=old,repeated_legacy_angle=split,
     constant_omega_angle=exact,splitting_change_radians=split-old,
     source_stored_omega=speed,legacy_path_endpoint_angular_derivative=deriv))
 # Full angular path can move though end pose equals its start.
 q=np.array([1.,0,0,0]);w=np.array([0,0,2*math.pi]);end=exponential(q,w,1.);mid=exponential(q,w,.25)
 assert difference(end,q)<1e-14 and difference(mid,q)>.5;checks+=1
 out=dict(status='PASS_REFERENCE_WITH_DETECTED_PARTITION_DEFECT',cases=240,assertions=checks,
    max_quaternion_difference_repeated_euler=max_euler,max_exponential_partition_error=max_exp,
    max_anchored_legacy_partition_error=max_anchor,witnesses=records,
    scope='constant angular velocity kinematics only; no claim that new exponential drift is in Judas',runtime_seconds=time.perf_counter()-start)
 p=Path(__file__).resolve().parents[1]/'results/rotation_results.json';p.write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
if __name__=='__main__':main()
