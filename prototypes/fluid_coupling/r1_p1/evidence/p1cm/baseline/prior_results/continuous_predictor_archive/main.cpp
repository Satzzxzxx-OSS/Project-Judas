#include "vof.hpp"
#include "transport_oracles.hpp"
#include <iomanip>
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
struct StepStats {StageStats predictorX,predictorY,final;bool valid{};double orderAverageError{};};
using ExactField=std::function<double(double,double)>;

static Vec2 normalAt(const Grid&g,const std::vector<double>&a,int i,int j){
    auto at=[&](int x,int y){x=std::clamp(x,0,g.nx-1);y=std::clamp(y,0,g.ny-1);return a[g.C(x,y)];};
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
    if(x){
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
static std::vector<double> predictor(const Grid&g,const std::vector<double>&base,const Fluxes&f,Axis a,double dt,double&identityResidual){
    std::vector<double>out(base.size());double cell=g.dx*g.dy,v0=0,v1=0,dilation=0;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){double net=0;
        double divDt=0;
        if(a==Axis::X){double left=g.u[g.U(i,j)]*dt*g.dy,right=g.u[g.U(i+1,j)]*dt*g.dy;net=f.x[g.U(i+1,j)]-f.x[g.U(i,j)];divDt=(right-left)/cell;}
        else {double bottom=g.v[g.V(i,j)]*dt*g.dx,top=g.v[g.V(i,j+1)]*dt*g.dx;net=f.y[g.V(i,j+1)]-f.y[g.V(i,j)];divDt=(top-bottom)/cell;}
        double delta=-net/cell+base[g.C(i,j)]*divDt;out[g.C(i,j)]=base[g.C(i,j)]+delta;
        v0+=base[g.C(i,j)]*cell;v1+=out[g.C(i,j)]*cell;dilation+=base[g.C(i,j)]*divDt*cell;
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
    const std::vector<double>& out,const Fluxes& flux,Axis axis,double dt,
    const StageStats& stats) {
    const double cell=g.dx*g.dy;
    const int i=stats.badI,j=stats.badJ;
    auto detail=[&](int x,int y) {
        if(x<0||x>=g.nx||y<0||y>=g.ny)return;
        size_t k=g.C(x,y);double left,right,wl,wr,h;
        if(axis==Axis::X){left=flux.x[g.U(x,y)];right=flux.x[g.U(x+1,y)];wl=g.u[g.U(x,y)];wr=g.u[g.U(x+1,y)];h=g.dx;}
        else {left=flux.y[g.V(x,y)];right=flux.y[g.V(x,y+1)];wl=g.v[g.V(x,y)];wr=g.v[g.V(x,y+1)];h=g.dy;}
        double divDt=(wr-wl)*dt/h,advected=base[k]-(right-left)/cell;
        std::fprintf(stderr,"  cell=(%d,%d) Cn=%.17g Fminus=%.17g Fplus=%.17g wminus=%.17g wplus=%.17g divDt=%.17g conservative_part=%.17g dilation=%.17g predictor=%.17g\n",
          x,y,base[k],left,right,wl,wr,divDt,advected,base[k]*divDt,out[k]);
    };
    std::fprintf(stderr,"FIRST MATERIAL INVALID: test=%s step=%d operation=%s cell=(%d,%d) alpha=%.17g tolerance=%.17g\n",stats.test.c_str(),stats.step,stats.stage.c_str(),i,j,stats.badAlpha,roundoffAlphaTolerance);
    for(int y=j-1;y<=j+1;++y)for(int x=i-1;x<=i+1;++x)detail(x,y);
}

// Judas Symmetric Geometric Transport (JSGT), an intentional nonlinear method
// change. This is NOT claimed to reproduce the authors' COSMIC Eq. 15 exactly.
static StepStats jsgtStep(Grid&g,double dt,const std::string&test,int step,
    const ExactField&exactX,const ExactField&exactY,const ExactField&exactFinal,std::ofstream&csv){
    StepStats result;double v0=g.volume(),maxFx=0,maxFy=0,resX=0,resY=0;
    Reconstruction rn=reconstruct(g,g.c);
    Fluxes fluxXn=directionalFluxes(g,rn,Axis::X,dt,maxFx),fluxYn=directionalFluxes(g,rn,Axis::Y,dt,maxFy);
    if(!std::isfinite(maxFx)||!std::isfinite(maxFy)){std::fprintf(stderr,"%s step %d: directional Courant exceeds 1\n",test.c_str(),step);return result;}
    std::vector<double>cx=predictor(g,g.c,fluxXn,Axis::X,dt,resX),cy=predictor(g,g.c,fluxYn,Axis::Y,dt,resY);
    result.predictorX=inspect(g,cx,test,"predictor_x",step,v0,maxFx,resX,exactX);writeTrace(csv,result.predictorX);
    if(result.predictorX.invalid){printPredictorFailure(g,g.c,cx,fluxXn,Axis::X,dt,result.predictorX);return result;}
    result.predictorY=inspect(g,cy,test,"predictor_y",step,v0,maxFy,resY,exactY);writeTrace(csv,result.predictorY);
    if(result.predictorY.invalid){printPredictorFailure(g,g.c,cy,fluxYn,Axis::Y,dt,result.predictorY);return result;}
    // Each full predictor gets its own PLIC. Never re-PLIC averaged fractions.
    Reconstruction rcx=reconstruct(g,cx),rcy=reconstruct(g,cy);
    double maxCrossX=0,maxCrossY=0;
    Fluxes crossX=directionalFluxes(g,rcy,Axis::X,dt,maxCrossX),crossY=directionalFluxes(g,rcx,Axis::Y,dt,maxCrossY);
    Fluxes finalX=fluxXn,finalY=fluxYn;double maxFinal=0;
    for(size_t k=0;k<finalX.x.size();++k){finalX.x[k]=.5*(fluxXn.x[k]+crossX.x[k]);maxFinal=std::max(maxFinal,std::abs(finalX.x[k]));}
    for(size_t k=0;k<finalY.y.size();++k){finalY.y[k]=.5*(fluxYn.y[k]+crossY.y[k]);maxFinal=std::max(maxFinal,std::abs(finalY.y[k]));}
    double residual=0;std::vector<double>next=finalUpdate(g,g.c,finalX,finalY,residual);
    // Diagnostic identity only: these compositions never feed the accepted state.
    // The equality is claimed only when each directional velocity divergence is zero.
    bool zeroDirectionalDivergence=true;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i)
        zeroDirectionalDivergence=zeroDirectionalDivergence&&g.u[g.U(i+1,j)]==g.u[g.U(i,j)]&&g.v[g.V(i,j+1)]==g.v[g.V(i,j)];
    if(zeroDirectionalDivergence){
        double unused=0;auto xy=predictor(g,cy,crossX,Axis::X,dt,unused),yx=predictor(g,cx,crossY,Axis::Y,dt,unused);
        for(size_t k=0;k<next.size();++k)result.orderAverageError=std::max(result.orderAverageError,std::abs(next[k]-.5*(xy[k]+yx[k])));
    }
    result.final=inspect(g,next,test,"jsgt_final",step,v0,maxFinal,residual,exactFinal);writeTrace(csv,result.final);
    if(result.final.invalid){int i=result.final.badI,j=result.final.badJ;size_t k=g.C(i,j);
        std::fprintf(stderr,"FIRST MATERIAL INVALID: test=%s step=%d operation=jsgt_final cell=(%d,%d) Cn=%.17g Cx=%.17g Cy=%.17g xL=%.17g xR=%.17g yB=%.17g yT=%.17g result=%.17g\n",test.c_str(),step,i,j,g.c[k],cx[k],cy[k],finalX.x[g.U(i,j)],finalX.x[g.U(i+1,j)],finalY.y[g.V(i,j)],finalY.y[g.V(i,j+1)],next[k]);return result;}
    g.c.swap(next);result.valid=true;return result;
}

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
struct RunSummary {bool pass{};int completed{};double l1{-1},lo{1},hi{},maxResidual{},maxOrderError{},runtime{},volumeError{};};
using VelocitySetter=std::function<void(Grid&,double)>;
using ReferenceAt=std::function<Reference(double)>;
static RunSummary runCase(int n,const std::string&name,double T,int steps,
    VelocitySetter velocity,ReferenceAt exact,std::ofstream&trace,std::ofstream&summary,
    bool constant=false,double constantValue=0,double errorLimit=.1){
    auto start=std::chrono::steady_clock::now();Grid g(n,n,1.0/n,1.0/n);initialize(g,exact(0));
    if(constant)g.inflowLeft=g.inflowRight=g.inflowBottom=g.inflowTop=constantValue;
    RunSummary result;double initialVolume=g.volume(),dt=T/steps,maxCFL=0,maxDiv=0,maxDirDiv=0;
    bool valid=true;int failedStep=-1;std::string failedStage;
    for(int k=0;k<steps;++k){
        velocity(g,k*dt);maxCFL=std::max(maxCFL,maxCourant(g,dt));maxDiv=std::max(maxDiv,maxDivergence(g,false));maxDirDiv=std::max(maxDirDiv,maxDivergence(g,true));
        // Predictor references deliberately omitted for nonuniform directional subproblems;
        // final-state reference is the independent full analytical flow map.
        auto stats=jsgtStep(g,dt,name,k,{}, {},asField(g,exact((k+1)*dt)),trace);
        for(const StageStats* s:{&stats.predictorX,&stats.predictorY,&stats.final})if(!s->stage.empty()){
            result.lo=std::min(result.lo,s->minimum);result.hi=std::max(result.hi,s->maximum);result.maxResidual=std::max(result.maxResidual,std::abs(s->massResidual));
            if(s->invalid&&failedStage.empty())failedStage=s->stage;
        }
        result.maxOrderError=std::max(result.maxOrderError,stats.orderAverageError);
        if(!stats.valid){valid=false;failedStep=k;break;}
        ++result.completed;
    }
    result.runtime=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    result.volumeError=std::abs(g.volume()-initialVolume);
    if(valid)result.l1=l1Error(g,g.c,asField(g,exact(T)));
    bool unchanged=true;if(constant)for(double c:g.c)unchanged=unchanged&&c==constantValue;
    // These use the pre-existing alpha and volume tolerances; no widened gates.
    result.pass=valid&&bounded(g,g.c)&&result.volumeError<1e-11&&result.maxResidual<1e-11&&result.maxOrderError<roundoffAlphaTolerance&&result.l1<errorLimit&&(!constant||unchanged);
    std::printf("%s n=%d completed=%d/%d dt=%.17g CFL=%.9g volume_error=%.17g residual=%.17g min=%.17g max=%.17g L1=%.17g order_identity_error=%.17g div=%.17g directional_div=%.17g runtime=%.6gs %s",
        name.c_str(),n,result.completed,steps,dt,maxCFL,result.volumeError,result.maxResidual,result.lo,result.hi,result.l1,result.maxOrderError,maxDiv,maxDirDiv,result.runtime,result.pass?"PASS":"FAIL");
    if(!valid)std::printf(" first_failed_step=%d operation=%s (candidate rejected; no final-time L1)",failedStep,failedStage.c_str());
    std::printf("\n");
    summary<<name<<','<<n<<','<<dt<<','<<maxCFL<<','<<result.completed<<','<<steps<<','<<initialVolume<<','<<result.volumeError<<','<<result.maxResidual<<','<<result.lo<<','<<result.hi<<','<<result.l1<<','<<result.maxOrderError<<','<<maxDiv<<','<<maxDirDiv<<','<<result.runtime<<','<<(result.pass?"PASS":"FAIL")<<','<<failedStep<<','<<failedStage<<'\n';
    return result;
}
static int stepCount(int n,double T,double speed,double cfl){return int(std::ceil(T*speed*n/cfl));}
static int run(){
    std::setvbuf(stdout,nullptr,_IONBF,0); // Preserve diagnostic order in captured logs.
    // Invoked with the prototype directory as cwd: all artifacts remain isolated.
    std::filesystem::create_directories("results");std::ofstream trace("results/jsgt_trace.csv"),summary("results/jsgt_summary.csv");trace<<std::setprecision(17);summary<<std::setprecision(17);csvHeader(trace);
    summary<<"test,n,dt,CFL,completed_steps,requested_steps,initial_volume,accepted_state_volume_error,max_stage_residual,min_alpha,max_alpha,final_L1,order_average_identity_error,max_total_divergence,max_directional_divergence,runtime_s,status,first_failed_step,first_failed_operation\n";
    auto uniform=[](Grid&g,double){setUniformVelocity(g,.17,-.11);};
    for(double value:{0.,1.}){ReferenceAt exact=[value](double){return [value](int,int){return value;};};
        if(!runCase(24,value==0?"constant_gas":"constant_liquid",.1,3,uniform,exact,trace,summary,true,value).pass)return 1;}
    auto refinement=[&](const std::string&name,auto factory,double T,double speed,VelocitySetter velocity,double errorLimit){
        double previous=std::numeric_limits<double>::infinity();
        for(int n:{24,48,96}){auto r=runCase(n,name,T,stepCount(n,T,speed,.42),velocity,factory(n),trace,summary,false,0,errorLimit);if(!r.pass){std::fprintf(stderr,"P1-C STOP: %s n=%d failed\n",name.c_str(),n);return false;}if(!(r.l1<previous)){std::fprintf(stderr,"P1-C STOP: %s error did not decrease: %.17g >= %.17g\n",name.c_str(),r.l1,previous);return false;}previous=r.l1;}return true;};
    if(!refinement("aligned_translation",[](int n)->ReferenceAt{return [n](double t){return rectangleReference(n,{.2L+.17L*t,.3L-.11L*t,.4L+.17L*t,.6L-.11L*t});};},.25,.17,uniform,.1))return 1;
    if(!refinement("oblique_translation",[](int n)->ReferenceAt{return [n](double t){return polygonReference(n,oracle::orientedBox(.5L+.07L*t,.5L-.05L*t,.22L,.09L,.63L));};},.2,.07,[](Grid&g,double){setUniformVelocity(g,.07,-.05);},.1))return 1;
    if(!refinement("slotted_translation",[](int n)->ReferenceAt{return [n](double t){return slotReference(n,.13*t,.09*t,0);};},.2,.13,[](Grid&g,double){setUniformVelocity(g,.13,.09);},.1))return 1;
    // Even step count ensures the prescribed reversal occurs exactly at a step boundary.
    double previous=std::numeric_limits<double>::infinity();
    for(int n:{24,48,96}){ReferenceAt exact=[n](double t){double d=t<=.2?t:.4-t;return rectangleReference(n,{.25L+.11L*d,.25L-.07L*d,.42L+.11L*d,.48L-.07L*d});};
        auto r=runCase(n,"reversal",.4,2*stepCount(n,.2,.11,.4),[](Grid&g,double t){double s=t<.2?1:-1;setUniformVelocity(g,s*.11,-s*.07);},exact,trace,summary,false,0,.04);if(!r.pass||!(r.l1<previous)){std::fprintf(stderr,"P1-C STOP: reversal failed boundedness/conservation/refinement\n");return 1;}previous=r.l1;}
    {int n=32;double omega=2*std::acos(-1.0);ReferenceAt exact=[=](double t){return slotReference(n,0,0,omega*t);};auto r=runCase(n,"slotted_rotation",1,stepCount(n,1,omega*.5,.28),[omega](Grid&g,double){for(int j=0;j<g.ny;++j)for(int i=0;i<=g.nx;++i)g.u[g.U(i,j)]=-omega*((j+.5)*g.dy-.5);for(int j=0;j<=g.ny;++j)for(int i=0;i<g.nx;++i)g.v[g.V(i,j)]=omega*((i+.5)*g.dx-.5);},exact,trace,summary,false,0,.25);if(!r.pass){std::fprintf(stderr,"P1-C STOP: rotation failed\n");return 1;}}
    std::printf("JSGT uniform/zero-directional-divergence gate PASS; starting nonuniform family\n");
    // Independent analytical strain oracle: det(flow map)=exp(t)*exp(-t)=1.
    // MAC divergences are +1 and -1. No flow/fixture classification in the solver.
    auto strain=[](Grid&g,double){for(int j=0;j<g.ny;++j)for(int i=0;i<=g.nx;++i)g.u[g.U(i,j)]=i*g.dx-.5;for(int j=0;j<=g.ny;++j)for(int i=0;i<g.nx;++i)g.v[g.V(i,j)]=-(j*g.dy-.5);};
    // Dyadic grid: the manufactured strain is exactly discretely divergence-free,
    // so the unchanged bit-exact constant assertion has its stated premise.
    for(double value:{0.,1.}){ReferenceAt exact=[value](double){return [value](int,int){return value;};};if(!runCase(32,value==0?"strain_constant_gas":"strain_constant_liquid",.125,4,strain,exact,trace,summary,true,value).pass){std::fprintf(stderr,"P1-C STOP: nonuniform constant field failed\n");return 1;}}
    for(int n:{24,48,96}){ReferenceAt exact=[n](double t){long double ex=std::exp((long double)t),ey=std::exp(-(long double)t);return rectangleReference(n,{.5L-.25L*ex,.5L-.25L*ey,.5L+.25L*ex,.5L+(145.0L/192-.5L)*ey});};auto r=runCase(n,"nonuniform_strain",.125,n/6,strain,exact,trace,summary);if(!r.pass){std::fprintf(stderr,"P1-C STOP: nonuniform divergence-free strain failed; no limiter or further integration\n");return 1;}}
    std::printf("P1-C JSGT PASS (no moving-solid or pressure/body work executed)\n");return 0;
}
}
int main(){return r1p1::run();}
