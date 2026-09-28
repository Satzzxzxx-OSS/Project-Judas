"""Independent exact geometry oracle. NOT Judas production code.
Quaternion input denotes an orientation: R(q)/dot(q,q), evaluated rationally.
Centers/radii/extents may be exact rational values or exact IEEE input bits.
Box-box oracle returns SAT separation SIGN, not an exterior Euclidean distance.
"""
from fractions import Fraction as F
from dataclasses import dataclass
from itertools import product
import math

def rat(x):
    if isinstance(x,F): return x
    return F(x)
def vec(v): return tuple(rat(x) for x in v)
def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def dot(a,b):return sum((x*y for x,y in zip(a,b)),F(0))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def matvec(R,v):return tuple(dot(row,v) for row in R)
def columns(R):return tuple(zip(*R))
def matmul(A,B):return tuple(tuple(dot(row,c) for c in columns(B)) for row in A)
def rotation(q):
    w,x,y,z=vec(q); d=w*w+x*x+y*y+z*z
    if d==0:raise ValueError('zero quaternion')
    return ((1-2*(y*y+z*z)/d,2*(x*y-w*z)/d,2*(x*z+w*y)/d),
            (2*(x*y+w*z)/d,1-2*(x*x+z*z)/d,2*(y*z-w*x)/d),
            (2*(x*z-w*y)/d,2*(y*z+w*x)/d,1-2*(x*x+y*y)/d))
I=rotation((1,0,0,0))
@dataclass(frozen=True)
class Sphere:
    c:tuple
    r:F
@dataclass(frozen=True)
class Box:
    c:tuple
    h:tuple
    R:tuple=I

def sphere_sphere(a,b):
    d=sub(a.c,b.c)
    return dot(d,d)-(a.r+b.r)**2

def sphere_box(s,b):
    rel=sub(s.c,b.c); local=tuple(dot(u,rel) for u in columns(b.R))
    outside=tuple(max(abs(v)-h,F(0)) for v,h in zip(local,b.h))
    return dot(outside,outside)-s.r*s.r

def sat_separations(a,b):
    A=columns(a.R); B=columns(b.R); d=sub(b.c,a.c)
    out=[]
    for axis in list(A)+list(B)+[cross(u,v) for u in A for v in B]:
        if dot(axis,axis)==0:continue
        sep=abs(dot(d,axis))-sum((h*abs(dot(u,axis)) for h,u in zip(a.h,A)),F(0))-sum((h*abs(dot(u,axis)) for h,u in zip(b.h,B)),F(0))
        out.append((sep,axis))
    return out

def witness(a,b):
    if isinstance(a,Sphere) and isinstance(b,Sphere):return sphere_sphere(a,b)
    if isinstance(a,Sphere) and isinstance(b,Box):return sphere_box(a,b)
    if isinstance(a,Box) and isinstance(b,Sphere):return sphere_box(b,a)
    return max(s for s,_ in sat_separations(a,b))
def overlap(a,b):return witness(a,b)<=0

def vertices(b):
    return [add(b.c,matvec(b.R,tuple(h*s for h,s in zip(b.h,signs)))) for signs in product((-1,1),repeat=3)]

def aabb(s):
    if isinstance(s,Sphere):return sub(s.c,(s.r,)*3),add(s.c,(s.r,)*3)
    extent=tuple(sum((abs(s.R[k][j])*s.h[j] for j in range(3)),F(0)) for k in range(3))
    return sub(s.c,extent),add(s.c,extent)
def aabb_overlap(a,b):return all(a[0][i]<=b[1][i] and b[0][i]<=a[1][i] for i in range(3))

def transform(s,R,t=(0,0,0)):
    c=add(matvec(R,s.c),vec(t))
    if isinstance(s,Sphere):return Sphere(c,s.r)
    return Box(c,s.h,matmul(R,s.R))

def compound_children(parent_c,parent_R,children):
    return [transform(s,parent_R,parent_c) for s in children]
def compound_overlap(a,b):return any(overlap(x,y) for x in a for y in b)

# Directed binary64 interval arithmetic. Deliberately simple research filter.
# The exact fallback below uses rational predicates, NOT an epsilon verdict.
@dataclass(frozen=True)
class Iv:
    lo:float
    hi:float
    @staticmethod
    def exact(x):
        if isinstance(x,Iv): return x
        if isinstance(x,F):
            f=float(x); ff=F(f)
            return Iv(math.nextafter(f,-math.inf) if ff>x else f,
                      math.nextafter(f,math.inf) if ff<x else f)
        f=float(x); return Iv(f,f)
    def __add__(self,x):
        x=Iv.exact(x)
        return Iv(math.nextafter(self.lo+x.lo,-math.inf),math.nextafter(self.hi+x.hi,math.inf))
    __radd__=__add__
    def __neg__(self):return Iv(-self.hi,-self.lo)
    def __sub__(self,x):return self+-Iv.exact(x)
    def __rsub__(self,x):return Iv.exact(x)+-self
    def __mul__(self,x):
        x=Iv.exact(x); a=[self.lo*x.lo,self.lo*x.hi,self.hi*x.lo,self.hi*x.hi]
        return Iv(math.nextafter(min(a),-math.inf),math.nextafter(max(a),math.inf))
    __rmul__=__mul__
    def __abs__(self):
        if self.lo>=0:return self
        if self.hi<=0:return -self
        return Iv(0,max(-self.lo,self.hi))
    def positive_part(self):return Iv(max(0.,self.lo),max(0.,self.hi))

