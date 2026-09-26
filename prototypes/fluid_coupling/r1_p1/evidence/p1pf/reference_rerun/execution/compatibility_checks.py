#!/usr/bin/env python3
"""Independent research checks; NOT Judas source or P1-D/E validation.
Requires numpy, shapely. Runs a small independently written geometric WY/JSGT
transport and a mass/momentum ledger on staggered dual volumes. Separately
checks exact prescribed rigid co-motion remapping of polygonal cut cells.
No pressure solve, fluid/body dynamics, or production performance claims.
"""
from __future__ import annotations
import json, math, platform, time
from pathlib import Path
import numpy as np
from shapely.geometry import Polygon, box
from shapely.affinity import translate, rotate


def clip(poly, nx, ny, bound):
    if not poly: return []
    out=[]
    a=poly[-1]; da=nx*a[0]+ny*a[1]-bound
    for b in poly:
        db=nx*b[0]+ny*b[1]-bound
        if (da<=0) != (db<=0):
            t=da/(da-db)
            out.append((a[0]+t*(b[0]-a[0]),a[1]+t*(b[1]-a[1])))
        if db<=0: out.append(b)
        a=b; da=db
    return out


def area(poly):
    if len(poly)<3: return 0.
    return abs(math.fsum(poly[k][0]*poly[(k+1)%len(poly)][1]
                         -poly[k][1]*poly[(k+1)%len(poly)][0]
                         for k in range(len(poly))))*.5


def plic(c):
    tol=256*np.finfo(float).eps
    if not np.all(np.isfinite(c)) or float(c.min()) < -tol or float(c.max()) > 1+tol:
        raise ValueError("Materially invalid PLIC input fraction")
    gy=.5*(np.roll(c,-1,axis=0)-np.roll(c,1,axis=0))
    gx=.5*(np.roll(c,-1,axis=1)-np.roll(c,1,axis=1))
    polys=[]
    square=[(0.,0.),(1.,0.),(1.,1.),(0.,1.)]
    for j in range(c.shape[0]):
        row=[]
        for i in range(c.shape[1]):
            C=float(c[j,i])
            # Roundoff endpoints are interpreted geometrically, never written back.
            if C<=0: row.append([]); continue
            if C>=1: row.append(square); continue
            nx=-float(gx[j,i]);ny=-float(gy[j,i]); n=abs(nx)+abs(ny)
            if n==0: nx,ny,n=1.,0.,1.
            nx/=n;ny/=n
            a,b=sorted([abs(nx),abs(ny)])
            F=min(C,1-C)
            if a==0: t=b*F
            elif F<=a/(2*b): t=math.sqrt(2*a*b*F)
            else: t=b*F+a/2
            if C>.5: t=a+b-t
            bound=t+min(0.,nx)+min(0.,ny)
            row.append(clip(square,nx,ny,bound))
        polys.append(row)
    return polys


def flow(n,kind):
    # MAC grid has n/2 cells per direction. Periodic face indexing.
    nc=n//2; H=1/nc
    if kind=='translation':
        U=np.full((nc,nc),.37);V=np.full((nc,nc),-.21)
    else:
        y,x=np.meshgrid(np.arange(nc)*H,np.arange(nc)*H,indexing='ij')
        psi=.07*np.sin(2*np.pi*x)*np.sin(2*np.pi*y)
        U=(np.roll(psi,-1,axis=0)-psi)/H
        V=-(np.roll(psi,-1,axis=1)-psi)/H
    # Boundary fine normal velocity copied; middle fine face averaged.
    u=np.empty((n,n));v=np.empty((n,n))
    for J in range(nc):
        for I in range(nc):
            for j in [2*J,2*J+1]:
                u[j,2*I]=U[J,I]
                u[j,2*I+1]=.5*(U[J,I]+U[J,(I+1)%nc])
            for i in [2*I,2*I+1]:
                v[2*J,i]=V[J,I]
                v[2*J+1,i]=.5*(V[J,I]+V[(J+1)%nc,I])
    return U,V,u,v


