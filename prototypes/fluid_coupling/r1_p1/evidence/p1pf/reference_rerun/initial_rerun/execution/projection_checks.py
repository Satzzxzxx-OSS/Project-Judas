#!/usr/bin/env python3
"""Independent research check: resolved, frozen-geometry MAC fluid/body projection.

Not Judas source. No advection, body-pose advance, cut-cell approximation, or PBF.
The geometry really is a union of grid cells; arbitrary curves are NOT voxelized.
All lengths use a 1 m unit-depth slice. Boundary fluid half-dual masses are retained.
Run with OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python projection_checks.py.
"""
from __future__ import annotations
import json
import math
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Callable
import numpy as np
from scipy import linalg


def cross(a: np.ndarray, b: np.ndarray) -> float:
    return float(a[0]*b[1]-a[1]*b[0])


class IncompatibleConstraints(RuntimeError):
    pass


@dataclass
class Body:
    mass: float
    inertia: float
    centre: np.ndarray              # grid-local coordinates


@dataclass
class Face:
    axis: int
    i: int
    j: int
    xy: np.ndarray                 # grid-local, used without large-origin subtraction
    area: float
    adjacent: list[tuple[int, int, int]]  # (cell_i, cell_j, outward sign)
    mass: float
    owner: int | None               # body id, -1 for domain wall, None interior
    side: str | None               # domain side


@dataclass
class Projection:
    velocity: np.ndarray
    impulses: np.ndarray
    rank: int
    nullity: int
    whitened_condition: float
    constraint_residual: float
    impulse_residual: float
    energy_identity_residual: float
    kinetic_before: float
    kinetic_after: float
    prescribed_boundary_work: float
    rejected_compatibility: float


