#include "vof.hpp"
#include "transport_oracles.hpp"
#include "nonlinear_transport_oracle.hpp"
#include <iomanip>
#include <array>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

namespace r1p1 {
struct Grid {
    int nx,ny; double dx,dy;
    std::vector<double> c,u,v;
    double inflowLeft{},inflowRight{},inflowBottom{},inflowTop{};
    bool periodicX=false,periodicY=false;
    Grid(int x,int y,double hx,double hy):nx(x),ny(y),dx(hx),dy(hy),c(size_t(x*y),0),u(size_t((x+1)*y),0),v(size_t(x*(y+1)),0){}
    size_t C(int i,int j)const{return size_t(j*nx+i);}
    size_t U(int i,int j)const{return size_t(j*(nx+1)+i);}
    size_t V(int i,int j)const{return size_t(j*nx+i);}
    double volume()const{double q=0;for(double a:c)q+=a*dx*dy;return q;}
};
enum class Axis { X, Y };
struct Reconstruction {std::vector<Poly> polygons;std::vector<double> alpha;};
struct StageStats {
    std::string test,stage;int step{};double v0{},v1{},minimum{},maximum{},maxFlux{},massResidual{},l1{};int outside{},invalid{};int outsideI{-1},outsideJ{-1};double outsideAlpha{};int badI{-1},badJ{-1};double badAlpha{};
};
struct StepStats {
    StageStats predictorX,predictorY,orderXY,orderYX,final;
    bool valid{}; std::string failure;
    double orderAverageError{},divergenceAdjustedIdentityError{},boundary{},
        cflX{},cflY{},cflSum{},maxDiv{},dtDiv{},volumeBefore{};
};
using ExactField=std::function<double(double,double)>;

static Vec2 normalAt(const Grid&g,const std::vector<double>&a,int i,int j){
    auto at=[&](int x,int y){x=g.periodicX?(x%g.nx+g.nx)%g.nx:std::clamp(x,0,g.nx-1);y=g.periodicY?(y%g.ny+g.ny)%g.ny:std::clamp(y,0,g.ny-1);return a[g.C(x,y)];};
    double gx=(at(i+1,j)-at(i-1,j))/(2*g.dx),gy=(at(i,j+1)-at(i,j-1))/(2*g.dy),m=std::hypot(gx,gy);
    return m>1e-14?Vec2{-gx/m,-gy/m}:Vec2{0,-1};
}
static constexpr double roundoffAlphaTolerance=256*std::numeric_limits<double>::epsilon();
static bool bounded(const Grid&,const std::vector<double>&a){for(double q:a)if(!std::isfinite(q)||q < -roundoffAlphaTolerance||q>1+roundoffAlphaTolerance)return false;return true;}
static Reconstruction reconstruct(const Grid&g,const std::vector<double>&a){
    Reconstruction r;r.polygons.resize(a.size());r.alpha=a;
    if(!bounded(g,a))return r;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){Vec2 normal=normalAt(g,a,i,j);r.polygons[g.C(i,j)]=plic({0,0,1,1},a[g.C(i,j)],{normal.x*g.dx,normal.y*g.dy});}
    return r;
}
static double geometricFaceFlux(const Grid&g,const Reconstruction&r,Axis axis,int f,int t,double dt){
    const bool x=axis==Axis::X;double speed=x?g.u[g.U(f,t)]:g.v[g.V(t,f)];if(speed==0)return 0;
    int donor=-1;double boundaryAlpha=0;bool exteriorDonor=false;
    if((x&&g.periodicX)||(!x&&g.periodicY)){
        int count=x?g.nx:g.ny;donor=((speed>0?f-1:f)%count+count)%count;
    }else if(x){
        if(f==0){if(speed<0)donor=0;else{exteriorDonor=true;boundaryAlpha=g.inflowLeft;}}
        else if(f==g.nx){if(speed>0)donor=g.nx-1;else{exteriorDonor=true;boundaryAlpha=g.inflowRight;}}
        else donor=speed>0?f-1:f;
    }else{
        if(f==0){if(speed<0)donor=0;else{exteriorDonor=true;boundaryAlpha=g.inflowBottom;}}
        else if(f==g.ny){if(speed>0)donor=g.ny-1;else{exteriorDonor=true;boundaryAlpha=g.inflowTop;}}
        else donor=speed>0?f-1:f;
    }
    double h=x?g.dx:g.dy,d=std::abs(speed)*dt;
    if(d>h)return std::numeric_limits<double>::quiet_NaN();
    if(exteriorDonor){if(boundaryAlpha==0)return 0;if(boundaryAlpha==1)return speed*dt*(x?g.dy:g.dx);return std::copysign(boundaryAlpha*d*(x?g.dy:g.dx),speed);}
    if(donor<0)return std::numeric_limits<double>::quiet_NaN();
    int i=x?donor:t,j=x?t:donor;
    size_t donorIndex=g.C(i,j);double donorAlpha=r.alpha[donorIndex];if(donorAlpha==0)return 0;if(donorAlpha==1)return speed*dt*(x?g.dy:g.dx);
    // Geometry is stored in the donor cell unit square, avoiding origin-dependent cancellation.
    Rect cell{0,0,1,1};d/=h;const Poly&p=r.polygons[donorIndex];double swept=0;
    if(x)swept=speed>0?areaInStrip(p,true,cell.x1-d,cell.x1):areaInStrip(p,true,cell.x0,cell.x0+d);
    else swept=speed>0?areaInStrip(p,false,cell.y1-d,cell.y1):areaInStrip(p,false,cell.y0,cell.y0+d);
    return std::copysign(swept*(g.dx*g.dy),speed);
}
struct Fluxes {std::vector<double>x,y;};
static Fluxes directionalFluxes(const Grid&g,const Reconstruction&r,Axis axis,double dt,double&maxAbs){
    Fluxes f;maxAbs=0;
    if(axis==Axis::X){f.x.assign(g.u.size(),0);for(int j=0;j<g.ny;++j)for(int i=0;i<=g.nx;++i){double q=geometricFaceFlux(g,r,axis,i,j,dt);f.x[g.U(i,j)]=q;if(!std::isfinite(q))maxAbs=std::numeric_limits<double>::infinity();else maxAbs=std::max(maxAbs,std::abs(q));}}
    else {f.y.assign(g.v.size(),0);for(int j=0;j<=g.ny;++j)for(int i=0;i<g.nx;++i){double q=geometricFaceFlux(g,r,axis,j,i,dt);f.y[g.V(i,j)]=q;if(!std::isfinite(q))maxAbs=std::numeric_limits<double>::infinity();else maxAbs=std::max(maxAbs,std::abs(q));}}
    return f;
}
static double l1Error(const Grid&g,const std::vector<double>&a,const ExactField&exact){if(!exact)return -1;double e=0;for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i)e+=std::abs(a[g.C(i,j)]-exact((i+.5)*g.dx,(j+.5)*g.dy))*g.dx*g.dy;return e;}
static StageStats inspect(const Grid&g,const std::vector<double>&a,const std::string&name,const std::string&stage,int step,double v0,double maxFlux,double residual,const ExactField&exact){
    StageStats s;s.test=name;s.stage=stage;s.step=step;s.v0=v0;s.v1=0;s.minimum=std::numeric_limits<double>::infinity();s.maximum=-s.minimum;s.maxFlux=maxFlux;s.massResidual=residual;s.l1=l1Error(g,a,exact);
    for(size_t k=0;k<a.size();++k){double q=a[k];s.minimum=std::min(s.minimum,q);s.maximum=std::max(s.maximum,q);s.v1+=q*g.dx*g.dy;bool oob=!std::isfinite(q)||q<0||q>1;bool severe=!std::isfinite(q)||q < -roundoffAlphaTolerance||q>1+roundoffAlphaTolerance;if(oob){++s.outside;if(s.outsideI<0){s.outsideI=int(k%size_t(g.nx));s.outsideJ=int(k/size_t(g.nx));s.outsideAlpha=q;}}if(severe){++s.invalid;if(s.badI<0){s.badI=int(k%size_t(g.nx));s.badJ=int(k/size_t(g.nx));s.badAlpha=q;}}}
    return s;
}
static void writeTrace(std::ofstream&csv,const StageStats&s){csv<<s.test<<','<<s.stage<<','<<s.step<<','<<s.v0<<','<<s.v1<<','<<s.minimum<<','<<s.maximum<<','<<s.maxFlux<<','<<s.outside<<','<<s.invalid<<','<<s.massResidual<<','<<s.l1<<','<<s.outsideI<<','<<s.outsideJ<<','<<s.outsideAlpha<<','<<s.badI<<','<<s.badJ<<','<<s.badAlpha<<'\n';}
static std::vector<double> predictor(const Grid&g,const std::vector<double>&base,const std::vector<double>&frozenColour,const Fluxes&f,Axis a,double dt,double&identityResidual){
    std::vector<double>out(base.size());double cell=g.dx*g.dy,v0=0,v1=0,dilation=0;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){double net=0;
        double divDt=0;
        if(a==Axis::X){double left=g.u[g.U(i,j)]*dt*g.dy,right=g.u[g.U(i+1,j)]*dt*g.dy;net=f.x[g.U(i+1,j)]-f.x[g.U(i,j)];divDt=(right-left)/cell;}
        else {double bottom=g.v[g.V(i,j)]*dt*g.dx,top=g.v[g.V(i,j+1)]*dt*g.dx;net=f.y[g.V(i,j+1)]-f.y[g.V(i,j)];divDt=(top-bottom)/cell;}
        double delta=-net/cell+frozenColour[g.C(i,j)]*divDt;out[g.C(i,j)]=base[g.C(i,j)]+delta;
        v0+=base[g.C(i,j)]*cell;v1+=out[g.C(i,j)]*cell;dilation+=frozenColour[g.C(i,j)]*divDt*cell;
    }
    double boundary=0;
    if(a==Axis::X)for(int j=0;j<g.ny;++j)boundary+=f.x[g.U(g.nx,j)]-f.x[g.U(0,j)];
    else for(int i=0;i<g.nx;++i)boundary+=f.y[g.V(i,g.ny)]-f.y[g.V(i,0)];
    identityResidual=(v1-v0)+boundary-dilation;return out;
}
static double boundaryFlux(const Grid&g,const Fluxes&fx,const Fluxes&fy){double b=0;for(int j=0;j<g.ny;++j)b+=fx.x[g.U(g.nx,j)]-fx.x[g.U(0,j)];for(int i=0;i<g.nx;++i)b+=fy.y[g.V(i,g.ny)]-fy.y[g.V(i,0)];return b;}
static std::vector<double> finalUpdate(const Grid&g,const std::vector<double>&base,const Fluxes&fx,const Fluxes&fy,double&residual){
    std::vector<double>out(base.size());double cell=g.dx*g.dy,v0=0,v1=0;
    for(size_t k=0;k<base.size();++k)v0+=base[k]*cell;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){double nx=fx.x[g.U(i+1,j)]-fx.x[g.U(i,j)],ny=fy.y[g.V(i,j+1)]-fy.y[g.V(i,j)];out[g.C(i,j)]=base[g.C(i,j)]-(nx+ny)/cell;}
    for(size_t k=0;k<out.size();++k)v1+=out[k]*cell;
    residual=(v1-v0)+boundaryFlux(g,fx,fy);return out;
}
// No limiter or repair is permitted. Diagnose the first rejected operation in full.
static void printPredictorFailure(const Grid& g,const std::vector<double>& base,
    const std::vector<double>& out,const std::vector<double>& frozenColour,const Fluxes& flux,Axis axis,double dt,
    const StageStats& stats) {
    const double cell=g.dx*g.dy;
    const int i=stats.badI,j=stats.badJ;
    auto detail=[&](int x,int y) {
        if(x<0||x>=g.nx||y<0||y>=g.ny)return;
        size_t k=g.C(x,y);double left,right,wl,wr;
        if(axis==Axis::X){left=flux.x[g.U(x,y)];right=flux.x[g.U(x+1,y)];wl=g.u[g.U(x,y)];wr=g.u[g.U(x+1,y)];}
        else {left=flux.y[g.V(x,y)];right=flux.y[g.V(x,y+1)];wl=g.v[g.V(x,y)];wr=g.v[g.V(x,y+1)];}
        double transverse=axis==Axis::X?g.dy:g.dx;
        double divDt=(wr*dt*transverse-wl*dt*transverse)/cell,advected=base[k]-(right-left)/cell;
        std::fprintf(stderr,"  cell=(%d,%d) base_alpha=%.17g Fminus=%.17g Fplus=%.17g wminus=%.17g wplus=%.17g divDt=%.17g conservative_part=%.17g frozen_colour=%.17g dilation=%.17g predictor=%.17g\n",
          x,y,base[k],left,right,wl,wr,divDt,advected,frozenColour[k],frozenColour[k]*divDt,out[k]);
    };
    std::fprintf(stderr,"FIRST MATERIAL INVALID: test=%s step=%d operation=%s cell=(%d,%d) alpha=%.17g tolerance=%.17g\n",stats.test.c_str(),stats.step,stats.stage.c_str(),i,j,stats.badAlpha,roundoffAlphaTolerance);
    for(int y=j-1;y<=j+1;++y)for(int x=i-1;x<=i+1;++x)detail(x,y);
}