def ivvec(a):return tuple(Iv.exact(x) for x in a)
def ivdot(a,b):return sum((x*y for x,y in zip(a,b)),Iv.exact(0))
def ivwitnesses(a,b):
    if isinstance(a,Box) and isinstance(b,Sphere):a,b=b,a
    d=sub(ivvec(b.c),ivvec(a.c)) # enclose inputs, then perform actual outward interval subtraction
    if isinstance(a,Sphere) and isinstance(b,Sphere):
        r=Iv.exact(a.r+b.r)
        return [ivdot(d,d)-r*r]
    if isinstance(a,Sphere):
        # Sign independent of direction of relative vector.
        B=[ivvec(u) for u in columns(b.R)]
        local=[ivdot(u,d) for u in B]
        out=[(abs(v)-Iv.exact(h)).positive_part() for v,h in zip(local,b.h)]
        r=Iv.exact(a.r)
        return [ivdot(out,out)-r*r]
    A=[ivvec(u) for u in columns(a.R)];B=[ivvec(u) for u in columns(b.R)]
    # Include cross axes even if interval contains zero. Exact parallel axis = 0
    # is excluded using exact geometry in this oracle/filter research only.
    axes=list(A)+list(B)
    for i,u in enumerate(A):
        for j,v in enumerate(B):
            if dot(cross(columns(a.R)[i],columns(b.R)[j]),cross(columns(a.R)[i],columns(b.R)[j]))!=0:
                axes.append(cross(u,v))
    out=[]
    for ax in axes:
        out.append(abs(ivdot(d,ax))-sum((Iv.exact(h)*abs(ivdot(u,ax)) for h,u in zip(a.h,A)),Iv.exact(0))-sum((Iv.exact(h)*abs(ivdot(u,ax)) for h,u in zip(b.h,B)),Iv.exact(0)))
    return out

def filtered_overlap(a,b):
    ss=ivwitnesses(a,b)
    if any(s.lo>0 for s in ss):return False,False,ss
    if all(s.hi<0 for s in ss):return True,False,ss
    return overlap(a,b),True,ss

def point_box_sq(p,b):
    rel=sub(p,b.c);ll=tuple(dot(a,rel) for a in columns(b.R))
    dd=tuple(max(abs(v)-h,F(0)) for v,h in zip(ll,b.h))
    return dot(dd,dd)

def point_segment_sq(p,a,b):
    u=sub(b,a);uu=dot(u,u)
    s=max(F(0),min(F(1),dot(sub(p,a),u)/uu)) if uu else F(0)
    d=sub(p,add(a,mul(u,s)));return dot(d,d)

def segment_segment_sq(p0,p1,q0,q1):
    cand=[point_segment_sq(p0,q0,q1),point_segment_sq(p1,q0,q1),point_segment_sq(q0,p0,p1),point_segment_sq(q1,p0,p1)]
    u=sub(p1,p0);v=sub(q1,q0);w=sub(p0,q0)
    a=dot(u,u);b=dot(u,v);c=dot(v,v);d=dot(u,w);e=dot(v,w);D=a*c-b*b
    if D>0:
        s=(b*e-c*d)/D;t=(a*e-b*d)/D
        if 0<=s<=1 and 0<=t<=1:
            z=sub(add(w,mul(u,s)),mul(v,t));cand.append(dot(z,z))
    return min(cand)

def edges(b):
    ans=[]
    for axis in range(3):
        rest=[k for k in range(3) if k!=axis]
        for ss in product((-1,1),repeat=2):
            v=[F(0)]*3;v[axis]=-b.h[axis]
            for k,sign in zip(rest,ss):v[k]=sign*b.h[k]
            w=list(v);w[axis]=b.h[axis]
            ans.append((add(b.c,matvec(b.R,tuple(v))),add(b.c,matvec(b.R,tuple(w)))))
    return ans

def box_distance_sq(a,b):
    if overlap(a,b):return F(0)
    ds=[point_box_sq(p,b) for p in vertices(a)]+[point_box_sq(p,a) for p in vertices(b)]
    ds.extend(segment_segment_sq(*p,*q) for p in edges(a) for q in edges(b))
    return min(ds)

def translation_box_toi(a,b,va,vb,dt):
    A=columns(a.R);B=columns(b.R);delta=sub(b.c,a.c);vel=sub(vb,va)
    lo=F(0);hi=rat(dt)
    for axis in list(A)+list(B)+[cross(u,v) for u in A for v in B]:
        if dot(axis,axis)==0:continue
        c=dot(delta,axis);w=dot(vel,axis)
        r=sum((h*abs(dot(u,axis)) for h,u in zip(a.h,A)),F(0))+sum((h*abs(dot(u,axis)) for h,u in zip(b.h,B)),F(0))
        if w==0:
            if abs(c)>r:return None
            continue
        t0=(-r-c)/w;t1=(r-c)/w
        lo=max(lo,min(t0,t1));hi=min(hi,max(t0,t1))
        if lo>hi:return None
    return lo
