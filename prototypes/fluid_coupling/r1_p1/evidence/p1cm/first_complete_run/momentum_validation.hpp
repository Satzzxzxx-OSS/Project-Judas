#pragma once
// P1-C-M acceptance fixtures and independent expectations only.
// This header calls the single JSGT/momentum implementation; it contains no
// replacement fraction, mass, or momentum transport algorithm.
#include <array>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

namespace r1p1 {
namespace momentum_validation {
using Velocities=std::array<std::vector<double>,2>;
using ExactVelocity=std::function<double(int,const Grid&,const DualGrid&,size_t,double)>;
static constexpr double pi=3.141592653589793238462643383279502884;
struct Random {
    uint64_t state{32031};
    double unit(){state^=state>>12;state^=state<<25;state^=state>>27;return double((state*2685821657736338717ULL)>>11)*0x1.0p-53;}
};
static long double sum(const std::vector<double>&v){long double s=0;for(double x:v)s+=x;return s;}
static long double unsignedSum(const std::vector<double>&v){long double s=0;for(double x:v)s+=std::abs(x);return s;}
static std::array<double,2> center(const Grid&g,const DualGrid&d,size_t k){
    double loX=1,hiX=0,loY=1,hiY=0;
    for(size_t f:d.cells[k]){double x=(double(f%size_t(g.nx))+.5)*g.dx,y=(double(f/size_t(g.nx))+.5)*g.dy;loX=std::min(loX,x);hiX=std::max(hiX,x);loY=std::min(loY,y);hiY=std::max(hiY,y);}
    double x=0,y=0;for(size_t f:d.cells[k]){double a=(double(f%size_t(g.nx))+.5)*g.dx,b=(double(f/size_t(g.nx))+.5)*g.dy;if(g.periodicX&&hiX-loX>.5&&a<.5)a+=1;if(g.periodicY&&hiY-loY>.5&&b<.5)b+=1;x+=a;y+=b;}
    x/=d.cells[k].size();y/=d.cells[k].size();return {x>=1?x-1:x,y>=1?y-1:y};
}
static double sineAverage(double a,double b,double shift,bool cosine){
    const long double k=2*acosl(-1.L),l=static_cast<long double>(a)-shift,r=static_cast<long double>(b)-shift;
    return double(cosine?(sinl(k*r)-sinl(k*l))/(k*(b-a)):(cosl(k*l)-cosl(k*r))/(k*(b-a)));
}
// Exact dual-volume averages, integrating every actual constituent fine rectangle.
// This handles wrapped periodic and clipped physical boundary duals without ghosts.
static double smoothAverage(int q,const Grid&g,const DualGrid&d,size_t k,double t){
    long double integral=0;
    for(size_t f:d.cells[k]){int i=int(f%size_t(g.nx)),j=int(f/size_t(g.nx));double sx=sineAverage(i*g.dx,(i+1)*g.dx,.37*t,false),cx=sineAverage(i*g.dx,(i+1)*g.dx,.37*t,true),sy=sineAverage(j*g.dy,(j+1)*g.dy,-.21*t,false),cy=sineAverage(j*g.dy,(j+1)*g.dy,-.21*t,true);double w=q==0?.73+.20*sx+.10*cy:-.42+.15*cx-.05*sy;integral+=static_cast<long double>(w)*g.dx*g.dy;}
    return double(integral/d.volume[k]);
}
static Velocities constant(const MomentumState&m,double x,double y){Velocities w;for(int q=0;q<2;++q)w[q].assign(m.layout[q].cells.size(),q==0?x:y);return w;}
static Velocities smooth(const Grid&g,const MomentumState&m){Velocities w;for(int q=0;q<2;++q){w[q].resize(m.layout[q].cells.size());for(size_t k=0;k<w[q].size();++k)w[q][k]=smoothAverage(q,g,m.layout[q],k,0);}return w;}
static void slab(Grid&g,double left,double right,double bottom=0,double top=1){for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i)g.c[g.C(i,j)]=oracle::rectangleFraction(g.nx,i,j,{left,bottom,right,top});}
static void mixedFractions(Grid&g){Random rng;for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){double x=(i+.5)*g.dx,y=(j+.5)*g.dy;double c=(x-.48)*(x-.48)+(y-.51)*(y-.51)<.24*.24?1:0;if(x>.30&&x<.65&&y>.32&&y<.67)c=rng.unit();g.c[g.C(i,j)]=c;}}
struct CoarseFlow {std::vector<double> u,v;double prolongationError{},divergenceError{};};
static CoarseFlow coarseFlow(Grid&g,bool nonuniform){
    int nx=g.nx/2,ny=g.ny/2;double hx=2*g.dx,hy=2*g.dy;CoarseFlow f;f.u.resize(size_t((nx+1)*ny));f.v.resize(size_t(nx*(ny+1)));
    auto psi=[&](int i,int j){i=(i%nx+nx)%nx;j=(j%ny+ny)%ny;return .07*std::sin(2*pi*i*hx)*std::sin(2*pi*j*hy);};
    for(int j=0;j<ny;++j)for(int i=0;i<=nx;++i)f.u[size_t(j*(nx+1)+i)]=nonuniform?(psi(i,j+1)-psi(i,j))/hy:.37;
    for(int j=0;j<=ny;++j)for(int i=0;i<nx;++i)f.v[size_t(j*nx+i)]=nonuniform?-(psi(i+1,j)-psi(i,j))/hx:-.21;
    f.prolongationError=prolongMAC(g,f.u,f.v);
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){int I=i/2,J=j/2;double parent=(f.u[size_t(J*(nx+1)+I+1)]-f.u[size_t(J*(nx+1)+I)])/hx+(f.v[size_t((J+1)*nx+I)]-f.v[size_t(J*nx+I)])/hy;double fine=(g.u[g.U(i+1,j)]-g.u[g.U(i,j)])/g.dx+(g.v[g.V(i,j+1)]-g.v[g.V(i,j)])/g.dy;f.divergenceError=std::max(f.divergenceError,std::abs(fine-parent));}
    return f;
}
static Velocities referenceSmooth(const Grid&g,const MomentumState&m,const CoarseFlow&f){Velocities w;int nx=g.nx/2,ny=g.ny/2;for(int q=0;q<2;++q){w[q].resize(m.layout[q].cells.size());for(size_t k=0;k<w[q].size();++k){auto xy=center(g,m.layout[q],k);int i=int(std::llround(xy[0]*nx-(q==0?0:.5))),j=int(std::llround(xy[1]*ny-(q==0?.5:0)));i=(i%nx+nx)%nx;j=(j%ny+ny)%ny;w[q][k]=q==0?f.u[size_t(j*(nx+1)+i)]:f.v[size_t(j*nx+i)];}}return w;}
static bool layoutAudit(const Grid&g,const MomentumState&m){
    for(int q=0;q<2;++q){std::vector<int>count(g.c.size());long double vol=0;for(size_t k=0;k<m.layout[q].cells.size();++k){double expected=0;for(size_t f:m.layout[q].cells[k]){if(f>=g.c.size())return false;++count[f];expected+=g.dx*g.dy;}if(expected!=m.layout[q].volume[k]||expected<=0)return false;vol+=expected;}
        for(int n:count){if(n!=1)return false;}
        if(std::abs(vol-g.nx*g.dx*g.ny*g.dy)>1e-13L)return false;
    }return true;
}
struct Result {
    bool pass{};int completed{};double ratio{},dt{},cfl{},runtime{},localNorm{},globalMassNorm{},globalPNorm{},commonError{},comboEnergyNorm{},energyGrowth{},oldMassWrong{},velocityMeanWrong{},fractionMin{1},fractionMax{},velocityMin{std::numeric_limits<double>::infinity()},velocityMax{-std::numeric_limits<double>::infinity()},l1x{-1},l1y{-1},finalMass{},initialMass{},massExport{},massSource{},liquidVolume{};
    std::array<double,2>initialP{},finalP{},pExport{},pSource{};std::vector<double>fraction;Velocities velocity;std::string failure;
};
struct Logs {
    std::ofstream trace,summary,fractions,controls;
    Logs():trace("evidence/p1cm/current/momentum_steps.csv"),summary("evidence/p1cm/current/momentum_summary.csv"),fractions("evidence/p1cm/current/momentum_fraction_trace.csv"),controls("evidence/p1cm/current/momentum_controls.csv"){
        trace<<std::setprecision(17);summary<<std::setprecision(17);fractions<<std::setprecision(17);controls<<std::setprecision(17);
        csvHeader(fractions);
        trace<<"test,density_ratio,step,dt,CFL_sum,min_fraction,max_fraction,min_mass,fine_mass_raw,fine_mass_norm,dual_mass_raw,dual_mass_norm,combination_mass_raw,combination_mass_norm,core_cumulative_mass_raw,core_cumulative_mass_norm,core_cumulative_px_raw,core_cumulative_px_norm,core_cumulative_py_raw,core_cumulative_py_norm,source_adjusted_cumulative_mass_raw,source_adjusted_cumulative_mass_norm,source_adjusted_cumulative_px_raw,source_adjusted_cumulative_px_norm,source_adjusted_cumulative_py_raw,source_adjusted_cumulative_py_norm,mass_source,px_source,py_source,common_vector_error,wx_min,wx_max,wy_min,wy_max,kinetic_before,kinetic_after,mean_branch_energy,combination_energy_raw,combination_energy_norm,full_step_energy_growth_norm,wrong_old_mass_error,wrong_velocity_mean_error,accepted,failure\n";
        summary<<"test,fine_n,MAC_n,density_ratio,steps,requested_steps,dt,CFL_sum,local_mass_max_norm,cumulative_mass_max_norm,cumulative_momentum_max_norm,common_vector_max_error,combination_energy_max_norm,full_step_energy_max_growth_norm,wrong_old_mass_error,wrong_velocity_mean_error,min_fraction,max_fraction,min_velocity,max_velocity,final_L1_x,final_L1_y,initial_mass,final_mass,mass_export,mass_source,initial_px,final_px,px_export,px_source,initial_py,final_py,py_export,py_source,liquid_volume,runtime_s,status,failure\n";
        controls<<"test,quantity,actual,expected,error_or_wrong_result,status\n";
    }
};
static Result run(const std::string&name,Grid g,double rhoL,const Velocities&w,double dt,int steps,Logs&logs,bool common=false,bool closed=true,ExactVelocity exact={}){
    auto start=std::chrono::steady_clock::now();Result r;r.ratio=rhoL;r.dt=dt;r.cfl=cumulativeCourant(g,dt);MomentumState m(g,rhoL,1);m.initialize(g,w);m.boundaryVelocity={w[0].empty()?0:w[0][0],w[1].empty()?0:w[1][0]};
    if(!layoutAudit(g,m)){r.failure="dual_layout_partition";std::fprintf(stderr,"P1-C-M STOP %s: dual layout does not partition physical cells/volume\n",name.c_str());return r;}
    double uref=1;for(const auto&a:w)for(double x:a)uref=std::max(uref,std::abs(x));
    auto mass0=fineMass(g,g.c,rhoL,1);r.initialMass=double(sum(mass0));long double massInitial=sum(mass0),unsignedMass=unsignedSum(mass0),massBoundary=0,massSources=0;
    std::array<long double,2>pInitial{sum(m.P[0]),sum(m.P[1])},pMagnitude{unsignedSum(m.P[0]),unsignedSum(m.P[1])},pBoundary{},pSources{};
    long double massBoundaryMagnitude=0;std::array<long double,2>pBoundaryMagnitude{};
    for(int q=0;q<2;++q)r.initialP[q]=double(pInitial[q]);
    for(int step=0;step<steps;++step){
        TransportRecord record;auto fraction=jsgtStep(g,dt,name,step,{},{},{},logs.fractions,{},&record);
        if(!fraction.valid){r.failure="fraction_"+fraction.failure;break;}
        auto d=advanceMomentum(g,record,dt,m);
        auto fine=fineMass(g,g.c,rhoL,1);long double totalMass=sum(fine);
        massBoundary+=d.massBoundary;massSources+=d.massSource;massBoundaryMagnitude+=d.massBoundaryAbs;
        for(int q=0;q<2;++q){pBoundary[q]+=d.pBoundary[q];pSources[q]+=d.pSource[q];pBoundaryMagnitude[q]+=d.pBoundaryAbs[q];}
        double massScale=double(std::max({1.L,unsignedMass,massBoundaryMagnitude}));double massRaw=double(totalMass-massInitial+massBoundary-massSources),massNorm=std::abs(massRaw)/massScale;
        std::array<double,2>pRaw{},pNorm{};for(int q=0;q<2;++q){double scale=double(std::max({static_cast<long double>(uref),pMagnitude[q],pBoundaryMagnitude[q]}));pRaw[q]=double(sum(m.P[q])-pInitial[q]+pBoundary[q]-pSources[q]);pNorm[q]=std::abs(pRaw[q])/scale;}
        double commonError=0;Velocities actual;for(int q=0;q<2;++q){auto mass=restrictValues(m.layout[q],fine);actual[q].resize(mass.size());for(size_t k=0;k<mass.size();++k){double v=m.P[q][k]/mass[k];actual[q][k]=v;if(common)commonError=std::max(commonError,std::abs(v-w[q][k]));if(!(mass[k]>0)||!std::isfinite(v))r.failure="nonpositive_or_nonfinite_state";r.velocityMin=std::min(r.velocityMin,v);r.velocityMax=std::max(r.velocityMax,v);}}
        for(const StageStats*s:{&fraction.predictorX,&fraction.predictorY,&fraction.orderXY,&fraction.orderYX,&fraction.final}){r.fractionMin=std::min(r.fractionMin,s->minimum);r.fractionMax=std::max(r.fractionMax,s->maximum);}
        r.localNorm=std::max({r.localNorm,d.fineMassNorm,d.dualMassNorm,d.combinationMassNorm});r.globalMassNorm=std::max({r.globalMassNorm,massNorm,d.massResidualNorm});r.globalPNorm=std::max({r.globalPNorm,pNorm[0],pNorm[1],d.momentumResidualNorm[0],d.momentumResidualNorm[1]});r.commonError=std::max(r.commonError,commonError);r.comboEnergyNorm=std::max(r.comboEnergyNorm,d.combinationEnergyNorm);r.energyGrowth=std::max(r.energyGrowth,d.fullStepEnergyGrowthNorm);r.oldMassWrong=std::max(r.oldMassWrong,d.wrongOldMassError);r.velocityMeanWrong=std::max(r.velocityMeanWrong,d.wrongVelocityMeanError);
        if(!d.valid)r.failure="momentum_"+d.failure;
        else if(r.localNorm>1e-12)r.failure="local_mass_accounting";
        else if(massNorm>1e-11||pNorm[0]>1e-11||pNorm[1]>1e-11)r.failure="cumulative_conserved_budget";
        else if(commonError>1e-8*uref)r.failure="common_vector";
        else if(d.combinationEnergyNorm>1e-12)r.failure="branch_combination_energy";
        else if(closed&&d.fullStepEnergyGrowthNorm>1e-11)r.failure="closed_full_step_energy_growth";
        if(closed&&r.failure.empty()){if(std::abs(double(massSources))/massScale>1e-11)r.failure="frozen_mass_source_cancellation";for(int q=0;q<2&&r.failure.empty();++q)if(std::abs(double(pSources[q]))/double(std::max(static_cast<long double>(uref),pMagnitude[q]))>1e-11)r.failure="frozen_momentum_source_cancellation";}
        logs.trace<<name<<','<<rhoL<<','<<step<<','<<dt<<','<<r.cfl<<','<<r.fractionMin<<','<<r.fractionMax<<','<<d.minMass<<','<<d.fineMassRaw<<','<<d.fineMassNorm<<','<<d.dualMassRaw<<','<<d.dualMassNorm<<','<<d.combinationMassRaw<<','<<d.combinationMassNorm<<','<<d.massResidualRaw<<','<<d.massResidualNorm<<','<<d.momentumResidualRaw[0]<<','<<d.momentumResidualNorm[0]<<','<<d.momentumResidualRaw[1]<<','<<d.momentumResidualNorm[1]<<','<<massRaw<<','<<massNorm<<','<<pRaw[0]<<','<<pNorm[0]<<','<<pRaw[1]<<','<<pNorm[1]<<','<<d.massSource<<','<<d.pSource[0]<<','<<d.pSource[1]<<','<<commonError<<','<<d.velocityMin[0]<<','<<d.velocityMax[0]<<','<<d.velocityMin[1]<<','<<d.velocityMax[1]<<','<<d.kineticBefore<<','<<d.kineticAfter<<','<<d.meanBranchEnergy<<','<<d.combinationEnergyRaw<<','<<d.combinationEnergyNorm<<','<<d.fullStepEnergyGrowthNorm<<','<<d.wrongOldMassError<<','<<d.wrongVelocityMeanError<<','<<r.failure.empty()<<','<<r.failure<<'\n';
        if(!r.failure.empty())break;
        ++r.completed;
    }
    auto finalFine=fineMass(g,g.c,rhoL,1);r.finalMass=double(sum(finalFine));r.massExport=double(massBoundary);r.massSource=double(massSources);r.liquidVolume=g.volume();r.fraction=g.c;
    for(int q=0;q<2;++q){auto mass=restrictValues(m.layout[q],finalFine);r.velocity[q].resize(mass.size());for(size_t k=0;k<mass.size();++k)r.velocity[q][k]=m.P[q][k]/mass[k];r.finalP[q]=double(sum(m.P[q]));r.pExport[q]=double(pBoundary[q]);r.pSource[q]=double(pSources[q]);}
    if(exact&&r.completed==steps){std::array<double,2>error{};for(int q=0;q<2;++q)for(size_t k=0;k<m.layout[q].cells.size();++k)error[q]+=m.layout[q].volume[k]*std::abs(r.velocity[q][k]-exact(q,g,m.layout[q],k,dt*steps));r.l1x=error[0];r.l1y=error[1];}
    r.runtime=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();r.pass=r.failure.empty()&&r.completed==steps;
    logs.summary<<name<<','<<g.nx<<','<<g.nx/2<<','<<rhoL<<','<<r.completed<<','<<steps<<','<<dt<<','<<r.cfl<<','<<r.localNorm<<','<<r.globalMassNorm<<','<<r.globalPNorm<<','<<r.commonError<<','<<r.comboEnergyNorm<<','<<r.energyGrowth<<','<<r.oldMassWrong<<','<<r.velocityMeanWrong<<','<<r.fractionMin<<','<<r.fractionMax<<','<<r.velocityMin<<','<<r.velocityMax<<','<<r.l1x<<','<<r.l1y<<','<<r.initialMass<<','<<r.finalMass<<','<<r.massExport<<','<<r.massSource<<','<<r.initialP[0]<<','<<r.finalP[0]<<','<<r.pExport[0]<<','<<r.pSource[0]<<','<<r.initialP[1]<<','<<r.finalP[1]<<','<<r.pExport[1]<<','<<r.pSource[1]<<','<<r.liquidVolume<<','<<r.runtime<<','<<(r.pass?"PASS":"FAIL")<<','<<r.failure<<'\n';
    std::printf("P1-C-M %s n=%d ratio=%.0f steps=%d/%d local=%.3g mass=%.3g momentum=%.3g common=%.3g combinationE=%.3g growthE=%.3g L1=(%.9g,%.9g) runtime=%.5gs %s %s\n",name.c_str(),g.nx,rhoL,r.completed,steps,r.localNorm,r.globalMassNorm,r.globalPNorm,r.commonError,r.comboEnergyNorm,r.energyGrowth,r.l1x,r.l1y,r.runtime,r.pass?"PASS":"FAIL",r.failure.c_str());return r;
}
static bool controls(Logs&logs){
    double p=segmentMomentum(.25,2,-1)+segmentMomentum(-.25,2,-1),correct=.75;
    bool ok=p==correct;logs.controls<<"counterflow,momentum,"<<p<<','<<correct<<",0,"<<(ok?"PASS":"FAIL")<<'\n';
    double netFirst=segmentMomentum(0,2,-1);ok=ok&&netFirst!=correct;logs.controls<<"negative_net_first,momentum,"<<netFirst<<','<<correct<<','<<std::abs(netFirst-correct)<<','<<(netFirst!=correct?"DETECTED":"FAIL")<<'\n';
    double m=.5*(1+9),P=.5*(1*1+9*0),velocity=P/m,wrong=.5*(1+0);ok=ok&&velocity==.1&&wrong!=velocity;
    logs.controls<<"unequal_branch_mass,velocity,"<<velocity<<",0.1,"<<wrong<<','<<(velocity==.1&&wrong!=velocity?"PASS":"FAIL")<<'\n';
    // Source-only analytical witness: opposite mass sources must use one frozen
    // velocity. This is a negative control, never passed into transport.
    double frozen=.73*(.25-.25),wrongCurrent=.73*.25+.4*(-.25);ok=ok&&frozen==0&&wrongCurrent!=0;
    logs.controls<<"frozen_source_cancellation,momentum_source,"<<frozen<<",0,"<<wrongCurrent<<','<<(frozen==0&&wrongCurrent!=0?"PASS":"FAIL")<<'\n';
    return ok;
}
static bool covariance(Logs&logs){
    int n=24;Grid base(n,n,1.0/n,1.0/n);base.periodicX=base.periodicY=true;slab(base,.19,.54,.29,.66);setUniformVelocity(base,.17,-.11);MomentumState original(base,1000,1);auto w=smooth(base,original);double dt=.4/cumulativeCourant(base,1);auto reference=run("covariance_base",base,1000,w,dt,12,logs);if(!reference.pass)return false;
    for(int transform=0;transform<2;++transform){
        bool swap=transform==0;Grid g(n,n,base.dx,base.dy);g.periodicX=g.periodicY=true;
        auto sourceCell=[&](int i,int j){return swap?base.C(j,i):base.C(n-1-i,j);};
        for(int j=0;j<n;++j)for(int i=0;i<n;++i)g.c[g.C(i,j)]=base.c[sourceCell(i,j)];
        for(int j=0;j<n;++j)for(int i=0;i<=n;++i)g.u[g.U(i,j)]=swap?base.v[base.V(j,i)]:-base.u[base.U(n-i,j)];
        for(int j=0;j<=n;++j)for(int i=0;i<n;++i)g.v[g.V(i,j)]=swap?base.u[base.U(j,i)]:base.v[base.V(n-1-i,j)];
        MomentumState transformed(g,1000,1);Velocities initial;double uref=1;
        for(int q=0;q<2;++q){int sourceQ=swap?1-q:q;double sign=(!swap&&q==0)?-1:1;initial[q].resize(transformed.layout[q].cells.size());for(size_t k=0;k<initial[q].size();++k){size_t f=transformed.layout[q].cells[k][0],old=sourceCell(int(f%size_t(n)),int(f/size_t(n)));size_t owner=original.layout[sourceQ].owner[old];initial[q][k]=sign*w[sourceQ][owner];uref=std::max(uref,std::abs(initial[q][k]));}}
        std::string name=swap?"covariance_axis_exchange":"covariance_x_reflection";auto result=run(name,g,1000,initial,dt,12,logs);if(!result.pass)return false;
        double fractionError=0,velocityError=0;for(int j=0;j<n;++j)for(int i=0;i<n;++i)fractionError=std::max(fractionError,std::abs(result.fraction[g.C(i,j)]-reference.fraction[sourceCell(i,j)]));
        for(int q=0;q<2;++q){int sourceQ=swap?1-q:q;double sign=(!swap&&q==0)?-1:1;for(size_t k=0;k<result.velocity[q].size();++k){size_t f=transformed.layout[q].cells[k][0],old=sourceCell(int(f%size_t(n)),int(f/size_t(n)));velocityError=std::max(velocityError,std::abs(result.velocity[q][k]-sign*reference.velocity[sourceQ][original.layout[sourceQ].owner[old]]));}}
        bool pass=fractionError<=roundoffAlphaTolerance&&velocityError<=1e-8*uref;logs.controls<<name<<",fraction_covariance,"<<fractionError<<",0,"<<roundoffAlphaTolerance<<','<<(pass?"PASS":"FAIL")<<'\n'<<name<<",velocity_covariance,"<<velocityError<<",0,"<<1e-8*uref<<','<<(pass?"PASS":"FAIL")<<'\n';
        std::printf("P1-C-M %s fraction_error=%.17g velocity_error=%.17g %s\n",name.c_str(),fractionError,velocityError,pass?"PASS":"FAIL");if(!pass)return false;
    }return true;
}
static int execute(){
    std::filesystem::create_directories("evidence/p1cm/current");Logs logs;if(!controls(logs)){std::fprintf(stderr,"P1-C-M STOP: analytic negative control failed\n");return 1;}
    double maxOldMassWrong=0,maxVelocityMeanWrong=0;
    // 18 cases reproduce the reference's categories and invariants, not its
    // independent reconstruction or NumPy RNG bit pattern.
    for(bool nonuniform:{false,true})for(double rho:{1.,1000.,1000000.})for(int mode=0;mode<3;++mode){
        Grid g(24,24,1.0/24,1.0/24);g.periodicX=g.periodicY=true;mixedFractions(g);auto flow=coarseFlow(g,nonuniform);
        if(flow.prolongationError>1e-12||flow.divergenceError>1e-12){std::fprintf(stderr,"P1-C-M STOP: prolongation divergence identity\n");return 1;}
        std::string prefix=nonuniform?"reference_streamfunction_":"reference_uniform_",label=mode==0?"constant":mode==1?"smooth":"seeded_random";
        logs.controls<<prefix+label<<",prolongation_fine_parent_divergence,"<<flow.divergenceError<<",0,"<<flow.prolongationError<<",PASS\n";
        MomentumState m(g,rho,1);Velocities w;if(mode==0)w=constant(m,.73,-.42);else if(mode==1)w=referenceSmooth(g,m,flow);else{Random rng;rng.state=32032;for(int q=0;q<2;++q){w[q].resize(m.layout[q].cells.size());for(double&v:w[q])v=2*rng.unit()-1;}}
        auto r=run(prefix+label,g,rho,w,.4/cumulativeCourant(g,1),12,logs,mode==0);maxOldMassWrong=std::max(maxOldMassWrong,r.oldMassWrong);maxVelocityMeanWrong=std::max(maxVelocityMeanWrong,r.velocityMeanWrong);if(!r.pass)return 1;
    }
    for(double rho:{1.,1000.,1000000.}){Grid g(24,24,1.0/24,1.0/24);g.periodicX=g.periodicY=true;slab(g,.20,.50,.30,.60);setUniformVelocity(g,.37,-.21);MomentumState m(g,rho,1);auto r=run("genuine_common_comotion",g,rho,constant(m,.37,-.21),.4/cumulativeCourant(g,1),12,logs,true);if(!r.pass)return 1;maxOldMassWrong=std::max(maxOldMassWrong,r.oldMassWrong);}
    double prevX=std::numeric_limits<double>::infinity(),prevY=prevX;
    for(int n:{24,48,96}){Grid g(n,n,1.0/n,1.0/n);g.periodicX=g.periodicY=true;slab(g,.2,.5,.3,.6);setUniformVelocity(g,.37,-.21);MomentumState m(g,1,1);double T=.5;int steps=int(std::ceil(T*cumulativeCourant(g,1)/.4));auto r=run("smooth_momentum_refinement",g,1,smooth(g,m),T/steps,steps,logs,false,true,smoothAverage);if(!r.pass||!(r.l1x<prevX)||!(r.l1y<prevY)){std::fprintf(stderr,"P1-C-M STOP: momentum error did not decrease\n");return 1;}prevX=r.l1x;prevY=r.l1y;}
    for(double rho:{1.,1000.,1000000.}){
        Grid g(48,48,1.0/48,1.0/48);g.periodicY=true;slab(g,.8,1);setUniformVelocity(g,.3,0);MomentumState m(g,rho,1);double T=.5;int steps=int(std::ceil(T*cumulativeCourant(g,1)/.4));auto r=run("open_boundary_slab",g,rho,constant(m,.3,0),T/steps,steps,logs,true,false);if(!r.pass)return 1;
        double expectedMass=1+.05*(rho-1),expectedExport=.15*(rho-1),massScale=std::max(1.,1+.2*(rho-1));double retainedError=std::abs(r.liquidVolume-.05),massError=std::abs(r.finalMass-expectedMass)/massScale,exportError=std::abs(r.massExport-expectedExport)/massScale,momentumError=std::abs(r.pExport[0]-.3*expectedExport)/massScale;
        bool pass=retainedError<=1e-11&&massError<=1e-11&&exportError<=1e-11&&momentumError<=1e-11&&std::abs(r.finalP[1])<=1e-11*massScale;
        logs.controls<<"open_boundary_slab_"<<rho<<",retained_liquid,"<<r.liquidVolume<<",0.05,"<<retainedError<<','<<(pass?"PASS":"FAIL")<<'\n'<<"open_boundary_slab_"<<rho<<",net_mass_export,"<<r.massExport<<','<<expectedExport<<','<<exportError<<','<<(pass?"PASS":"FAIL")<<'\n'<<"open_boundary_slab_"<<rho<<",net_px_export,"<<r.pExport[0]<<','<<.3*expectedExport<<','<<momentumError<<','<<(pass?"PASS":"FAIL")<<'\n';
        if(!pass){std::fprintf(stderr,"P1-C-M STOP: independent open-slab oracle\n");return 1;}
    }
    if(!covariance(logs))return 1;
    bool oldDetected=maxOldMassWrong>1e-8,meanDetected=maxVelocityMeanWrong>1e-8;logs.controls<<"negative_old_mass,actual_transport_error,"<<maxOldMassWrong<<",0,1e-8,"<<(oldDetected?"DETECTED":"FAIL")<<'\n'<<"negative_velocity_mean,actual_transport_error,"<<maxVelocityMeanWrong<<",0,1e-8,"<<(meanDetected?"DETECTED":"FAIL")<<'\n';
    if(!oldDetected||!meanDetected){std::fprintf(stderr,"P1-C-M STOP: actual negative controls were not discriminating\n");return 1;}
    std::printf("P1-C-M momentum validation PASS; no pressure/body integration performed\n");return 0;
}
} // namespace momentum_validation
static int runMomentumValidation(){return momentum_validation::execute();}
} // namespace r1p1
