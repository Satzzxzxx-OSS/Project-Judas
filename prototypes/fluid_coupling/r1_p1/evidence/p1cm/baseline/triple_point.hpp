#pragma once
#include "vof.hpp"
#include <array>
#include <limits>

namespace r1p1 {
struct PhaseCell { double liquid{}; double solid{}; };
struct TriplePLIC { Poly solid, fluid, gas; Vec2 solidNormal{}, fluidNormal{}; double solidOffset{}, fluidOffset{}; bool fluidKeepLeft{}; bool triple{}; double objective{}; };

inline Poly clipLine(const Poly& p, Vec2 a, Vec2 b, bool keepLeft=true) {
    Vec2 d=b-a, n{-d.y,d.x};
    return clip(p,n,dot(n,a),keepLeft);
}
inline std::vector<Vec2> lineRectIntersections(Rect c, Vec2 n, double q) {
    std::vector<Vec2> x; const double e=1e-12;
    auto add=[&](Vec2 p){for(auto v:x)if(std::hypot(v.x-p.x,v.y-p.y)<e)return;x.push_back(p);};
    auto edge=[&](Vec2 a,Vec2 b){double da=dot(n,a)-q,db=dot(n,b)-q;if(std::abs(da)<e)add(a);if((da<0&&db>0)||(da>0&&db<0))add(a+(b-a)*(da/(da-db)));};
    edge({c.x0,c.y0},{c.x1,c.y0});edge({c.x1,c.y0},{c.x1,c.y1});edge({c.x1,c.y1},{c.x0,c.y1});edge({c.x0,c.y1},{c.x0,c.y0});
    return x;
}
inline Vec2 youngsNormal(const std::vector<PhaseCell>& a,int nx,int ny,double dx,double dy,int i,int j,bool solid) {
    auto at=[&](int x,int y){x=std::clamp(x,0,nx-1);y=std::clamp(y,0,ny-1);return solid?a[size_t(y*nx+x)].solid:a[size_t(y*nx+x)].liquid;};
    double gx=(at(i+1,j)-at(i-1,j))/(2*dx), gy=(at(i,j+1)-at(i,j-1))/(2*dy), l=std::hypot(gx,gy);
    if(l<1e-12)return {0,1};
    return {-gx/l,-gy/l};
}
inline Poly phasePoly(Rect cell,double fraction,Vec2 n) { return plic(cell,fraction,n); }
inline Poly accessiblePoly(Rect cell,double solidFraction,Vec2 nSolidIn) {
    Poly box=rectangle(cell); if(solidFraction<=1e-14)return box; if(solidFraction>=1-1e-14)return {};
    Poly sp=plic(cell,solidFraction,nSolidIn); // solid lies on the retained side
    if(sp.p.empty())return box;
    double q=-std::numeric_limits<double>::infinity();
    for(auto p:sp.p) q=std::max(q,dot(nSolidIn,p));
    // Fluid is the opposite half-space. For a convex cut by one line, this is
    // exactly the complement of the solid PLIC within the Cartesian cell.
    return clip(box,nSolidIn,q,false);
}
struct CandidateCut { Poly polygon; Vec2 normal{}; double offset{}; bool keepLeft{}; };
inline std::vector<CandidateCut> candidateLiquidCuts(const Poly& accessible,Vec2 z,double targetArea) {
    std::vector<CandidateCut> out;if(accessible.p.size()<3||targetArea<=0||targetArea>=area(accessible))return out;
    for(size_t e=0;e<accessible.p.size();++e){Vec2 a=accessible.p[e],b=accessible.p[(e+1)%accessible.p.size()];
        auto value=[&](double t,bool left){Vec2 x=a+(b-a)*t,d=x-z;if(std::hypot(d.x,d.y)<1e-12)return std::numeric_limits<double>::quiet_NaN();Vec2 n{-d.y,d.x};Poly cut=clip(accessible,n,dot(n,z),left);return area(cut)-targetArea;};
        for(bool left:{true,false}){constexpr int samples=24;double t0=0,v0=value(t0,left);for(int s=1;s<=samples;++s){double t1=double(s)/samples,v1=value(t1,left);if(std::isfinite(v0)&&std::isfinite(v1)&&(v0==0||v1==0||std::signbit(v0)!=std::signbit(v1))){double lo=t0,hi=t1,fl=v0;for(int it=0;it<60;++it){double m=.5*(lo+hi),fm=value(m,left);if(std::abs(fm)<1e-13*std::max(1.0,targetArea)){lo=hi=m;break;}if(std::signbit(fl)==std::signbit(fm)){lo=m;fl=fm;}else hi=m;}double t=.5*(lo+hi);Vec2 x=a+(b-a)*t,d=x-z,n{-d.y,d.x};double q=dot(n,z);Poly poly=clip(accessible,n,q,left);if(!poly.p.empty()){bool duplicate=false;for(auto&r:out)if(std::abs(area(r.polygon)-area(poly))<1e-10&&std::hypot(r.polygon.p.front().x-poly.p.front().x,r.polygon.p.front().y-poly.p.front().y)<1e-8)duplicate=true;if(!duplicate)out.push_back({std::move(poly),n,q,left});}}
            t0=t1;v0=v1;}
        }
    }
    return out;
}
inline double tripleObjective(const std::vector<PhaseCell>& a,int nx,int ny,double dx,double dy,int i,int j,const CandidateCut& liquidCandidate) {
    double g=0;
    for(int y=std::max(0,j-1);y<=std::min(ny-1,j+1);++y)for(int x=std::max(0,i-1);x<=std::min(nx-1,i+1);++x){
        Rect c{x*dx,y*dy,(x+1)*dx,(y+1)*dy};const auto ph=a[size_t(y*nx+x)];
        Vec2 ns=youngsNormal(a,nx,ny,dx,dy,x,y,true);
        Poly fluid=accessiblePoly(c,ph.solid,ns);
        Poly pred=clip(fluid,liquidCandidate.normal,liquidCandidate.offset,liquidCandidate.keepLeft);
        double predF=area(pred)/(dx*dy);
        double predG=std::max(0.0,1.0-ph.solid-predF),actualG=std::max(0.0,1.0-ph.solid-ph.liquid);
        g+=(ph.liquid-predF)*(ph.liquid-predF)+(actualG-predG)*(actualG-predG);
    }
    return g;
}
inline TriplePLIC reconstructTriple(const std::vector<PhaseCell>& a,int nx,int ny,double dx,double dy,int i,int j) {
    Rect c{i*dx,j*dy,(i+1)*dx,(j+1)*dy};TriplePLIC result;result.solidNormal=youngsNormal(a,nx,ny,dx,dy,i,j,true);
    auto solid=phasePoly(c,a[size_t(j*nx+i)].solid,result.solidNormal);result.solid=solid;
    double q=-std::numeric_limits<double>::infinity();for(auto p:solid.p)q=std::max(q,dot(result.solidNormal,p));result.solidOffset=q;
    if(a[size_t(j*nx+i)].solid<=1e-14||a[size_t(j*nx+i)].solid>=1-1e-14){result.fluid=phasePoly(c,a[size_t(j*nx+i)].liquid,youngsNormal(a,nx,ny,dx,dy,i,j,false));result.gas={};return result;}
    Poly accessible=clip(rectangle(c),result.solidNormal,q,false);double target=a[size_t(j*nx+i)].liquid*dx*dy;
    if(target<=1e-14||target>=area(accessible)-1e-14){result.fluid=target<=1e-14?Poly{}:accessible;result.gas={};return result;}
    auto ends=lineRectIntersections(c,result.solidNormal,q);if(ends.size()<2)return result;Vec2 tangent=ends[1]-ends[0];double tl=std::hypot(tangent.x,tangent.y);tangent=tangent*(1.0/tl);Vec2 mid=(ends[0]+ends[1])*.5;
    auto objective=[&](double s){Vec2 z=mid+tangent*s;auto cuts=candidateLiquidCuts(accessible,z,target);double best=std::numeric_limits<double>::infinity();for(auto&cut:cuts)best=std::min(best,tripleObjective(a,nx,ny,dx,dy,i,j,cut));return best;};
    double bestG=std::numeric_limits<double>::infinity(),bestS=0;const double span=9.0*std::max(dx,dy);constexpr double gr=.6180339887498948482;
    for(double branch:{-1.0,1.0}){double lo=branch<0?-span:0,hi=branch<0?0:span;double x1=hi-gr*(hi-lo),x2=lo+gr*(hi-lo),f1=objective(x1),f2=objective(x2);for(int it=0;it<72;++it){if(f1>f2){lo=x1;x1=x2;f1=f2;x2=lo+gr*(hi-lo);f2=objective(x2);}else{hi=x2;x2=x1;f2=f1;x1=hi-gr*(hi-lo);f1=objective(x1);}}double s=.5*(lo+hi),g=objective(s);if(g<bestG){bestG=g;bestS=s;}}
    Vec2 z=mid+tangent*bestS;auto cuts=candidateLiquidCuts(accessible,z,target);double chosen=std::numeric_limits<double>::infinity();for(auto&cut:cuts){double g=tripleObjective(a,nx,ny,dx,dy,i,j,cut);if(g<chosen){chosen=g;result.fluid=cut.polygon;result.fluidNormal=cut.normal;result.fluidOffset=cut.offset;result.fluidKeepLeft=cut.keepLeft;}}
    result.gas=clip(accessible,result.fluidNormal,result.fluidOffset,!result.fluidKeepLeft);result.objective=chosen;result.triple=std::isfinite(chosen);return result;
}
} // namespace r1p1