class FrozenMAC:
    """One Cartesian MAC grid (which may be rigidly rotated in world space).

    Fine masses are sampled at half the MAC spacing. Every active face dual
    is the union of 2x2 fine cells, with missing solid/outside pieces excluded.
    Boundaries are physically aligned grid segments, not approximate cut cells.
    """
    def __init__(self, labels: np.ndarray, fine_density: np.ndarray,
                 bodies: dict[int, Body] | None = None,
                 boundary: dict[str, str] | None = None,
                 supports: list[tuple[int, int]] | None = None,
                 rotation: float = 0.0, origin=(0., 0.),
                 wall_velocity: Callable[[np.ndarray], np.ndarray] | None = None,
                 external_pressure: Callable[[np.ndarray], float] | None = None):
        self.labels = np.asarray(labels, dtype=int)
        self.nx, self.ny = self.labels.shape
        self.hx, self.hy = 1./self.nx, 1./self.ny
        self.rho = np.asarray(fine_density, dtype=float)
        assert self.rho.shape == (2*self.nx, 2*self.ny)
        if np.any(self.rho <= 0) or not np.all(np.isfinite(self.rho)):
            raise ValueError('All phase/fluid masses must be positive and finite.')
        self.bodies = bodies or {}
        self.boundary = {s:'wall' for s in ('left','right','bottom','top')}
        self.boundary.update(boundary or {})
        self.supports = supports or []
        c,s = math.cos(rotation), math.sin(rotation)
        self.Q = np.array([[c,-s],[s,c]])
        self.origin = np.array(origin, dtype=float)
        self.wall_velocity = wall_velocity or (lambda x: np.zeros(2))
        self.external_pressure = external_pressure or (lambda x: 0.)
        self.cells = [(i,j) for i in range(self.nx) for j in range(self.ny)
                      if self.labels[i,j] < 0]
        self.cell_index = {c:k for k,c in enumerate(self.cells)}
        self.faces: list[Face] = []
        self.face_index = {}
        for ax in (0,1):
            for i in range(self.nx+(ax==0)):
                for j in range(self.ny+(ax==1)):
                    pair = [(i-1,j,+1),(i,j,-1)] if ax==0 else [(i,j-1,+1),(i,j,-1)]
                    adj = [(a,b,sg) for a,b,sg in pair
                           if (a,b) in self.cell_index]
                    if not adj:
                        continue
                    owner=None; side=None
                    if len(adj)==1:
                        missing=[p for p in pair if p not in adj][0]
                        a,b,_=missing
                        if 0<=a<self.nx and 0<=b<self.ny:
                            owner=int(self.labels[a,b])
                            assert owner in self.bodies, 'No undeclared solid labels.'
                        else:
                            owner=-1
                            side='left' if a<0 else 'right' if a>=self.nx else 'bottom' if b<0 else 'top'
                    if ax==0:
                        fi=[2*i-1,2*i]; fj=[2*j,2*j+1]
                        xy=np.array([i*self.hx,(j+.5)*self.hy]); area=self.hy
                    else:
                        fi=[2*i,2*i+1]; fj=[2*j-1,2*j]
                        xy=np.array([(i+.5)*self.hx,j*self.hy]); area=self.hx
                    mass=0.
                    for a in fi:
                        for b in fj:
                            if 0<=a<2*self.nx and 0<=b<2*self.ny and self.labels[a//2,b//2]<0:
                                mass+=self.rho[a,b]*self.hx*self.hy/4.
                    assert mass>0
                    self.face_index[(ax,i,j)]=len(self.faces)
                    self.faces.append(Face(ax,i,j,xy,area,adj,mass,owner,side))
        self.nf=len(self.faces)
        self.body_offset={b:self.nf+3*k for k,b in enumerate(sorted(self.bodies))}
        n=self.nf+3*len(self.bodies)
        H=[f.mass for f in self.faces]
        for b in sorted(self.bodies):
            bod=self.bodies[b]
            if bod.mass<=0 or bod.inertia<=0:
                raise ValueError('Positive body mass/inertia required; use constraints for supports.')
            H += [bod.mass,bod.mass,bod.inertia]
        self.H=np.array(H)
        rows=[]; rhs=[]; meta=[]
        # Integrated incompressibility, pressure impulse pi = dt*p.
        for i,j in self.cells:
            row=np.zeros(n)
            for ax,a,b,sg in [(0,i,j,-1),(0,i+1,j,+1),(1,i,j,-1),(1,i,j+1,+1)]:
                k=self.face_index[(ax,a,b)]
                row[k]=sg*self.faces[k].area
            rows.append(row);rhs.append(0.);meta.append(('cell',(i,j)))
        self.nc=len(rows)
        self.wall_rows=[]
        self.body_rows={b:[] for b in self.bodies}
        for k,f in enumerate(self.faces):
            if f.owner is None:
                continue
            if f.owner==-1 and self.boundary[f.side]=='pressure':
                continue
            sg=f.adjacent[0][2]
            normal=sg*self.Q[:,f.axis]  # outward from fluid into solid / environment
            row=np.zeros(n);row[k]=-sg*f.area
            if f.owner>=0:
                off=self.body_offset[f.owner]
                lever=self.Q@(f.xy-self.bodies[f.owner].centre)
                row[off:off+2]=f.area*normal
                row[off+2]=f.area*cross(lever,normal)
                val=0.
                self.body_rows[f.owner].append(len(rows))
            else:
                val=-f.area*float(np.dot(normal,self.wall_velocity(self.Q@f.xy+self.origin)))
                self.wall_rows.append(len(rows))
            rows.append(row);rhs.append(val);meta.append(('surface',k))
        self.support_rows=[]
        for body,component in self.supports:
            row=np.zeros(n);row[self.body_offset[body]+component]=1.
            self.support_rows.append(len(rows))
            rows.append(row);rhs.append(0.);meta.append(('support',(body,component)))
        self.B=np.array(rows);self.rhs=np.array(rhs);self.meta=meta
        # Physical discrete linear/angular momentum using face impulse locations.
        self.moment=np.zeros((3,n))
        for k,f in enumerate(self.faces):
            e=self.Q[:,f.axis]; r=self.Q@f.xy
            self.moment[:2,k]=f.mass*e
            self.moment[2,k]=f.mass*cross(r,e)
        for body,bod in self.bodies.items():
            off=self.body_offset[body];r=self.Q@bod.centre
            self.moment[:2,off:off+2]=np.eye(2)*bod.mass
            self.moment[2,off]=-bod.mass*r[1]
            self.moment[2,off+1]=bod.mass*r[0]
            self.moment[2,off+2]=bod.inertia

    def provisional(self, dt: float, gravity=(0.,0.), body_force=None,
                    uniform_velocity=(0.,0.)) -> np.ndarray:
        g=np.array(gravity);U=np.array(uniform_velocity)
        w=np.zeros(len(self.H))
        for k,f in enumerate(self.faces):
            w[k]=np.dot(self.Q[:,f.axis],U+dt*g)
            if f.owner==-1 and self.boundary[f.side]=='pressure':
                sg=f.adjacent[0][2]
                pressure=self.external_pressure(self.Q@f.xy+self.origin)
                w[k]-=sg*f.area*dt*pressure/self.H[k]
        for b,bod in self.bodies.items():
            off=self.body_offset[b]
            w[off:off+2]=U+dt*g
            if body_force and b in body_force:
                f=np.asarray(body_force[b])
                w[off:off+3]+=dt*f/self.H[off:off+3]
        return w

    def project(self,wstar:np.ndarray, H_override=None, rhs_override=None) -> Projection:
        H=self.H if H_override is None else np.asarray(H_override)
        rhs=self.rhs if rhs_override is None else np.asarray(rhs_override)
        if np.any(H<=0):raise ValueError('No zero/negative mass inversion or mass floor.')
        invroot=1./np.sqrt(H)
        unscaled=self.B*invroot[None,:]
        norms=np.linalg.norm(unscaled,axis=1)
        if np.any(norms==0):raise ValueError('Zero constraint rows require explicit classification.')
        scale=1./norms
        C=scale[:,None]*unscaled
        ystar=np.sqrt(H)*wstar
        d=scale*(rhs-self.B@wstar)
        U,s,Vh=linalg.svd(C,full_matrices=False,lapack_driver='gesvd')
        tol=64*np.finfo(float).eps*max(C.shape)*s[0]
        rank=int(np.count_nonzero(s>tol))
        U=U[:,:rank];sr=s[:rank];Vh=Vh[:rank,:]
        coeff=U.T@d
        dy=Vh.T@(coeff/sr)
        incompat=d-C@dy
        incompat_err=float(np.linalg.norm(incompat)/max(1.,np.linalg.norm(d)))
        if incompat_err>1e-10:
            raise IncompatibleConstraints(f'Incompatible prescribed volume/constraint flux: {incompat_err:.6g}')
        w=wstar+invroot*dy
        lam=scale*(U@(coeff/(sr*sr)))
        delta=w-wstar
        cres=float(np.linalg.norm(self.B@w-rhs)/max(1.,np.linalg.norm(self.B@wstar),np.linalg.norm(rhs)))
        impulse=H*delta-self.B.T@lam
        ires=float(np.linalg.norm(impulse)/max(1.,np.linalg.norm(H*delta)))
        k0=float(.5*np.dot(H*wstar,wstar));k1=float(.5*np.dot(H*w,w))
        work=float(np.dot(lam,rhs));diss=float(.5*np.dot(H*delta,delta))
        eres=float(abs(k1-k0-work+diss)/max(1.,k0,k1,abs(work),diss))
        return Projection(w,lam,rank,len(rhs)-rank,float(sr[0]/sr[-1]),cres,ires,eres,k0,k1,work,incompat_err)

    def surface_force(self, result:Projection, body:int, dt:float) -> np.ndarray:
        # Read the interface pressure multipliers. No analytic buoyancy term.
        out=np.zeros(3)
        for row in self.body_rows[body]:
            k=self.meta[row][1];f=self.faces[k];sg=f.adjacent[0][2]
            normal=sg*self.Q[:,f.axis]
            lever=self.Q@(f.xy-self.bodies[body].centre)
            out[:2]+=f.area*normal*result.impulses[row]/dt
            out[2]+=f.area*cross(lever,normal)*result.impulses[row]/dt
        return out

    def momentum_budget(self,result:Projection,wstar:np.ndarray) -> float:
        external=np.zeros(len(self.H))
        for row in self.wall_rows+self.support_rows:
            external+=self.B[row,:]*result.impulses[row]
        mismatch=self.moment@(result.velocity-wstar)-self.moment@(external/self.H)
        return float(np.linalg.norm(mismatch)/max(1.,np.linalg.norm(self.moment@wstar),np.linalg.norm(self.moment@result.velocity)))

    def mass_partition_error(self) -> float:
        m=0.
        for i,j in self.cells:m+=self.rho[2*i:2*i+2,2*j:2*j+2].sum()*self.hx*self.hy/4
        mx=sum(f.mass for f in self.faces if f.axis==0)
        my=sum(f.mass for f in self.faces if f.axis==1)
        return max(abs(mx-m),abs(my-m))/max(1.,m)


def geometry_body(labels:np.ndarray, body:int, density:float)->Body:
    nx,ny=labels.shape;dx=1/nx;dy=1/ny
    pieces=np.array([[(i+.5)*dx,(j+.5)*dy] for i,j in zip(*np.where(labels==body))])
    centre=pieces.mean(axis=0);area=len(pieces)*dx*dy;mass=density*area
    inertia=density*dx*dy*sum((dx*dx+dy*dy)/12+np.dot(r-centre,r-centre) for r in pieces)
    return Body(mass,float(inertia),centre)


def layered_density(nx:int,ny:int,rhol:float,rhog:float,yfill:float)->np.ndarray:
    out=np.empty((2*nx,2*ny));h=1/(2*ny)
    for j in range(2*ny):
        # Analytical geometric overlap, not a clamp applied to evolved data.
        liquid_length=max(0.,min((j+1)*h,yfill)-j*h)
        out[:,j]=rhog+(rhol-rhog)*liquid_length/h
    return out


def metrics(p:Projection, grid:FrozenMAC, wstar:np.ndarray)->dict:
    return dict(constraint_residual=p.constraint_residual,impulse_residual=p.impulse_residual,
                energy_identity_residual=p.energy_identity_residual,
                momentum_budget_residual=grid.momentum_budget(p,wstar),
                mass_partition_error=grid.mass_partition_error(),
                nullity=p.nullity,whitened_condition=p.whitened_condition,
                kinetic_before=p.kinetic_before,kinetic_after=p.kinetic_after,
                prescribed_boundary_work=p.prescribed_boundary_work,
                max_abs_velocity=float(np.max(abs(p.velocity))))


def checked(name:str, grid:FrozenMAC, wstar:np.ndarray, results:list)->Projection:
    p=grid.project(wstar)
    d=metrics(p,grid,wstar)
    for key in ('constraint_residual','impulse_residual','energy_identity_residual','momentum_budget_residual','mass_partition_error'):
        if d[key]>1e-9:raise AssertionError((name,key,d[key]))
    if np.all(grid.rhs==0) and p.kinetic_after>p.kinetic_before+1e-9*max(1.,p.kinetic_before):
        raise AssertionError((name,'energy grew in homogeneous projection'))
    results.append({'name':name,**d})
    return p


def main():
    start=time.perf_counter();results=[];negative=[];nx=ny=12;dt=.01;g0=9.81
    # 1. All-fluid variable-density projection and both boundary types.
    for contrast in (1.,1e3,1e6):
        labels=np.full((8,8),-1);rng=np.random.default_rng(418)
        rho=1+(contrast-1)*rng.random((16,16))
        for opened in (False,True):
            grid=FrozenMAC(labels,rho,boundary={'top':'pressure'} if opened else {})
            w=rng.normal(size=len(grid.H))
            p=checked(f'random_fluid_{contrast:g}_open{opened}',grid,w,results)
            assert p.nullity==(0 if opened else 1)
            p2=grid.project(p.velocity)
            results[-1]['idempotence_error']=float(np.max(abs(p2.velocity-p.velocity)))
            assert results[-1]['idempotence_error']<1e-9
    # Exact two-phase jumps, not only random mixed densities.
    for contrast in (1.,1e3,1e6):
        labels=np.full((12,12),-1)
        rho=layered_density(12,12,contrast,1.,.5)
        grid=FrozenMAC(labels,rho,boundary={'top':'pressure'})
        w=grid.provisional(dt,(0.,-g0));p=checked('sharp_density_jump_'+str(contrast),grid,w,results)
        expected=np.array([g0*((1-(j+.5)/12)+(contrast-1)*max(.5-(j+.5)/12,0.)) for i,j in grid.cells])
        perr=float(np.max(abs(p.impulses[:grid.nc]/dt-expected))/max(1.,np.max(abs(expected))))
        results[-1]['hydro_pressure_relative_error']=perr
        assert perr<1e-9 and np.max(abs(p.velocity))<1e-8
    # 2. Supported hydrostatics and free neutral/light/heavy bodies in liquid.
    labels=np.full((nx,ny),-1);labels[5:7,3:5]=0
    rho=layered_density(nx,ny,1000.,1.,.5)
    for ratio in (.001,.5,1.,2.,1000.):
        bod=geometry_body(labels,0,1000*ratio)
        grid=FrozenMAC(labels,rho,{0:bod},boundary={'top':'pressure'})
        w=grid.provisional(dt,(0.,-g0))
        p=checked(f'submerged_free_density_ratio_{ratio:g}',grid,w,results)
        q=p.velocity[grid.body_offset[0]:]
        results[-1]['body_velocity']=q.tolist()
        if ratio==1.:assert np.max(abs(q))<1e-9
        else:assert q[1]*(1-ratio)>0
        # Body held by explicit mechanical constraints: pressure is hydrostatic.
        hold=FrozenMAC(labels,rho,{0:bod},boundary={'top':'pressure'},supports=[(0,k) for k in range(3)])
        w=hold.provisional(dt,(0.,-g0));hp=checked(f'submerged_supported_density_ratio_{ratio:g}',hold,w,results)
        assert np.max(abs(hp.velocity))<1e-9
        force=hold.surface_force(hp,0,dt)
        expected=1000*(2/nx)*(2/ny)*g0
        results[-1]['hydro_force']=force.tolist();results[-1]['hydro_force_expected_y']=expected
        assert np.max(abs(force-np.array([0.,expected,0.])))<1e-7
        cellp=hp.impulses[:hold.nc]/dt
        exact=np.array([g0*(1.*(1-(j+.5)/ny)+999.*max(.5-(j+.5)/ny,0.)) for i,j in hold.cells])
        results[-1]['hydro_pressure_error']=float(np.max(abs(cellp-exact)))
        assert results[-1]['hydro_pressure_error']<1e-7
    # 3. Body pressure and torque from a partially immersed, asymmetric L-shape.
    lab=np.full((nx,ny),-1);lab[4:8,4:5]=0;lab[4:5,5:8]=0
    bod=geometry_body(lab,0,800.);rho=layered_density(nx,ny,1000.,1.,.5)
    grid=FrozenMAC(lab,rho,{0:bod},boundary={'top':'pressure'},supports=[(0,k) for k in range(3)])
    w=grid.provisional(dt,(0.,-g0));p=checked('asymmetric_partial_immersion_hydrostatic_torque',grid,w,results)
    force=grid.surface_force(p,0,dt)
    # Independent analytical rectangle integrals: base 4/12 x 1/12 below
    # interface; upright 1/12 x 1/12 below, 1/12 x 2/12 above.
    rectangles=[(4/12,8/12,4/12,5/12,1000.),(4/12,5/12,5/12,6/12,1000.),(4/12,5/12,6/12,8/12,1.)]
    fy=tor=0.
    for x0,x1,y0,y1,den in rectangles:
        F=den*(x1-x0)*(y1-y0)*g0;fy+=F;tor+=(((x0+x1)/2)-bod.centre[0])*F
    results[-1].update(actual_force_torque=force.tolist(),expected_force_torque=[0.,fy,tor])
    assert np.max(abs(force-np.array([0.,fy,tor])))<1e-7
    # 4. Resolved U-shaped cup, supported, same boundary operator inside/outside.
    lab=np.full((nx,ny),-1);lab[3:9,2:3]=0;lab[3:4,3:9]=0;lab[8:9,3:9]=0
    rh=np.ones((2*nx,2*ny))
    # Internal water rectangle x=4/12..8/12, y=3/12..6/12.
    rh[8:16,6:12]=1000.
    bod=geometry_body(lab,0,500.)
    cup=FrozenMAC(lab,rh,{0:bod},boundary={'top':'pressure'},supports=[(0,k) for k in range(3)])
    w=cup.provisional(dt,(0.,-g0));p=checked('supported_resolved_cup_load',cup,w,results)
    force=cup.surface_force(p,0,dt)
    Vwater=(4/12)*(3/12);Vsolid=(6+6+6)/(12*12)
    expected_y=g0*(Vsolid-999*Vwater)
    support_imp=sum(p.impulses[r] for r in cup.support_rows if cup.meta[r][1][1]==1)
    results[-1].update(fluid_force_y=float(force[1]),expected_fluid_force_y=expected_y,
                      support_force_y=float(support_imp/dt),expected_support_force_y=bod.mass*g0-expected_y,
                      max_water_gas_speed=float(np.max(abs(p.velocity[:cup.nf]))))
    assert abs(force[1]-expected_y)<1e-7
    assert abs(support_imp/dt-(bod.mass*g0-expected_y))<1e-7
    assert np.max(abs(p.velocity))<1e-9
    # 5. True many-cell piston/plug inertia; closed-form Newtonian added mass.
    lab=np.full((12,6),-1);lab[5:7,:]=0
    rh=np.ones((24,12));rh[:10,:]=1000.
    piston_runs=[]
    for mass in (.01,.1,1.,80.,1000.,1e6):
        base=geometry_body(lab,0,1.);bod=Body(mass,base.inertia*mass/base.mass,base.centre)
        grid=FrozenMAC(lab,rh,{0:bod},boundary={'left':'pressure','right':'pressure'},supports=[(0,1),(0,2)])
        w=grid.provisional(dt,body_force={0:np.array([100.,0.,0.])})
        p=checked(f'open_piston_mass_{mass:g}',grid,w,results)
        mf=1000*(5/12)+1*(5/12);U=100*dt/(mass+mf)
        actual=float(p.velocity[grid.body_offset[0]])
        results[-1].update(expected_velocity=U,actual_velocity=actual,relative_velocity_error=abs(actual-U)/U,
                          fluid_mass_expected=mf)
        assert abs(actual-U)/U<1e-8
        xvel=[p.velocity[k] for k,f in enumerate(grid.faces) if f.axis==0]
        assert max(abs(np.array(xvel)-U))<1e-9
        piston_runs.append((mass,grid,w,p,U))
    # Negative: actual wrong interface-half-dual masses, without changing oracle.
    mass,grid,w,p,U=piston_runs[3]
    badH=grid.H.copy()
    for row in grid.body_rows[0]:badH[grid.meta[row][1]]*=.5
    bad=grid.project(w,H_override=badH)
    wrong=float(bad.velocity[grid.body_offset[0]])
    negative.append({'name':'halve_interface_half_dual_masses','correct_velocity':U,'wrong_velocity':wrong,
                     'relative_error':abs(wrong-U)/U})
    assert abs(wrong-U)/U>1e-3
    # A one-pass explicit reaction estimate is an independently wrong coupling control.
    mass,grid,w,p,U=piston_runs[0]
    mf=1000*(5/12)+5/12;qstar=100*dt/mass
    wrong=qstar-mf*qstar/mass
    negative.append({'name':'lagged_partitioned_piston','mass':mass,'correct_velocity':U,'wrong_velocity':wrong})
    assert abs(wrong-U)>1
    # 6. Sealed piston couples the pressure gauges of two disconnected chambers.
    bod=geometry_body(lab,0,480.)
    grid=FrozenMAC(lab,np.ones((24,12))*1000,{0:bod},supports=[(0,1),(0,2)])
    w=grid.provisional(dt,body_force={0:np.array([100.,0.,0.])})
    p=checked('sealed_piston_pressure_difference',grid,w,results)
    assert p.nullity==1
    lp=np.mean([p.impulses[k]/dt for k,(i,j) in enumerate(grid.cells) if i<5])
    rp=np.mean([p.impulses[k]/dt for k,(i,j) in enumerate(grid.cells) if i>=7])
    results[-1].update(left_pressure=float(lp),right_pressure=float(rp),pressure_difference=float(rp-lp),expected_difference=100.)
    assert abs(rp-lp-100.)<1e-8 and np.max(abs(p.velocity))<1e-9
    # Negative: pin one pressure separately in each chamber by deleting two rows/cols.
    A=(grid.B/grid.H[None,:])@grid.B.T;b=grid.rhs-grid.B@w
    pins=[grid.cell_index[(0,0)],grid.cell_index[(11,0)]]
    keep=np.array([i for i in range(len(b)) if i not in pins])
    lm=np.zeros(len(b));lm[keep]=linalg.lstsq(A[np.ix_(keep,keep)],b[keep],lapack_driver='gelsd')[0]
    wrongw=w+(grid.B.T@lm)/grid.H
    leak=float(np.max(abs(grid.B@wrongw-grid.rhs)))
    negative.append({'name':'pin_each_disconnected_chamber','constraint_violation':leak,
                     'body_velocity':float(wrongw[grid.body_offset[0]])})
    assert leak>1e-6
    # 7. Constant ambient pressure gives no force/torque/velocity on a closed body.
    lab=np.full((nx,ny),-1);lab[5:7,3:5]=0;bod=geometry_body(lab,0,700.)
    grid=FrozenMAC(lab,np.ones((24,24))*1000,{0:bod},boundary={s:'pressure' for s in ['left','right','top','bottom']},
                   external_pressure=lambda x:1234.)
    w=grid.provisional(dt);p=grid.project(w)
    # Known ambient-pressure impulses are part of wstar; separate check, not homogeneous energy gate.
    d=metrics(p,grid,w);results.append({'name':'constant_ambient_pressure',**d,'pressure_force':grid.surface_force(p,0,dt).tolist()})
    assert np.max(abs(p.velocity))<1e-9 and np.max(abs(grid.surface_force(p,0,dt)))<1e-7
    results[-1]['constant_pressure_error']=float(np.max(abs(p.impulses[:grid.nc]/dt-1234.)))
    assert results[-1]['constant_pressure_error']<1e-7
    # 8. Common free fall: prescribe outer-wall normal velocities coherently.
    rho=layered_density(nx,ny,1000.,1.,.5)
    g=np.array([1.7,-g0]);U=np.array([.8,-.3])
    grid=FrozenMAC(lab,rho,{0:bod},wall_velocity=lambda x:U+dt*g)
    w=grid.provisional(dt,g,uniform_velocity=U);p=checked('common_free_fall',grid,w,results)
    results[-1]['free_fall_correction']=float(np.max(abs(p.velocity-w)))
    assert np.max(abs(p.velocity-w))<1e-10
    # 9. Whole-grid/body/frame rotational and translational covariance.
    rng=np.random.default_rng(561);rho=1+999*rng.random((24,24))
    base=FrozenMAC(lab,rho,{0:bod});w=rng.normal(size=len(base.H))
    p=checked('covariance_reference',base,w,results)
    for angle,origin in [(.731,(0.,0.)),(.731,(1e9,-2e9))]:
        grid=FrozenMAC(lab,rho,{0:bod},rotation=angle,origin=origin)
        rotated=w.copy();off=grid.body_offset[0];rotated[off:off+2]=grid.Q@w[off:off+2]
        pp=checked(f'covariance_angle{angle}_origin{origin[0]:g}',grid,rotated,results)
        expected=p.velocity.copy();expected[off:off+2]=grid.Q@p.velocity[off:off+2]
        err=float(np.max(abs(pp.velocity-expected)));results[-1]['covariance_velocity_error']=err
        assert err<1e-9
    # Galilean transform is legal only when prescribed boundary data transform too.
    shift=np.array([2.3,-1.2]);grid=FrozenMAC(lab,rho,{0:bod},wall_velocity=lambda x:shift)
    add=grid.provisional(0.,uniform_velocity=shift)
    pp=checked('galilean_covariance',grid,w+add,results)
    err=float(np.max(abs(pp.velocity-p.velocity-add)));results[-1]['covariance_velocity_error']=err
    assert err<1e-9
    # 10. Slip: tangential fluid motion in a channel is not clamped.
    # Left/right pressure boundaries, fixed top/bottom walls; plug x velocity admissible.
    grid=FrozenMAC(np.full((8,6),-1),np.ones((16,12))*1000,boundary={'left':'pressure','right':'pressure'})
    w=grid.provisional(0.,uniform_velocity=(1.7,0.));p=checked('normal_only_free_slip',grid,w,results)
    assert np.max(abs(p.velocity-w))<1e-10
    # 11. Reject incompatible prescribed volume instead of deleting a mean RHS.
    grid=FrozenMAC(np.full((6,6),-1),np.ones((12,12))*1000)
    badrhs=grid.rhs.copy()
    row=next(r for r in grid.wall_rows if grid.faces[grid.meta[r][1]].side=='left')
    badrhs[row]=.1
    try:grid.project(np.zeros(len(grid.H)),rhs_override=badrhs)
    except IncompatibleConstraints as e:negative.append({'name':'incompatible_closed_wall_flux','caught':str(e)})
    else:raise AssertionError('Incompatible constraint was silently repaired.')
    # 12. Two independently moving bodies, widely differing inertias, same solve.
    lab=np.full((10,10),-1);lab[2:4,3:5]=0;lab[6:8,6:8]=1
    bodies={0:geometry_body(lab,0,1.),1:geometry_body(lab,1,1e4)}
    grid=FrozenMAC(lab,np.ones((20,20))*1000,bodies)
    rng=np.random.default_rng(151);w=rng.normal(size=len(grid.H))
    p=checked('two_bodies_mixed_mass',grid,w,results)
    # Negative: omit rotational lever arm from reaction after a correct solve.
    correct_delta=(grid.B.T@p.impulses)/grid.H
    wrong_delta=correct_delta.copy()
    for b in bodies:wrong_delta[grid.body_offset[b]+2]=0.
    angular_leak=float(abs((grid.moment@(wrong_delta-correct_delta))[2]))
    negative.append({'name':'drop_rotational_reaction','angular_momentum_error':angular_leak})
    assert angular_leak>1e-5
    # 13. Fixed-grid hydrostatics with arbitrary gravity, no rotation of the mesh.
    lab=np.full((12,12),-1);lab[5:7,3:5]=0
    bod=geometry_body(lab,0,1000.)
    for grav in [np.array([3.1,-9.81]),np.array([-7.,2.]),np.array([9.81,0.])]:
        grid=FrozenMAC(lab,np.full((24,24),1000.),{0:bod},
                       boundary={side:'pressure' for side in ('left','right','bottom','top')},
                       external_pressure=lambda x,g=grav:20000.+1000.*float(g@x))
        w=grid.provisional(dt,gravity=grav);p=checked('arbitrary_gravity_'+str(grav.tolist()),grid,w,results)
        force=grid.surface_force(p,0,dt);expected=-bod.mass*grav
        results[-1].update(pressure_force=force.tolist(),expected_pressure_force=expected.tolist())
        assert np.max(abs(p.velocity))<1e-9
        assert np.max(abs(force[:2]-expected))<1e-7 and abs(force[2])<1e-7
    # 14. Independent smooth-pressure convergence oracle, not a manufactured
    # RHS assembled from the discrete pressure matrix.
    errors=[]
    for n in (6,12,24):
        grid=FrozenMAC(np.full((n,n),-1),np.full((2*n,2*n),1000.),
                       boundary={side:'pressure' for side in ('left','right','bottom','top')})
        w=np.zeros(len(grid.H));amplitude=500.
        for k,f in enumerate(grid.faces):
            x,y=f.xy
            dp=(amplitude*np.pi*np.cos(np.pi*x)*np.sin(np.pi*y) if f.axis==0 else
                amplitude*np.pi*np.sin(np.pi*x)*np.cos(np.pi*y))
            w[k]=dt*dp/1000.
        p=checked('smooth_pressure_refinement_'+str(n),grid,w,results)
        expected=np.array([amplitude*np.sin(np.pi*(i+.5)/n)*np.sin(np.pi*(j+.5)/n) for i,j in grid.cells])
        err=float(np.mean(abs(p.impulses[:grid.nc]/dt-expected)))
        errors.append(err);results[-1]['pressure_L1']=err
        assert np.max(abs(p.velocity))<1e-9
    assert errors[2]<errors[1]<errors[0]
    assert all(3.7 < errors[k]/errors[k+1] < 4.4 for k in (0,1))
    keys=['constraint_residual','impulse_residual','energy_identity_residual','momentum_budget_residual','mass_partition_error']
    summary={k:max(r[k] for r in results) for k in keys}
    assert all(v<1e-9 for v in summary.values())
    payload={'status':'PASS','scope':'Independent frozen, resolved-geometry 2D projection; not Judas or moving-body timesteps.',
             'cases':len(results),'negative_controls':len(negative),'seconds':time.perf_counter()-start,
             'worst':summary,'results':results,'negative':negative}
    path=Path(__file__).with_name('projection_results.json');path.write_text(json.dumps(payload,indent=2)+'\n')
    print(json.dumps({k:v for k,v in payload.items() if k not in ('results','negative')},indent=2))
    for r in results:
        if any(k in r for k in ('hydro_force','actual_velocity','fluid_force_y','pressure_difference','free_fall_correction')):
            print(r['name'], {k:v for k,v in r.items() if k in ('hydro_force','actual_velocity','expected_velocity','relative_velocity_error','fluid_force_y','expected_fluid_force_y','support_force_y','expected_support_force_y','pressure_difference','free_fall_correction')})
    print('NEGATIVE CONTROLS',json.dumps(negative,indent=2))


if __name__=='__main__':main()
