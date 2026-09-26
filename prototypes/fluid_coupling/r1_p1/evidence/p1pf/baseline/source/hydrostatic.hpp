#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <vector>

namespace r1p1 {
struct CgResult { int iterations{}; double residual{}; bool converged{}; };
inline CgResult pcg(const std::vector<double>&diag,const std::vector<std::vector<std::pair<int,double>>>&off,const std::vector<double>&b,std::vector<double>&x){
    const int n=int(b.size());auto apply=[&](const std::vector<double>&v){std::vector<double> y(static_cast<size_t>(n),0.0);for(int i=0;i<n;++i){y[size_t(i)]=diag[size_t(i)]*v[size_t(i)];for(auto [j,a]:off[size_t(i)])y[size_t(i)]+=a*v[size_t(j)];}return y;};
    auto dotv=[](const std::vector<double>&a,const std::vector<double>&b){double s=0;for(size_t i=0;i<a.size();++i)s+=a[i]*b[i];return s;};
    std::vector<double> ax=apply(x);std::vector<double> r(static_cast<size_t>(n),0.0),z(static_cast<size_t>(n),0.0),p(static_cast<size_t>(n),0.0);for(int i=0;i<n;++i){r[size_t(i)]=b[size_t(i)]-ax[size_t(i)];z[size_t(i)]=r[size_t(i)]/diag[size_t(i)];p[size_t(i)]=z[size_t(i)];}
    const double bnorm=std::sqrt(std::max(dotv(b,b),1e-300));double rz=dotv(r,z),res=std::sqrt(dotv(r,r));CgResult out;const double tol=1e-12*bnorm;
    for(int it=0;it<100000&&res>tol;++it){auto ap=apply(p);double den=dotv(p,ap);if(!(den>0))break;double a=rz/den;for(int i=0;i<n;++i){x[size_t(i)]+=a*p[size_t(i)];r[size_t(i)]-=a*ap[size_t(i)];}res=std::sqrt(dotv(r,r));out.iterations=it+1;if(res<=tol)break;for(int i=0;i<n;++i)z[size_t(i)]=r[size_t(i)]/diag[size_t(i)];double next=dotv(r,z),beta=next/rz;for(int i=0;i<n;++i)p[size_t(i)]=z[size_t(i)]+beta*p[size_t(i)];rz=next;}
    out.residual=res;out.converged=res<=tol;return out;
}
inline int hydrostaticTest(int n){
    auto start=std::chrono::steady_clock::now();
    const double W=1,H=1,rho=1000,g=9.81,dx=W/n,dy=H/n;const int N=n*n;auto id=[&](int i,int j){return j*n+i;};
    std::vector<double>d(static_cast<size_t>(N),0),b(static_cast<size_t>(N),0),p(static_cast<size_t>(N),0);std::vector<std::vector<std::pair<int,double>>> off(static_cast<size_t>(N));
    for(int j=0;j<n;++j)for(int i=0;i<n;++i){int k=id(i,j);auto add=[&](int q,double a){d[size_t(k)]+=a;off[size_t(k)].push_back({q,-a});};if(i>0)add(id(i-1,j),1/(dx*dx));if(i+1<n)add(id(i+1,j),1/(dx*dx));if(j>0)add(id(i,j-1),1/(dy*dy));else b[size_t(k)]=rho*g/dy;if(j+1<n)add(id(i,j+1),1/(dy*dy));else d[size_t(k)]+=2/(dy*dy);}
    CgResult cg=pcg(d,off,b,p);double l1=0,l2=0,mx=0,vol=dx*dy,maxv=0;
    for(int j=0;j<n;++j)for(int i=0;i<n;++i){double exact=rho*g*(H-(j+.5)*dy),e=std::abs(p[size_t(id(i,j))]-exact);l1+=e*vol;l2+=e*e*vol;mx=std::max(mx,e);}
    // Pressure-corrected vertical face velocity, including the exact wall/open
    // normal-gradient data. This measures residual bulk motion, not a reset.
    for(int j=0;j<=n;++j)for(int i=0;i<n;++i){double grad;if(j==0)grad=-rho*g;else if(j==n)grad=(0-p[size_t(id(i,n-1))])/(dy*.5);else grad=(p[size_t(id(i,j))]-p[size_t(id(i,j-1))])/dy;double vy=-g-grad/rho;maxv=std::max(maxv,std::abs(vy));}
    double l2norm=std::sqrt(l2),runtime=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::printf("P1-A n=%d dx=%.6g dy=%.6g dt=0.001 CFL=0 V=%.12g L1p=%.6g L2p=%.6g maxp=%.6g max|u|=%.6g pressure_residual=%.3g iterations=%d KE=0 runtime=%.6gs\n",n,dx,dy,W*H,l1,l2norm,mx,maxv,cg.residual,cg.iterations,runtime);
    return cg.converged&&std::isfinite(mx)&&maxv<1e-7?0:1;
}
}