// These are the actual stored phase-volume transfers, not a second reconstruction.
struct TransportRecord {
    std::vector<double> cn,frozen,cx,cy,xThenY,yThenX,cfinal;
    Fluxes fxn,fyn,fxAfterY,fyAfterX;
};
// JSGT = symmetric nonlinear geometric flux averaging plus frozen WY predictors.
// This intentionally differs from van der Eijk-Wellens Eqs. 14 AND 15.
static StepStats jsgtStep(Grid&g,double dt,const std::string&test,int step,
    const ExactField&exactX,const ExactField&exactY,const ExactField&exactFinal,
    std::ofstream&csv,const std::vector<std::pair<int,int>>&probes,TransportRecord* record=nullptr){
    StepStats result; double v0=g.volume(),maxFx=0,maxFy=0,resX=0,resY=0;
    result.volumeBefore=v0;
    for(double u:g.u)result.cflX=std::max(result.cflX,std::abs(u)*dt/g.dx);
    for(double v:g.v)result.cflY=std::max(result.cflY,std::abs(v)*dt/g.dy);
    result.cflSum=result.cflX+result.cflY;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){
        double divergence=(g.u[g.U(i+1,j)]-g.u[g.U(i,j)])/g.dx
                         +(g.v[g.V(i,j+1)]-g.v[g.V(i,j)])/g.dy;
        result.maxDiv=std::max(result.maxDiv,std::abs(divergence));
    }
    result.dtDiv=dt*result.maxDiv;
    // A sufficient published-style restriction, not a sharp stability bound.
    if(!(result.cflSum<.5)){
        result.failure="cumulative_courant";
        std::fprintf(stderr,"STOP %s step %d: cumulative Courant %.17g is not < 0.5\n",test.c_str(),step,result.cflSum);
        return result;
    }
    if(!bounded(g,g.c)){result.failure="initial_fraction";return result;}
    // Freeze once, including the exact tie convention. No predictor reclassification.
    const std::vector<double> frozenColour=[&](){
        std::vector<double> c(g.c.size());
        for(size_t k=0;k<c.size();++k)c[k]=g.c[k]>.5?1.:0.;
        return c;
    }();
    auto acceptStage=[&](const StageStats&s){
        if(s.invalid){result.failure=s.stage;return false;}
        if(!std::isfinite(s.massResidual)||std::abs(s.massResidual)>=1e-11){
            result.failure=s.stage+"_budget";
            std::fprintf(stderr,"STOP %s step %d operation=%s residual=%.17g\n",test.c_str(),step,result.failure.c_str(),s.massResidual);
            return false;
        }
        return true;
    };
    Reconstruction rn=reconstruct(g,g.c);
    Fluxes fluxXn=directionalFluxes(g,rn,Axis::X,dt,maxFx),fluxYn=directionalFluxes(g,rn,Axis::Y,dt,maxFy);
    if(!std::isfinite(maxFx)||!std::isfinite(maxFy)){result.failure="original_face_flux";return result;}
    auto cx=predictor(g,g.c,frozenColour,fluxXn,Axis::X,dt,resX);
    result.predictorX=inspect(g,cx,test,"predictor_x",step,v0,maxFx,resX,exactX);writeTrace(csv,result.predictorX);
    if(!acceptStage(result.predictorX)){
        if(result.predictorX.invalid)printPredictorFailure(g,g.c,cx,frozenColour,fluxXn,Axis::X,dt,result.predictorX);
        return result;
    }
    auto cy=predictor(g,g.c,frozenColour,fluxYn,Axis::Y,dt,resY);
    result.predictorY=inspect(g,cy,test,"predictor_y",step,v0,maxFy,resY,exactY);writeTrace(csv,result.predictorY);
    if(!acceptStage(result.predictorY)){
        if(result.predictorY.invalid)printPredictorFailure(g,g.c,cy,frozenColour,fluxYn,Axis::Y,dt,result.predictorY);
        return result;
    }
    // Optional cell probes only emit evidence and never participate in the update.
    for(auto [i,j]:probes){
        const size_t k=g.C(i,j);const double area=g.dx*g.dy;
        double minus=fluxYn.y[g.V(i,j)],plus=fluxYn.y[g.V(i,j+1)];
        double dm=g.v[g.V(i,j)]*dt*g.dx,dp=g.v[g.V(i,j+1)]*dt*g.dx,d=(dp-dm)/area;
        std::printf("PROBE %s step=%d cell=(%d,%d) Cn=%.17g c=%.17g area=%.17g y_Fminus=%.17g y_Fplus=%.17g dy=%.17g conservative=%.17g correction=%.17g X=%.17g Y=%.17g\n",
          test.c_str(),step,i,j,g.c[k],frozenColour[k],area,minus,plus,d,g.c[k]-(plus-minus)/area,frozenColour[k]*d,cx[k],cy[k]);
    }
    // Reconstruct each predictor separately. Never reconstruct averaged fractions.
    Reconstruction rcx=reconstruct(g,cx),rcy=reconstruct(g,cy);
    double maxCrossX=0,maxCrossY=0;
    Fluxes crossX=directionalFluxes(g,rcy,Axis::X,dt,maxCrossX),crossY=directionalFluxes(g,rcx,Axis::Y,dt,maxCrossY);
    if(!std::isfinite(maxCrossX)||!std::isfinite(maxCrossY)){result.failure="transverse_face_flux";return result;}
    // Explicit full WY orders, diagnostic only. They share the ORIGINAL coefficient.
    double budgetXY=0,budgetYX=0;
    auto xy=predictor(g,cy,frozenColour,crossX,Axis::X,dt,budgetXY);
    auto yx=predictor(g,cx,frozenColour,crossY,Axis::Y,dt,budgetYX);
    result.orderXY=inspect(g,xy,test,"wy_x_after_y",step,result.predictorY.v1,maxCrossX,budgetXY,exactFinal);writeTrace(csv,result.orderXY);
    if(!acceptStage(result.orderXY)){
        if(result.orderXY.invalid)printPredictorFailure(g,cy,xy,frozenColour,crossX,Axis::X,dt,result.orderXY);
        return result;
    }
    result.orderYX=inspect(g,yx,test,"wy_y_after_x",step,result.predictorX.v1,maxCrossY,budgetYX,exactFinal);writeTrace(csv,result.orderYX);
    if(!acceptStage(result.orderYX)){
        if(result.orderYX.invalid)printPredictorFailure(g,cx,yx,frozenColour,crossY,Axis::Y,dt,result.orderYX);
        return result;
    }
    Fluxes finalX=fluxXn,finalY=fluxYn;double maxFinal=0;
    for(size_t k=0;k<finalX.x.size();++k){finalX.x[k]=.5*(fluxXn.x[k]+crossX.x[k]);maxFinal=std::max(maxFinal,std::abs(finalX.x[k]));}
    for(size_t k=0;k<finalY.y.size();++k){finalY.y[k]=.5*(fluxYn.y[k]+crossY.y[k]);maxFinal=std::max(maxFinal,std::abs(finalY.y[k]));}
    double residual=0;auto next=finalUpdate(g,g.c,finalX,finalY,residual);
    result.boundary=boundaryFlux(g,finalX,finalY); // Positive = liquid leaving domain.
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){
        size_t k=g.C(i,j);double defect=.5*(xy[k]+yx[k])-next[k];
        double dx=(g.u[g.U(i+1,j)]*dt*g.dy-g.u[g.U(i,j)]*dt*g.dy)/(g.dx*g.dy);
        double dy=(g.v[g.V(i,j+1)]*dt*g.dx-g.v[g.V(i,j)]*dt*g.dx)/(g.dx*g.dy);
        result.orderAverageError=std::max(result.orderAverageError,std::abs(defect));
        result.divergenceAdjustedIdentityError=std::max(result.divergenceAdjustedIdentityError,std::abs(defect-frozenColour[k]*(dx+dy)));
    }
    result.final=inspect(g,next,test,"jsgt_final",step,v0,maxFinal,residual,exactFinal);writeTrace(csv,result.final);
    if(!acceptStage(result.final)){
        if(result.final.invalid){int i=result.final.badI,j=result.final.badJ;size_t k=g.C(i,j);
            std::fprintf(stderr,"FIRST MATERIAL INVALID: test=%s step=%d operation=jsgt_final cell=(%d,%d) Cn=%.17g Cx=%.17g Cy=%.17g xL=%.17g xR=%.17g yB=%.17g yT=%.17g result=%.17g\n",test.c_str(),step,i,j,g.c[k],cx[k],cy[k],finalX.x[g.U(i,j)],finalX.x[g.U(i+1,j)],finalY.y[g.V(i,j)],finalY.y[g.V(i,j+1)],next[k]);
        }
        return result;
    }
    if(result.dtDiv>roundoffAlphaTolerance||result.orderAverageError>roundoffAlphaTolerance||result.divergenceAdjustedIdentityError>roundoffAlphaTolerance){
        result.failure="divergence_or_order_identity";
        std::fprintf(stderr,"STOP %s step %d: dt_div=%.17g mean_defect=%.17g adjusted_defect=%.17g\n",test.c_str(),step,result.dtDiv,result.orderAverageError,result.divergenceAdjustedIdentityError);
        return result;
    }
    if(record){
        record->cn=g.c;record->frozen=frozenColour;record->cx=cx;record->cy=cy;
        record->xThenY=yx;record->yThenX=xy;record->cfinal=next;
        record->fxn=fluxXn;record->fyn=fluxYn;record->fxAfterY=crossX;record->fyAfterX=crossY;
    }
    g.c.swap(next);result.valid=true;return result;
}

