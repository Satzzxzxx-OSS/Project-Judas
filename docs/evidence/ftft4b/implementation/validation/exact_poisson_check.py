#!/usr/bin/env python3
"""Exact Fraction cross-check of the selected policy's two-normal specialization.
Inputs are represented engine mass/inertia/velocity/contact data, not solver answers.
No timing scheduler, friction substitute, or proposed replacement model.
"""
from fractions import Fraction as Q
import json,math,sys
from pathlib import Path
BASE=Path(__file__).resolve().parent
raw=[json.loads(l) for l in (BASE/'policy-witness-final.jsonl').read_text().splitlines()]
assembly=next(x for x in raw if 'A' in x);states=[x for x in raw if 'body' in x]
result=next(x for x in raw if 'initial_energy' in x)
f=lambda x:Q.from_float(float(x))
m=list(map(f,assembly['masses']));I=f(assembly['inertia_body'][2][2]);v0=[f(x['before_v'][1]) for x in states];w0=f(states[0]['before_w'][2]);r=[Q(2),Q(-1,4)];e=[Q(1),Q(1,2)]
A=[[1/m[0]+r[i]*r[j]/I+(1/m[i+1] if i==j else 0) for j in range(2)] for i in range(2)]
det=A[0][0]*A[1][1]-A[0][1]*A[1][0];assert det>0 and A[0][0]>0

def solve(order):
 u=[v0[0]+r[i]*w0-v0[i+1] for i in order];a=[[A[i][j] for j in order] for i in order];budget=[Q(0),Q(0)];total=[Q(0),Q(0)];history=[]
 for rnd in range(16):
  expanding=[p>0 for p in budget];compressing=[not expanding[i] and u[i]<0 for i in range(2)]
  if not any(expanding+compressing):break
  p=budget.copy();incoming=u.copy()
  if all(compressing):
   d=a[0][0]*a[1][1]-a[0][1]*a[1][0]
   p=[(-u[0]*a[1][1]+u[1]*a[0][1])/d,(-u[1]*a[0][0]+u[0]*a[1][0])/d]
  else:
   for i in range(2):
    if compressing[i]:p[i]=(-u[i]-a[i][1-i]*p[1-i])/a[i][i]
  assert all(x>=0 for x in p)
  u=[u[i]+sum(a[i][j]*p[j] for j in range(2)) for i in range(2)]
  assert all(u[i]==0 for i in range(2) if compressing[i])
  budget=[p[i]*e[order[i]] if compressing[i] and -incoming[i]>Q(1,2) else Q(0) for i in range(2)]
  for i in range(2):total[order[i]]+=p[i]
  history.append(dict(round=rnd,order=order,compressing=compressing,expanding=expanding,incoming=list(map(str,incoming)),impulse=list(map(str,p)),outgoing=list(map(str,u))))
 else:raise RuntimeError('budget exhausted')
 vv=[v0[0]+sum(total)/m[0],v0[1]-total[0]/m[1],v0[2]-total[1]/m[2]];ww=w0+sum(r[i]*total[i] for i in range(2))/I
 K=lambda v,w:sum(m[i]*v[i]*v[i] for i in range(3))/2+I*w*w/2
 kb=K(v0,w0);ka=K(vv,ww)
 P=lambda v:sum(m[i]*v[i] for i in range(3))
 L=lambda v,w:I*w+sum(r[i]*m[i+1]*v[i+1] for i in range(2))
 assert P(vv)==P(v0) and L(vv,ww)==L(v0,w0)
 assert all(x>=0 for x in u)
 return dict(energy_before=kb,energy_after=ka,gain=ka-kb,velocity=vv,omega=ww,total_impulse=total,history=history)
a=solve([0,1]);b=solve([1,0])
assert a['velocity']==b['velocity'] and a['omega']==b['omega']
errors={k:abs(float(a[k])-result[n]) for k,n in [('energy_before','initial_energy'),('energy_after','final_energy'),('gain','gain')]}
# Declared arithmetic comparison, not an energy allowance or a model adjustment.
assert max(errors.values())<1e-9
assert a['gain']>Q(1,10000)
trace=[x for x in raw if 'round' in x]
assert len(trace)==len(a['history'])==4
for cpp,exact in zip(trace,a['history']):
 assert max(abs(cpp['impulse'][i]-float(Q(exact['impulse'][i]))) for i in range(2))<1e-9
out={'scope':'Exact verification of a contradiction; NOT a physical pass or full PLUS implementation',
 'matrix':[[str(x) for x in row] for row in A],'matrix_rank':2,'determinant':str(det),
 'minimum_norm_unique':True,'active_set_eliminations':0,'friction':0,'restitution':[1,.5],
 'capture_threshold':.5,'all_compression_approaches_above_capture':True,
 'rounds':a['history'],'energy_before':str(a['energy_before']),'energy_after':str(a['energy_after']),
 'gain_exact':str(a['gain']),'gain_J':float(a['gain']),
 'linear_momentum_residual_exact':'0','angular_momentum_residual_exact':'0',
 'contact_order_invariant_exact':True,'cpp_comparison_abs_error':errors,
 'energy_physical_requirement':'FAIL','exact_equation_check':'PASS',
 'remaining_contact_velocities_nonnegative':True,
 'round_scheduling':'Remaining Poisson expansion budget takes precedence over observing; otherwise even isolated restitution would be suppressed at compression end.'}
(BASE/'exact-poisson-results.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({k:out[k] for k in ['gain_J','linear_momentum_residual_exact','angular_momentum_residual_exact','contact_order_invariant_exact','cpp_comparison_abs_error','energy_physical_requirement','exact_equation_check']},indent=2))
sys.exit(1) # Preserve the physical energy-budget failure.