def phase_flux(c, vel, axis, dt, h):
    n=len(c); ps=plic(c); out=np.zeros_like(c)
    for j in range(n):
        for i in range(n):
            speed=float(vel[j,i]); width=abs(speed)*dt/h
            assert width<1.
            if axis==1:
                donor=(i-1)%n if speed>=0 else i
                p=ps[j][donor]
                cut=clip(p,-1.,0.,-(1-width)) if speed>=0 else clip(p,1.,0.,width)
            else:
                donor=(j-1)%n if speed>=0 else j
                p=ps[donor][i]
                cut=clip(p,0.,-1.,-(1-width)) if speed>=0 else clip(p,0.,1.,width)
            out[j,i]=math.copysign(area(cut)*h*h,speed)
    return out


def div(f,axis): return np.roll(f,-1,axis=axis)-f


def restrict_cells(a,ox,oy):
    n=len(a); I=(2*np.arange(n//2)+ox)%n;J=(2*np.arange(n//2)+oy)%n
    return sum(a[np.ix_((J+dj)%n,(I+di)%n)] for dj in [0,1] for di in [0,1])


def restrict_faces(f,axis,ox,oy):
    n=len(f);I=(2*np.arange(n//2)+ox)%n;J=(2*np.arange(n//2)+oy)%n
    if axis==1:
        return f[np.ix_(J,I)]+f[np.ix_((J+1)%n,I)]
    return f[np.ix_(J,I)]+f[np.ix_(J,(I+1)%n)]


def momentum_face_flux(fine_mass_flux, stage_velocity, axis, ox, oy):
    """Upwind each fine face BEFORE summing; segments may counterflow."""
    n=len(fine_mass_flux)
    I=(2*np.arange(n//2)+ox)%n; J=(2*np.arange(n//2)+oy)%n
    if axis==1:
        segments=[fine_mass_flux[np.ix_(J,I)],
                  fine_mass_flux[np.ix_((J+1)%n,I)]]
    else:
        segments=[fine_mass_flux[np.ix_(J,I)],
                  fine_mass_flux[np.ix_(J,(I+1)%n)]]
    upstream_left=np.roll(stage_velocity,1,axis=axis)
    return sum(fm*np.where(fm>=0,upstream_left,stage_velocity) for fm in segments)


def counterflow_check():
    # Equal/opposite mass transfers must not erase momentum exchange.
    fine=np.zeros((4,4)); fine[0,0]=.25; fine[1,0]=-.25
    w=np.array([[-1.,2.],[-1.,2.]])
    correct=momentum_face_flux(fine,w,1,0,0)[0,0]
    expected=.25*2.+(-.25)*(-1.)
    wrong=0.0 # net mass transfer is zero
    assert abs(correct-expected)<1e-15
    assert abs(wrong-expected)>.1
    return dict(segment_mass_transfers=[.25,-.25],donor_velocities=[2.,-1.],
                net_mass_transfer=0.,momentum_transfer=float(correct),
                analytic_momentum_transfer=expected,net_first_wrong_momentum=wrong)


def momentum_experiments():
    rng=np.random.default_rng(32031); n=24;h=1/n;Vcell=h*h
    yy,xx=np.meshgrid((np.arange(n)+.5)*h,(np.arange(n)+.5)*h,indexing='ij')
    # Mixed smooth fractions plus sharp pure phases; not an accuracy oracle.
    c0=np.where((xx-.48)**2+(yy-.51)**2<.24**2,1.,0.)
    c0[(xx>.30)&(xx<.65)&(yy>.32)&(yy<.67)]=rng.random(np.count_nonzero((xx>.30)&(xx<.65)&(yy>.32)&(yy<.67)))
    rows=[]
    for kind in ['translation','nonuniform_divergence_free']:
        U,V,u,v=flow(n,kind)
        dt=.40/(np.max(abs(u))/h+np.max(abs(v))/h)
        d=(dt*div(u,1)/h,dt*div(v,0)/h)
        for ratio in [1.,1000.,1e6]:
            for mode in ['constant','smooth','random']:
                constant=(mode=='constant')
                c=c0.copy(); rho=1.+(ratio-1)*c
                offsets=[(-1,0),(0,-1)]
                vals=[np.full_like(U,.73),np.full_like(V,-.42)] if constant else ([U.copy(),V.copy()] if mode=='smooth' else [rng.uniform(-1,1,U.shape),rng.uniform(-1,1,V.shape)])
                P=[restrict_cells(rho*Vcell,*off)*val for off,val in zip(offsets,vals)]
                Pinit=[float(p.sum()) for p in P]
                max_mass_defect=max_uniform_error=max_total_p_err=max_identity=max_closure=0.
                mismatch_error=0.
                max_energy_increase=0.
                min_vel=[float(val.min()) for val in vals];max_vel=[float(val.max()) for val in vals];max_overshoot=0.
                cmin=float(c.min());cmax=float(c.max())
                mass_total0=float((rho*Vcell).sum())
                for step in range(12):
                    frozen=(c>.5).astype(float)
                    source_rho=1.+(ratio-1)*frozen
                    M0=[restrict_cells((1+(ratio-1)*c)*Vcell,*off) for off in offsets]
                    source_velocity=[p/m for p,m in zip(P,M0)]
                    Eold=sum(float(np.sum(.5*p*p/m)) for p,m in zip(P,M0))
                    branchC=[];branchP=[];branchM=[];branchFlux=[]
                    for order in [(1,0),(0,1)]:
                        C=c.copy(); PB=[p.copy() for p in P];fluxes={}
                        for axis in order:
                            vel=u if axis==1 else v
                            dd=d[0] if axis==1 else d[1]
                            FL=phase_flux(C,vel,axis,dt,h)
                            QT=vel*dt*h
                            FM=ratio*FL+(QT-FL)
                            Cnew=C-div(FL,axis)/Vcell+frozen*dd
                            S=source_rho*dd*Vcell
                            cmin=min(cmin,float(Cnew.min()));cmax=max(cmax,float(Cnew.max()))
                            for q,off in enumerate(offsets):
                                M=restrict_cells((1+(ratio-1)*C)*Vcell,*off)
                                Mn=restrict_cells((1+(ratio-1)*Cnew)*Vcell,*off)
                                face=restrict_faces(FM,axis,*off)
                                SQ=restrict_cells(S,*off)
                                ledger=M-div(face,axis)+SQ
                                max_mass_defect=max(max_mass_defect,float(np.max(abs(Mn-ledger)))/max(1.,float(np.max(Mn))))
                                velq=PB[q]/M
                                FP=momentum_face_flux(FM,velq,axis,*off)
                                PB[q]=PB[q]-div(FP,axis)+source_velocity[q]*SQ
                            fluxes[axis]=FL
                            C=Cnew
                        branchC.append(C);branchP.append(PB);branchFlux.append(fluxes)
                        branchM.append([restrict_cells((1+(ratio-1)*C)*Vcell,*off) for off in offsets])
                    Cnew=.5*(branchC[0]+branchC[1])
                    direct=c-.5*(div(branchFlux[0][1]+branchFlux[1][1],1)+div(branchFlux[0][0]+branchFlux[1][0],0))/Vcell
                    max_identity=max(max_identity,float(np.max(abs(Cnew-direct))))
                    P=[.5*(branchP[0][q]+branchP[1][q]) for q in [0,1]]
                    for q,off in enumerate(offsets):
                        M=.5*(branchM[0][q]+branchM[1][q])
                        Mr=restrict_cells((1+(ratio-1)*Cnew)*Vcell,*off)
                        max_closure=max(max_closure,float(np.max(abs(M-Mr)))/max(1.,float(np.max(M))))
                        if constant:
                            target=.73 if q==0 else -.42
                            # Negative control: normalize new momentum by OLD mass.
                            mismatch_error=max(mismatch_error,float(np.max(abs(P[q]/M0[q]-target))))
                            max_uniform_error=max(max_uniform_error,float(np.max(abs(P[q]/M-target))))
                        pnorm=max(1.,float(np.sum(abs(M0[q]*source_velocity[q]))))
                        max_total_p_err=max(max_total_p_err,abs(float(P[q].sum())-Pinit[q])/pnorm)
                    # Full-step energy / bounds checks: diagnostics, not assumed invariants.
                    Mnew=[restrict_cells((1+(ratio-1)*Cnew)*Vcell,*off) for off in offsets]
                    Enew=sum(float(np.sum(.5*p*p/m)) for p,m in zip(P,Mnew))
                    max_energy_increase=max(max_energy_increase,(Enew-Eold)/max(1.,Eold))
                    for q in [0,1]:
                        aq=P[q]/Mnew[q]
                        max_overshoot=max(max_overshoot,float(aq.max())-max_vel[q],min_vel[q]-float(aq.min()))
                    c=Cnew
                mass_final=float(((1+(ratio-1)*c)*Vcell).sum())
                rows.append(dict(flow=kind,density_ratio=ratio,transported_velocity=mode,steps=12,
                    max_full_step_relative_energy_increase=max_energy_increase,
                    max_velocity_bound_overshoot=max_overshoot,
                    max_discrete_divergence=float(np.max(abs(d[0]+d[1]))),cumulative_cfl=.4,
                    min_fraction=cmin,max_fraction=cmax,
                    normalized_dual_mass_ledger_error=max_mass_defect,normalized_mass_restriction_error=max_closure,
                    uniform_velocity_error=max_uniform_error if constant else None,
                    negative_control_old_mass_velocity_error=mismatch_error if constant else None,
                    normalized_total_momentum_error=max_total_p_err,
                    normalized_total_mass_error=abs(mass_final-mass_total0)/max(1.,mass_total0),
                    sweep_mean_vs_shared_flux_error=max_identity))
                assert cmin>=-256*np.finfo(float).eps and cmax<=1+256*np.finfo(float).eps,rows[-1]
                assert max_mass_defect<1e-12 and max_identity<1e-12,rows[-1]
                assert max_total_p_err<1e-11,rows[-1]
                assert rows[-1]["normalized_total_mass_error"]<1e-11,rows[-1]
                assert max_closure<1e-12,rows[-1]
                if constant: assert max_uniform_error<1e-8,rows[-1]
    return rows


def grid_partition(domain,h):
    minx,miny,maxx,maxy=domain.bounds; items=[]
    for j in range(math.floor(miny/h),math.ceil(maxy/h)):
        for i in range(math.floor(minx/h),math.ceil(maxx/h)):
            g=domain.intersection(box(i*h,j*h,(i+1)*h,(j+1)*h))
            if not g.is_empty and g.area>0:
                pieces=list(g.geoms) if g.geom_type in ('MultiPolygon','GeometryCollection') else [g]
                items.extend((i,j,p) for p in pieces if p.geom_type=='Polygon' and p.area>0)
    return items


def remap_experiments():
    rows=[];h=.125
    # Wall spans the tank, generating genuinely disconnected fluid regions.
    for delta in [0.031,1e-9]:
        wallx=.5+delta; thickness=.003
        tank=box(0.,0.,1.,1.);wall=box(wallx,0.,wallx+thickness,1.)
        fluid=tank.difference(wall)
        phase=fluid.intersection(Polygon([(-1,-1),(2,-1),(2,.81),(-1,.39)]))
        old=grid_partition(fluid,h)
        for name,T in [('translation',lambda g:translate(g,xoff=.037,yoff=.049)),
                       ('rotation',lambda g:rotate(g,11.5,origin=(.5,.5),use_radians=False))]:
            newdomain=T(fluid);newphase=T(phase);new=grid_partition(newdomain,h)
            oldarea=np.array([p.area for _,_,p in old]);newarea=np.array([p.area for _,_,p in new])
            transfer=np.zeros((len(new),len(old)));liquid=np.zeros_like(transfer)
            for i,(_,_,p) in enumerate(old):
                adv=T(p);advliquid=T(p.intersection(phase))
                for j,(_,_,q) in enumerate(new):
                    if not adv.intersects(q):continue
                    g=adv.intersection(q)
                    transfer[j,i]=g.area
                    liquid[j,i]=advliquid.intersection(q).area
            Lnew=liquid.sum(axis=1)
            Lexact=np.array([p.intersection(newphase).area for _,_,p in new])
            tcol=transfer.sum(axis=0);trow=transfer.sum(axis=1)
            Mji=1000*liquid+(transfer-liquid)
            masses=Mji.sum(axis=1)
            # Manufactured transported uniform vector; not a rotating fluid solution.
            U=np.array([.73,-.42]);newP=masses[:,None]*U
            newU=newP/masses[:,None]
            oldleft=np.array([p.centroid.x<wallx for _,_,p in old])
            mapped_left=T(box(0,0,wallx,1))
            newleft=np.array([p.intersection(mapped_left).area>.5*p.area for _,_,p in new])
            wrong=transfer[newleft[:,None]!=oldleft[None,:]].sum()
            rows.append(dict(transform=name,small_cut_width=delta,old_subcells=len(old),new_subcells=len(new),
                min_old_area=float(oldarea.min()),max_donor_partition_error=float(np.max(abs(tcol-oldarea))),
                max_receiver_capacity_error=float(np.max(abs(trow-newarea))),
                max_liquid_oracle_error=float(np.max(abs(Lnew-Lexact))),
                total_phase_volume_error=abs(float(Lnew.sum())-phase.area),
                analytic_total_liquid_area=.6-thickness*(.53+.14*wallx+.07*thickness),
                analytic_total_liquid_error=abs(float(Lnew.sum())-(.6-thickness*(.53+.14*wallx+.07*thickness))),
                min_phase_transfer=float(liquid.min()),max_phase_excess=float(np.max(liquid-transfer)),
                max_capacity_excess=float(np.max(Lnew-newarea)),cross_wall_transfer=float(wrong),
                max_uniform_velocity_error=float(np.max(abs(newU-U)))))
            for key in ['max_donor_partition_error','max_receiver_capacity_error','max_liquid_oracle_error','total_phase_volume_error','cross_wall_transfer']:
                assert rows[-1][key]<1e-12,rows[-1]
    return rows


def averaging_check():
    m1,m2=1.,9.;u1,u2=1.,0.
    M=.5*(m1+m2);P=.5*(m1*u1+m2*u2)
    return dict(branch_masses=[m1,m2],branch_velocities=[u1,u2],correct_mass=M,correct_momentum=P,
                correct_velocity=P/M,independent_velocity_average=.5*(u1+u2),
                wrong_momentum=M*.5*(u1+u2),
                mean_branch_energy=.25*(m1*u1*u1+m2*u2*u2),mixed_energy=.5*P*P/M)


if __name__=='__main__':
    start=time.perf_counter()
    results=dict(scope='Independent mathematical/geometry checks, NOT Judas/P1 solver acceptance',
                 python=platform.python_version(),numpy=np.__version__,
                 momentum_ledger=momentum_experiments(),geometric_remap=remap_experiments(),averaging=averaging_check(),counterflow=counterflow_check())
    results['runtime_seconds']=time.perf_counter()-start
    out=Path(__file__).with_name('compatibility_results.json');out.write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps(results,indent=2))