#include "mass_momentum.hpp"

// Reference only: the former sequential Weymouth-Yue split. It is deliberately
// not called by any acceptance fixture. The binary correction field is frozen
// from C^n, then the x-updated field is reconstructed before the y sweep.
[[maybe_unused]] static std::vector<double> wySequentialReference(const Grid&g,double dt){
    std::vector<double>binary(g.c.size());for(size_t k=0;k<g.c.size();++k)binary[k]=g.c[k]>.5?1.0:0.0;
    Reconstruction rn=reconstruct(g,g.c);double m=0;Fluxes fx=directionalFluxes(g,rn,Axis::X,dt,m);
    if(!std::isfinite(m))return {};
    std::vector<double>cx(g.c.size());double cell=g.dx*g.dy;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){size_t k=g.C(i,j);double div=(g.u[g.U(i+1,j)]-g.u[g.U(i,j)])/g.dx;cx[k]=g.c[k]-(fx.x[g.U(i+1,j)]-fx.x[g.U(i,j)])/cell+binary[k]*div*dt;}
    if(!bounded(g,cx))return {};
    Reconstruction rx=reconstruct(g,cx);Fluxes fy=directionalFluxes(g,rx,Axis::Y,dt,m);if(!std::isfinite(m))return {};
    std::vector<double>out(cx.size());
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){size_t k=g.C(i,j);double div=(g.v[g.V(i,j+1)]-g.v[g.V(i,j)])/g.dy;out[k]=cx[k]-(fy.y[g.V(i,j+1)]-fy.y[g.V(i,j)])/cell+binary[k]*div*dt;}
    return out;
}

