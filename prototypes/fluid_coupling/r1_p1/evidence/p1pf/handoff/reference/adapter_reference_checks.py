#!/usr/bin/env python3
"""Independent periodic transport->projection adapter check, not Judas source.
Uses unchanged fixed-grid functions extracted from the supplied P1-C-M reference;
its unrelated Shapely moving-geometry imports/functions are never executed.
Executes 6 cases x 12 transport steps, then ONE frozen all-fluid projection each.
The prescribed transport advector is never overwritten by projected velocities.
"""
from __future__ import annotations
import ast
import json
import math
import time
from pathlib import Path
from types import SimpleNamespace
import numpy as np
from projection_checks import FrozenMAC

FUNCTIONS = {'clip','area','plic','flow','phase_flux','div','restrict_cells',
             'restrict_faces','momentum_face_flux'}
source = Path(__file__).with_name('compatibility_checks.py').read_text()
tree=ast.parse(source)
selected=[node for node in tree.body if isinstance(node,ast.FunctionDef) and node.name in FUNCTIONS]
assert {n.name for n in selected}==FUNCTIONS
namespace={'np':np,'math':math}
exec(compile(ast.Module(body=selected,type_ignores=[]),'<unchanged-fixed-grid-functions>','exec'), namespace)
flow=namespace['flow']; flux=namespace['phase_flux']; div=namespace['div']
restrict=namespace['restrict_cells']; momentum_flux=namespace['momentum_face_flux']

def main():
    start=time.perf_counter(); n=24; nc=n//2; h=1/n; H=1/nc; V=h*h
    yy,xx=np.meshgrid((np.arange(n)+.5)*h,(np.arange(n)+.5)*h,indexing='ij')
    initial=((xx-.48)**2+(yy-.51)**2 < .24**2).astype(float)
    offsets=[(-1,0),(0,-1)]; rows=[]
    for ratio in (1.,1e3,1e6):
        for kind in ('common_comotion','nonuniform_transport_snapshot'):
            U,Vv,ux,uy=flow(n,'translation' if kind=='common_comotion' else 'nonuniform_divergence_free')
            dt=.4/(np.max(abs(ux))/h+np.max(abs(uy))/h)
            directional={1:dt*div(ux,1)/h,0:dt*div(uy,0)/h}
            C=initial.copy()
            mass=[restrict((1+(ratio-1)*C)*V,*o) for o in offsets]
            P=[mass[0]*U,mass[1]*Vv]
            for step in range(12):
                frozen=(C>.5).astype(float); source_rho=1+(ratio-1)*frozen
                frozen_velocity=[p/restrict((1+(ratio-1)*C)*V,*o) for p,o in zip(P,offsets)]
                branchC=[]; branchP=[]
                for order in ((1,0),(0,1)):
                    ca=C.copy(); pa=[p.copy() for p in P]
                    for axis in order:
                        adv=ux if axis==1 else uy; dd=directional[axis]
                        ql=flux(ca,adv,axis,dt,h); qt=adv*dt*h
                        fm=ratio*ql+(qt-ql)
                        cn=ca-div(ql,axis)/V+frozen*dd
                        for q,o in enumerate(offsets):
                            ma=restrict((1+(ratio-1)*ca)*V,*o)
                            fp=momentum_flux(fm,pa[q]/ma,axis,*o)
                            sm=restrict(source_rho*dd*V,*o)
                            pa[q]=pa[q]-div(fp,axis)+frozen_velocity[q]*sm
                        ca=cn
                    branchC.append(ca);branchP.append(pa)
                C=.5*(branchC[0]+branchC[1])
                P=[.5*(branchP[0][q]+branchP[1][q]) for q in range(2)]
            mass=[restrict((1+(ratio-1)*C)*V,*o) for o in offsets]
            diag=np.concatenate([m.ravel() for m in mass])
            moment=np.concatenate([p.ravel() for p in P]); wstar=moment/diag
            B=np.zeros((nc*nc,2*nc*nc))
            for j in range(nc):
                for i in range(nc):
                    r=j*nc+i
                    B[r,j*nc+i]-=H
                    B[r,j*nc+(i+1)%nc]+=H
                    B[r,nc*nc+j*nc+i]-=H
                    B[r,nc*nc+((j+1)%nc)*nc+i]+=H
            grid=SimpleNamespace(B=B,H=diag,rhs=np.zeros(nc*nc))
            p=FrozenMAC.project(grid,wstar)
            p2=FrozenMAC.project(grid,p.velocity)
            momentum_error=[]
            for q in range(2):
                sl=slice(q*nc*nc,(q+1)*nc*nc)
                err=abs(float(np.sum(diag[sl]*p.velocity[sl])-np.sum(moment[sl])))/max(1.,float(np.sum(abs(moment[sl]))))
                momentum_error.append(err)
            result={'name':kind+'_ratio_'+str(ratio),'transport_steps':12,
                    'pressure_cells':nc*nc,'velocity_dofs':len(diag),
                    'nullity':p.nullity,'constraint_residual':p.constraint_residual,
                    'impulse_residual':p.impulse_residual,'energy_identity_residual':p.energy_identity_residual,
                    'momentum_budget_residual':max(momentum_error),
                    'kinetic_before':p.kinetic_before,'kinetic_after':p.kinetic_after,
                    'idempotence_error':float(np.max(abs(p2.velocity-p.velocity))),
                    'min_fraction':float(C.min()),'max_fraction':float(C.max())}
            if kind=='common_comotion':
                target=np.concatenate([U.ravel(),Vv.ravel()])
                result['common_velocity_error']=float(np.max(abs(p.velocity-target)))
                assert result['common_velocity_error']<1e-8
            for key in ('constraint_residual','impulse_residual','energy_identity_residual','momentum_budget_residual','idempotence_error'):
                assert result[key]<1e-9,(key,result)
            assert p.nullity==1
            assert p.kinetic_after <= p.kinetic_before+1e-9*max(1.,p.kinetic_before)
            assert C.min()>=-256*np.finfo(float).eps and C.max()<=1+256*np.finfo(float).eps
            rows.append(result)
    output={'status':'PASS','scope':'Six independent all-fluid periodic mass-momentum snapshots followed by frozen projection; not Judas C++ or moving-solid integration.',
            'cases':len(rows),'transport_timesteps':sum(r['transport_steps'] for r in rows),
            'seconds':time.perf_counter()-start,'results':rows}
    Path(__file__).with_name('adapter_reference_results.json').write_text(json.dumps(output,indent=2)+'\n')
    print(json.dumps(output,indent=2))
if __name__=='__main__': main()