// Acceptance oracles live in a separate header and never call production PLIC,
// half-plane clipping, area(), areaInStrip(), or the transport implementation.
using Reference=std::function<double(int,int)>;
static ExactField asField(const Grid&g,Reference r){return [&g,r](double x,double y){return r(int(x/g.dx),int(y/g.dy));};}
static Reference rectangleReference(int n,oracle::Box r){return [n,r](int i,int j){return oracle::rectangleFraction(n,i,j,r);};}
static Reference polygonReference(int n,oracle::Polygon p){return [n,p](int i,int j){return oracle::polygonFraction(n,i,j,p);};}
static Reference slotReference(int n,double sx,double sy,double theta){return [=](int i,int j){return oracle::slotFraction(n,i,j,sx,sy,theta);};}
static void initialize(Grid&g,const Reference&r){for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i)g.c[g.C(i,j)]=r(i,j);}
static void setUniformVelocity(Grid&g,double ux,double uy){std::fill(g.u.begin(),g.u.end(),ux);std::fill(g.v.begin(),g.v.end(),uy);}
static double maxCourant(const Grid&g,double dt){double q=0;for(double u:g.u)q=std::max(q,std::abs(u)*dt/g.dx);for(double v:g.v)q=std::max(q,std::abs(v)*dt/g.dy);return q;}
static double maxDivergence(const Grid&g,bool directional){double q=0;for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){double dx=(g.u[g.U(i+1,j)]-g.u[g.U(i,j)])/g.dx,dy=(g.v[g.V(i,j+1)]-g.v[g.V(i,j)])/g.dy;q=std::max(q,directional?std::max(std::abs(dx),std::abs(dy)):std::abs(dx+dy));}return q;}
static void csvHeader(std::ofstream&f){f<<"test,stage,step,volume_before,volume_after,min_alpha,max_alpha,max_abs_face_flux,outside_0_1_cells,beyond_roundoff_cells,conservation_or_update_residual,L1_geometric_error,first_outside_i,first_outside_j,first_outside_alpha,first_material_invalid_i,first_material_invalid_j,first_material_invalid_alpha\n";}
struct RunSummary {bool pass{};int completed{};double l1{-1},lo{1},hi{},maxResidual{},maxOrderError{},runtime{},volumeError{},finalVolume{},exportedVolume{};};
using VelocitySetter=std::function<void(Grid&,double)>;
using ReferenceAt=std::function<Reference(double)>;
static double cumulativeCourant(const Grid&g,double dt){
    double ux=0,vy=0;for(double u:g.u)ux=std::max(ux,std::abs(u));for(double v:g.v)vy=std::max(vy,std::abs(v));
    return dt*(ux/g.dx+vy/g.dy);
}
static void stepHeader(std::ofstream&f){
    f<<"test,step,dt,courant_x,courant_y,courant_sum,max_total_divergence,dt_max_total_divergence,min_X,max_X,min_Y,max_Y,min_WxY,max_WxY,min_WyX,max_WyX,min_final,max_final,volume_before,volume_after,outward_boundary_volume,update_residual,order_mean_error,divergence_adjusted_identity_error,accepted,failure\n";
}
static void writeStep(std::ofstream&f,const std::string&name,int step,double dt,const StepStats&s){
    f<<name<<','<<step<<','<<dt<<','<<s.cflX<<','<<s.cflY<<','<<s.cflSum<<','<<s.maxDiv<<','<<s.dtDiv;
    for(const StageStats*p:{&s.predictorX,&s.predictorY,&s.orderXY,&s.orderYX,&s.final}){
        if(p->stage.empty())f<<",,";else f<<','<<p->minimum<<','<<p->maximum;
    }
    f<<','<<s.volumeBefore<<',';
    if(!s.final.stage.empty())f<<s.final.v1;
    f<<','<<s.boundary<<',';
    if(!s.final.stage.empty())f<<s.final.massResidual;
    f<<','<<s.orderAverageError<<','<<s.divergenceAdjustedIdentityError<<','<<s.valid<<','<<s.failure<<'\n';
}
static RunSummary runCase(int n,const std::string&name,double T,int requestedSteps,
    VelocitySetter velocity,ReferenceAt exact,std::ofstream&trace,std::ofstream&summary,std::ofstream&stepLog,
    bool constant=false,double constantValue=0,double errorLimit=.1,
    const std::vector<std::pair<int,int>>&probes={}){
    auto start=std::chrono::steady_clock::now();Grid g(n,n,1.0/n,1.0/n);initialize(g,exact(0));
    if(constant)g.inflowLeft=g.inflowRight=g.inflowBottom=g.inflowTop=constantValue;
    // Uniform halving policy preserves fixture geometry, duration and reversal boundary.
    // It is only a sufficient timestep selection, not a fraction limiter.
    velocity(g,0);int steps=requestedSteps;
    while(!(cumulativeCourant(g,T/steps)<.5))steps*=2;
    MomentumState momentum(g,1000.,1.);
    std::array<std::vector<double>,2> initialW;
    for(int q=0;q<2;++q)initialW[q].assign(momentum.layout[q].cells.size(),q==0?.73:-.42);
    momentum.boundaryVelocity={{.73,-.42}};momentum.initialize(g,initialW);
    std::filesystem::create_directories("evidence/p1cm/current");
    std::ofstream momentumLog("evidence/p1cm/current/fraction_regression_momentum.csv",std::ios::app);
    momentumLog<<std::setprecision(17);
    RunSummary result;double initialVolume=g.volume(),dt=T/steps,maxCFL=0,maxSum=0,maxDiv=0,maxDirDiv=0;
    double totalBoundary=0,maxGlobalBudget=0,maxAdjusted=0;
    bool valid=true;int failedStep=-1;std::string failedStage;
    for(int k=0;k<steps;++k){
        velocity(g,k*dt);maxCFL=std::max(maxCFL,maxCourant(g,dt));maxSum=std::max(maxSum,cumulativeCourant(g,dt));
        maxDiv=std::max(maxDiv,maxDivergence(g,false));maxDirDiv=std::max(maxDirDiv,maxDivergence(g,true));
        TransportRecord record;
        auto stats=jsgtStep(g,dt,name,k,{}, {},asField(g,exact((k+1)*dt)),trace,k==0?probes:std::vector<std::pair<int,int>>{},&record);
        if(stats.valid){
            auto md=advanceMomentum(g,record,dt,momentum);
            momentumLog<<name<<','<<n<<','<<k<<','<<md.fineMassRaw<<','<<md.fineMassNorm<<','<<md.dualMassRaw<<','<<md.dualMassNorm<<','<<md.combinationMassNorm<<','<<md.massResidualNorm<<','<<md.momentumResidualNorm[0]<<','<<md.momentumResidualNorm[1]<<','<<md.commonVectorError[0]<<','<<md.commonVectorError[1]<<','<<md.kineticBefore<<','<<md.kineticAfter<<','<<md.combinationEnergyNorm<<','<<md.fullStepEnergyGrowthNorm<<','<<md.massResidualRaw<<','<<md.momentumResidualRaw[0]<<','<<md.momentumResidualRaw[1]<<','<<md.combinationMassRaw<<','<<md.combinationEnergyRaw<<','<<md.meanBranchEnergy<<','<<md.velocityMin[0]<<','<<md.velocityMax[0]<<','<<md.velocityMin[1]<<','<<md.velocityMax[1]<<','<<md.minMass<<','<<md.massSource<<','<<md.pSource[0]<<','<<md.pSource[1]<<','<<md.valid<<','<<md.failure<<'\n';
            if(!md.valid){std::fprintf(stderr,"P1-C-M regression STOP: %s step=%d %s fine=%.17g dual=%.17g combination=%.17g\n",name.c_str(),k,md.failure.c_str(),md.fineMassNorm,md.dualMassNorm,md.combinationMassNorm);stats.valid=false;stats.failure=md.failure;}
        }
        writeStep(stepLog,name,k,dt,stats);
        for(const StageStats* st:{&stats.predictorX,&stats.predictorY,&stats.orderXY,&stats.orderYX,&stats.final})if(!st->stage.empty()){
            result.lo=std::min(result.lo,st->minimum);result.hi=std::max(result.hi,st->maximum);result.maxResidual=std::max(result.maxResidual,std::abs(st->massResidual));
        }
        result.maxOrderError=std::max(result.maxOrderError,stats.orderAverageError);
        maxAdjusted=std::max(maxAdjusted,stats.divergenceAdjustedIdentityError);
        if(!stats.valid){valid=false;failedStep=k;failedStage=stats.failure;break;}
        ++result.completed;totalBoundary+=stats.boundary;
        maxGlobalBudget=std::max(maxGlobalBudget,std::abs(g.volume()-initialVolume+totalBoundary));
        if(maxGlobalBudget>=1e-11){valid=false;failedStep=k;failedStage="cumulative_volume_budget";break;}
    }
    result.runtime=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    result.finalVolume=g.volume();result.exportedVolume=totalBoundary;
    result.volumeError=std::abs(g.volume()-initialVolume+totalBoundary);
    if(valid)result.l1=l1Error(g,g.c,asField(g,exact(T)));
    bool unchanged=true;if(constant)for(double c:g.c)unchanged=unchanged&&c==constantValue;
    // Existing bounds, conservation, identity and geometric error gates remain unchanged.
    result.pass=valid&&bounded(g,g.c)&&result.volumeError<1e-11&&result.maxResidual<1e-11&&result.maxOrderError<roundoffAlphaTolerance&&result.l1<errorLimit&&(!constant||unchanged);
    std::printf("%s n=%d completed=%d/%d initial_steps=%d dt=%.17g CFL=%.9g cumulative_CFL=%.9g boundary=%.17g volume_error=%.17g residual=%.17g min=%.17g max=%.17g L1=%.17g order_identity_error=%.17g div=%.17g directional_div=%.17g runtime=%.6gs %s",
        name.c_str(),n,result.completed,steps,requestedSteps,dt,maxCFL,maxSum,totalBoundary,result.volumeError,result.maxResidual,result.lo,result.hi,result.l1,result.maxOrderError,maxDiv,maxDirDiv,result.runtime,result.pass?"PASS":"FAIL");
    if(!valid)std::printf(" first_failed_step=%d operation=%s (candidate rejected; no final-time L1)",failedStep,failedStage.c_str());
    std::printf("\n");
    summary<<name<<','<<n<<','<<dt<<','<<maxCFL<<','<<maxSum<<','<<result.completed<<','<<steps<<','<<requestedSteps<<','<<initialVolume<<','<<g.volume()<<','<<totalBoundary<<','<<result.volumeError<<','<<maxGlobalBudget<<','<<result.maxResidual<<','<<result.lo<<','<<result.hi<<','<<result.l1<<','<<result.maxOrderError<<','<<maxAdjusted<<','<<maxDiv<<','<<maxDirDiv<<','<<result.runtime<<','<<(result.pass?"PASS":"FAIL")<<','<<failedStep<<','<<failedStage<<'\n';
    return result;
}
static int stepCount(int n,double T,double speed,double cfl){return int(std::ceil(T*speed*n/cfl));}
static int run(){
    std::setvbuf(stdout,nullptr,_IONBF,0); // Preserve diagnostic order in captured logs.
    std::filesystem::create_directories("evidence/p1cm/current");
    {std::ofstream f("evidence/p1cm/current/fraction_regression_momentum.csv");f<<"case,n,step,fine_mass_raw,fine_mass_norm,dual_mass_raw,dual_mass_norm,combination_mass_norm,mass_budget_norm,px_budget_norm,py_budget_norm,common_x_error,common_y_error,energy_before,energy_after,combination_energy_norm,full_step_energy_growth_norm,mass_budget_raw,px_budget_raw,py_budget_raw,combination_mass_raw,combination_energy_raw,mean_branch_energy,vx_min,vx_max,vy_min,vy_max,min_mass,mass_source,px_source,py_source,valid,failure\n";}
    // Invoked with the prototype directory as cwd: all artifacts remain isolated.
    std::filesystem::create_directories("results");std::ofstream trace("results/jsgt_trace.csv"),summary("results/jsgt_summary.csv"),stepLog("results/jsgt_steps.csv");trace<<std::setprecision(17);summary<<std::setprecision(17);stepLog<<std::setprecision(17);csvHeader(trace);stepHeader(stepLog);
    summary<<"test,n,dt,CFL,cumulative_CFL,completed_steps,steps,initial_requested_steps,initial_volume,final_volume,outward_boundary_volume,accepted_state_volume_error,max_cumulative_volume_budget,max_stage_residual,min_alpha,max_alpha,final_L1,order_average_identity_error,divergence_adjusted_identity_error,max_total_divergence,max_directional_divergence,runtime_s,status,first_failed_step,first_failed_operation\n";
    auto uniform=[](Grid&g,double){setUniformVelocity(g,.17,-.11);};
    for(double value:{0.,1.}){ReferenceAt exact=[value](double){return [value](int,int){return value;};};
        if(!runCase(24,value==0?"constant_gas":"constant_liquid",.1,3,uniform,exact,trace,summary,stepLog,true,value).pass)return 1;}
    auto refinement=[&](const std::string&name,auto factory,double T,double speed,VelocitySetter velocity,double errorLimit){
        double previous=std::numeric_limits<double>::infinity();
        for(int n:{24,48,96}){auto r=runCase(n,name,T,stepCount(n,T,speed,.42),velocity,factory(n),trace,summary,stepLog,false,0,errorLimit);if(!r.pass){std::fprintf(stderr,"P1-C STOP: %s n=%d failed\n",name.c_str(),n);return false;}if(!(r.l1<previous)){std::fprintf(stderr,"P1-C STOP: %s error did not decrease: %.17g >= %.17g\n",name.c_str(),r.l1,previous);return false;}previous=r.l1;}return true;};
    if(!refinement("aligned_translation",[](int n)->ReferenceAt{return [n](double t){return rectangleReference(n,{.2L+.17L*t,.3L-.11L*t,.4L+.17L*t,.6L-.11L*t});};},.25,.17,uniform,.1))return 1;
    if(!refinement("oblique_translation",[](int n)->ReferenceAt{return [n](double t){return polygonReference(n,oracle::orientedBox(.5L+.07L*t,.5L-.05L*t,.22L,.09L,.63L));};},.2,.07,[](Grid&g,double){setUniformVelocity(g,.07,-.05);},.1))return 1;
    if(!refinement("slotted_translation",[](int n)->ReferenceAt{return [n](double t){return slotReference(n,.13*t,.09*t,0);};},.2,.13,[](Grid&g,double){setUniformVelocity(g,.13,.09);},.1))return 1;
    // Even step count ensures the prescribed reversal occurs exactly at a step boundary.
    double previous=std::numeric_limits<double>::infinity();
    for(int n:{24,48,96}){ReferenceAt exact=[n](double t){double d=t<=.2?t:.4-t;return rectangleReference(n,{.25L+.11L*d,.25L-.07L*d,.42L+.11L*d,.48L-.07L*d});};
        auto r=runCase(n,"reversal",.4,2*stepCount(n,.2,.11,.4),[](Grid&g,double t){double s=t<.2?1:-1;setUniformVelocity(g,s*.11,-s*.07);},exact,trace,summary,stepLog,false,0,.04);if(!r.pass||!(r.l1<previous)){std::fprintf(stderr,"P1-C STOP: reversal failed boundedness/conservation/refinement\n");return 1;}previous=r.l1;}
    {int n=32;double omega=2*std::acos(-1.0);ReferenceAt exact=[=](double t){return slotReference(n,0,0,omega*t);};auto r=runCase(n,"slotted_rotation",1,stepCount(n,1,omega*.5,.28),[omega](Grid&g,double){for(int j=0;j<g.ny;++j)for(int i=0;i<=g.nx;++i)g.u[g.U(i,j)]=-omega*((j+.5)*g.dy-.5);for(int j=0;j<=g.ny;++j)for(int i=0;i<g.nx;++i)g.v[g.V(i,j)]=omega*((i+.5)*g.dx-.5);},exact,trace,summary,stepLog,false,0,.25);if(!r.pass){std::fprintf(stderr,"P1-C STOP: rotation failed\n");return 1;}}
    std::printf("JSGT uniform/zero-directional-divergence gate PASS; starting nonuniform family\n");
    // Independent analytical strain oracle: det(flow map)=exp(t)*exp(-t)=1.
    // MAC divergences are +1 and -1. No flow/fixture classification in the solver.
    auto strain=[](Grid&g,double){for(int j=0;j<g.ny;++j)for(int i=0;i<=g.nx;++i)g.u[g.U(i,j)]=i*g.dx-.5;for(int j=0;j<=g.ny;++j)for(int i=0;i<g.nx;++i)g.v[g.V(i,j)]=-(j*g.dy-.5);};
    // Dyadic grid: the manufactured strain is exactly discretely divergence-free,
    // so the unchanged bit-exact constant assertion has its stated premise.
    for(double value:{0.,1.}){ReferenceAt exact=[value](double){return [value](int,int){return value;};};if(!runCase(32,value==0?"strain_constant_gas":"strain_constant_liquid",.125,4,strain,exact,trace,summary,stepLog,true,value).pass){std::fprintf(stderr,"P1-C STOP: nonuniform constant field failed\n");return 1;}}
    previous=std::numeric_limits<double>::infinity();
    for(int n:{24,48,96}){ReferenceAt exact=[n](double t){long double ex=std::exp((long double)t),ey=std::exp(-(long double)t);return rectangleReference(n,{.5L-.25L*ex,.5L-.25L*ey,.5L+.25L*ex,.5L+(145.0L/192-.5L)*ey});};
        auto r=runCase(n,"nonuniform_strain",.125,n/6,strain,exact,trace,summary,stepLog,false,0,.1,n==24?std::vector<std::pair<int,int>>{{6,18},{7,18}}:std::vector<std::pair<int,int>>{});
        if(!r.pass||!(r.l1<previous)){std::fprintf(stderr,"P1-C STOP: nonuniform divergence-free strain failed boundedness/conservation/refinement; no limiter or further integration\n");return 1;}previous=r.l1;}
    // Second family is nonlinear. These MAC values are exact face averages of
    // u=(x-.5)^2, v=-2*(x-.5)*(y-.5), sampled without divergence projection.
    auto nonlinear=[](Grid&g,double){
        for(int j=0;j<g.ny;++j)for(int i=0;i<=g.nx;++i){double s=i*g.dx-.5;g.u[g.U(i,j)]=s*s;}
        for(int j=0;j<=g.ny;++j)for(int i=0;i<g.nx;++i)g.v[g.V(i,j)]=-2*((i+.5)*g.dx-.5)*(j*g.dy-.5);
    };
    const oracle::Box nonlinearInitial{.25L,.3L,.75L,.7L};
    std::ofstream oracleLog("results/jsgt_nonlinear_oracle.csv");oracleLog<<std::setprecision(20);
    oracleLog<<"n,time,oracle_area,expected_area,area_error,min_fraction,max_fraction,t0_rectangle_max_error\n";
    previous=std::numeric_limits<double>::infinity();
    for(int n:{24,48,96}){
        // Independently check the closed-form reference before scoring transport.
        for(oracle::Real t:{0.L,.25L,.5L}){
            oracle::Real sum=0;double lo=1,hi=0,initialError=0;
            for(int j=0;j<n;++j)for(int i=0;i<n;++i){
                double f=oracle::nonlinearFraction(n,i,j,nonlinearInitial,t);
                sum+=oracle::Real(f)/(n*n);lo=std::min(lo,f);hi=std::max(hi,f);
                if(t==0)initialError=std::max(initialError,std::abs(f-oracle::rectangleFraction(n,i,j,nonlinearInitial)));
            }
            oracle::Real expected=(nonlinearInitial.x1-nonlinearInitial.x0)*(nonlinearInitial.y1-nonlinearInitial.y0);
            oracleLog<<n<<','<<t<<','<<sum<<','<<expected<<','<<std::abs(sum-expected)<<','<<lo<<','<<hi<<','<<initialError<<'\n';
            if(std::abs(sum-expected)>1e-14L||lo < -roundoffAlphaTolerance||hi>1+roundoffAlphaTolerance||initialError!=0){
                std::fprintf(stderr,"STOP: nonlinear analytical oracle failed its independent area/bounds audit\n");return 1;
            }
        }
        ReferenceAt exact=[=](double t){return [=](int i,int j){return oracle::nonlinearFraction(n,i,j,nonlinearInitial,t);};};
        auto r=runCase(n,"nonlinear_incompressible",.5,n,nonlinear,exact,trace,summary,stepLog);
        if(!r.pass||!(r.l1<previous)){
            std::fprintf(stderr,"P1-C STOP: nonlinear nonuniform fixture failed boundedness/conservation/refinement\n");return 1;
        }
        previous=r.l1;
    }
    // Exercise a genuinely nonzero boundary flux. Exact remaining area at T=.5
    // is .01; original area is .04, so .03 must leave the right boundary.
    {
        int n=48;ReferenceAt exact=[n](double t){return rectangleReference(n,{.75L+.4L*t,.3L,.95L+.4L*t,.5L});};
        auto r=runCase(n,"boundary_exit_translation",.5,48,[](Grid&g,double){setUniformVelocity(g,.4,0);},exact,trace,summary,stepLog);
        if(!r.pass){std::fprintf(stderr,"P1-C STOP: open-boundary volume budget failed\n");return 1;}
    }
    // Independent exact outflow oracle without oblique/corner reconstruction error:
    // a full-height slab [0.75,0.95] translates by 0.2; retained=.05, exported=.15.
    {
        int n=48;ReferenceAt exact=[n](double t){return rectangleReference(n,{.75L+.4L*t,0,.95L+.4L*t,1});};
        auto r=runCase(n,"boundary_exit_slab",.5,48,[](Grid&g,double){setUniformVelocity(g,.4,0);},exact,trace,summary,stepLog,false,0,1e-11);
        std::printf("BOUNDARY ORACLE retained_error=%.17g exported_error=%.17g\n",std::abs(r.finalVolume-.05),std::abs(r.exportedVolume-.15));
        if(!r.pass||std::abs(r.finalVolume-.05)>=1e-11||std::abs(r.exportedVolume-.15)>=1e-11){
            std::fprintf(stderr,"P1-C STOP: analytical boundary-exit slab oracle failed\n");return 1;
        }
    }
    std::printf("P1-C frozen-WY JSGT PASS (no moving-solid or pressure/body work executed)\n");return 0;
}
}
#include "momentum_validation.hpp"
int main(){int result=r1p1::run();return result?result:r1p1::runMomentumValidation();}
