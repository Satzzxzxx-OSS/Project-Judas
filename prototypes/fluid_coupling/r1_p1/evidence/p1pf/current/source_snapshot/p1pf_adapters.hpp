#pragma once
// P1-PF adapters only, included after momentum_validation.hpp. All transport
// below calls actual Grid/jsgtStep/advanceMomentum; no alternate reconstruction.
#include "frozen_projection.hpp"
#include <cstring>

namespace r1p1 {
namespace p1pf_adapters {
static bool byteEqual(const std::vector<double>&a,const std::vector<double>&b){return a.size()==b.size()&&(a.empty()||std::memcmp(a.data(),b.data(),a.size()*sizeof(double))==0);}
static double maxDifference(const std::vector<double>&a,const std::vector<double>&b){if(a.size()!=b.size())throw std::invalid_argument("adapter vector dimensions");double e=0;for(size_t k=0;k<a.size();++k)e=std::max(e,std::abs(a[k]-b[k]));return e;}
static void require(bool condition,const std::string&message){if(!condition)throw std::runtime_error(message);}
struct Logs {
    std::ofstream summary,transport,fractions,cells,faces,fine,advector,singular,matrix;
    Logs():summary("evidence/p1pf/current/adapters_summary.csv"),transport("evidence/p1pf/current/adapters_transport_steps.csv"),fractions("evidence/p1pf/current/adapter_fraction_trace.csv"),cells("evidence/p1pf/current/adapters_cells.csv"),faces("evidence/p1pf/current/adapters_faces.csv"),fine("evidence/p1pf/current/adapters_fine.csv"),advector("evidence/p1pf/current/adapters_advector.csv"),singular("evidence/p1pf/current/adapters_singular_values.csv"),matrix("evidence/p1pf/current/adapters_matrix.csv"){
        for(std::ofstream*f:{&summary,&transport,&fractions,&cells,&faces,&fine,&advector,&singular,&matrix}){if(!*f)throw std::runtime_error("cannot open adapter evidence output");*f<<std::setprecision(17);}
        csvHeader(fractions);
        summary<<"name,density_ratio,fine_n,MAC_n,transport_steps,dt,CFL_sum,pressure_cells,velocity_dofs,rank,nullity,rank_threshold,sigma_max,sigma_min_retained,compatibility_raw,compatibility_normalized,constraint_raw,constraint_normalized,impulse_raw,impulse_normalized,energy_raw,energy_normalized,kinetic_before,kinetic_after,dissipation,px_before,px_after,px_budget_raw,px_budget_normalized,py_before,py_after,py_budget_raw,py_budget_normalized,max_cell_divergence,max_face_impulse_mismatch,mass_partition_raw,mass_partition_normalized,idempotence_error,second_constraint_residual,second_impulse_residual,second_energy_residual,common_velocity_error,projection_correction,min_fraction,max_fraction,fraction_bytes_unchanged,mass_bytes_unchanged,advector_bytes_unchanged,transport_momentum_bytes_unchanged,prolongation_error,prolongation_divergence_error,runtime_s,status\n";
        transport<<"name,step,dt,CFL_sum,min_fraction,max_fraction,min_mass,fine_mass_norm,dual_mass_norm,combination_mass_norm,mass_budget_norm,px_budget_norm,py_budget_norm,common_x_error,common_y_error,kinetic_before,kinetic_after,combination_energy_norm,full_step_energy_growth_norm,advector_bytes_unchanged,status\n";
        cells<<"name,I,J,integrated_divergence_before,integrated_divergence_after,divergence_before,divergence_after,pressure_impulse,raw_constraint,second_raw_constraint\n";
        faces<<"name,component,I,J,dof,actual_dual_mass,momentum_before,velocity_before,velocity_after,momentum_after,momentum_change,transpose_constraint_impulse,impulse_mismatch,second_velocity,idempotence_difference\n";
        fine<<"name,i,j,fraction_before_projection,fraction_after_projection,actual_mass_before_projection,actual_mass_after_projection\n";
        advector<<"name,component,array_index,before_transport,after_transport,after_projection\n";
        singular<<"name,index,singular_value,rank_threshold,retained\n";
        matrix<<"name,row,dof,coefficient\n";
    }
};
// Audit actual periodic dual ordering before interpreting component arrays as
// MAC arrays. Each component partitions all fine mass; seam DOFs are unique.
static void verifyPeriodicLayout(const Grid&g,const MomentumState&m){
    require(g.periodicX&&g.periodicY,"adapter requires both periodic directions");
    require(momentum_validation::layoutAudit(g,m),"actual transport dual partition audit");
    const int nx=g.nx/2,ny=g.ny/2;
    for(int q=0;q<2;++q){require(m.layout[q].cells.size()==size_t(nx*ny),"periodic seam DOF duplicated");
        for(int J=0;J<ny;++J)for(int I=0;I<nx;++I){std::vector<size_t>expected;
            for(int dj=0;dj<2;++dj)for(int di=0;di<2;++di){int i=(2*I-(q==0)+di+g.nx)%g.nx,j=(2*J-(q==1)+dj+g.ny)%g.ny;expected.push_back(g.C(i,j));}
            require(m.layout[q].cells[size_t(J*nx+I)]==expected,"actual dual/MAC index mismatch");
        }
    }
}
static void runOne(double densityRatio,bool nonuniform,Logs&logs){
    const auto start=std::chrono::steady_clock::now();constexpr int n=24,nc=12,steps=12,N=nc*nc;
    const std::string name=std::string(nonuniform?"nonuniform_transport_snapshot":"common_comotion")+"_ratio_"+std::to_string(int(densityRatio));
    Grid g(n,n,1.0/n,1.0/n);g.periodicX=g.periodicY=true;
    // Reference's binary sharp disk; not a subcell circle oracle or mixed fractions.
    for(int j=0;j<n;++j)for(int i=0;i<n;++i){double x=(i+.5)*g.dx,y=(j+.5)*g.dy;g.c[g.C(i,j)]=(x-.48)*(x-.48)+(y-.51)*(y-.51)<.24*.24?1.:0.;}
    const auto flow=momentum_validation::coarseFlow(g,nonuniform);
    require(flow.prolongationError<=1e-12&&flow.divergenceError<=1e-12,"adapter MAC prolongation identity");
    MomentumState m(g,densityRatio,1.);verifyPeriodicLayout(g,m);
    const auto initialVelocity=momentum_validation::referenceSmooth(g,m,flow);m.initialize(g,initialVelocity);
    const auto frozenU=g.u,frozenV=g.v;const double dt=.4/cumulativeCourant(g,1.);
    require(std::abs(cumulativeCourant(g,dt)-.4)<1e-14,"adapter cumulative Courant .4");
    for(int step=0;step<steps;++step){
        TransportRecord record;auto fraction=jsgtStep(g,dt,name,step,{},{},{},logs.fractions,{},&record);
        require(fraction.valid,"adapter actual fraction transport: "+fraction.failure);
        const auto d=advanceMomentum(g,record,dt,m);const bool frozen=byteEqual(g.u,frozenU)&&byteEqual(g.v,frozenV);
        const double minC=*std::min_element(g.c.begin(),g.c.end()),maxC=*std::max_element(g.c.begin(),g.c.end());
        logs.transport<<name<<','<<step<<','<<dt<<','<<cumulativeCourant(g,dt)<<','<<minC<<','<<maxC<<','<<d.minMass<<','<<d.fineMassNorm<<','<<d.dualMassNorm<<','<<d.combinationMassNorm<<','<<d.massResidualNorm<<','<<d.momentumResidualNorm[0]<<','<<d.momentumResidualNorm[1]<<','<<d.commonVectorError[0]<<','<<d.commonVectorError[1]<<','<<d.kineticBefore<<','<<d.kineticAfter<<','<<d.combinationEnergyNorm<<','<<d.fullStepEnergyGrowthNorm<<','<<frozen<<','<<(d.valid&&frozen?"PASS":"FAIL")<<'\n';
        require(d.valid,"adapter actual momentum transport: "+d.failure);require(frozen,"transport changed prescribed adapter advector");
    }
    // C is transport's mass authority. These SAME fineMass/restrictValues calls
    // normalize final P inside advanceMomentum. No alternate density path.
    const auto massFine=fineMass(g,g.c,m.rho_l,m.rho_g);
    const std::array<std::vector<double>,2>massDual{{restrictValues(m.layout[0],massFine),restrictValues(m.layout[1],massFine)}};
    const auto fractionBefore=g.c,massBefore=massFine;const auto momentumBefore=m.P;
    const auto advectorBeforeU=g.u,advectorBeforeV=g.v;
    pf::Vec H(2*N),wstar(2*N),rhs(N,0.);
    for(int q=0;q<2;++q)for(int k=0;k<N;++k){size_t z=size_t(q*N+k);H[z]=massDual[q][size_t(k)];wstar[z]=m.P[q][size_t(k)]/H[z];require(H[z]>0&&std::isfinite(wstar[z]),"adapter invalid actual mass/momentum");}
    const auto hBefore=H;pf::Matrix B(N,2*N);const double hx=2*g.dx,hy=2*g.dy;
    for(int j=0;j<nc;++j)for(int i=0;i<nc;++i){const int row=j*nc+i;B(row,j*nc+i)-=hy;B(row,j*nc+(i+1)%nc)+=hy;B(row,N+j*nc+i)-=hx;B(row,N+((j+1)%nc)*nc+i)+=hx;}
    const auto projected=pf::project(B,H,rhs,wstar);
    // Second solve measures idempotence only; neither result changes the advector.
    const auto second=pf::project(B,H,rhs,projected.velocity);
    const auto divergenceBefore=pf::mul(B,wstar),divergenceAfter=pf::mul(B,projected.velocity);
    const auto transposeImpulse=pf::transpose_mul(B,projected.impulses);
    const auto massAfter=fineMass(g,g.c,m.rho_l,m.rho_g);
    const std::array<std::vector<double>,2>dualAfter{{restrictValues(m.layout[0],massAfter),restrictValues(m.layout[1],massAfter)}};
    const bool fractionSame=byteEqual(fractionBefore,g.c);
    const bool massSame=byteEqual(massBefore,massAfter)&&byteEqual(hBefore,H)&&byteEqual(massDual[0],dualAfter[0])&&byteEqual(massDual[1],dualAfter[1]);
    const bool advectorSame=byteEqual(advectorBeforeU,g.u)&&byteEqual(advectorBeforeV,g.v)&&byteEqual(frozenU,g.u)&&byteEqual(frozenV,g.v);
    const bool inputMomentumSame=byteEqual(momentumBefore[0],m.P[0])&&byteEqual(momentumBefore[1],m.P[1]);
    double massPartitionRaw=0,massPartitionNorm=0,commonError=0,maxFaceMismatch=0;
    const long double physicalMass=momentum_validation::sum(massFine);
    std::array<double,2>pBefore{},pAfter{},momentumRaw{},momentumNorm{};
    for(int q=0;q<2;++q){
        const double partition=std::abs(double(momentum_validation::sum(massDual[q])-physicalMass));massPartitionRaw=std::max(massPartitionRaw,partition);massPartitionNorm=std::max(massPartitionNorm,partition/std::max(1.,double(physicalMass)));
        long double after=0;for(int k=0;k<N;++k){size_t z=size_t(q*N+k);after+=static_cast<long double>(H[z])*projected.velocity[z];if(!nonuniform)commonError=std::max(commonError,std::abs(projected.velocity[z]-initialVelocity[q][size_t(k)]));}
        const long double before=momentum_validation::sum(m.P[q]);pBefore[q]=double(before);pAfter[q]=double(after);momentumRaw[q]=double(after-before);momentumNorm[q]=std::abs(momentumRaw[q])/std::max(1.,double(momentum_validation::unsignedSum(m.P[q])));
    }
    for(int j=0;j<nc;++j)for(int i=0;i<nc;++i){size_t r=size_t(j*nc+i);logs.cells<<name<<','<<i<<','<<j<<','<<divergenceBefore[r]<<','<<divergenceAfter[r]<<','<<divergenceBefore[r]/(hx*hy)<<','<<divergenceAfter[r]/(hx*hy)<<','<<projected.impulses[r]<<','<<projected.raw_constraint[r]<<','<<second.raw_constraint[r]<<'\n';}
    for(int q=0;q<2;++q)for(int j=0;j<nc;++j)for(int i=0;i<nc;++i){size_t k=size_t(j*nc+i),z=size_t(q*N)+k;double pn=projected.momenta[z],dp=pn-m.P[q][k],mismatch=dp-transposeImpulse[z];maxFaceMismatch=std::max(maxFaceMismatch,std::abs(mismatch));logs.faces<<name<<','<<q<<','<<i<<','<<j<<','<<z<<','<<H[z]<<','<<m.P[q][k]<<','<<wstar[z]<<','<<projected.velocity[z]<<','<<pn<<','<<dp<<','<<transposeImpulse[z]<<','<<mismatch<<','<<second.velocity[z]<<','<<second.velocity[z]-projected.velocity[z]<<'\n';}
    for(int j=0;j<n;++j)for(int i=0;i<n;++i){size_t k=g.C(i,j);logs.fine<<name<<','<<i<<','<<j<<','<<fractionBefore[k]<<','<<g.c[k]<<','<<massBefore[k]<<','<<massAfter[k]<<'\n';}
    for(size_t k=0;k<g.u.size();++k)logs.advector<<name<<",0,"<<k<<','<<frozenU[k]<<','<<advectorBeforeU[k]<<','<<g.u[k]<<'\n';
    for(size_t k=0;k<g.v.size();++k)logs.advector<<name<<",1,"<<k<<','<<frozenV[k]<<','<<advectorBeforeV[k]<<','<<g.v[k]<<'\n';
    for(size_t k=0;k<projected.singular_values.size();++k)logs.singular<<name<<','<<k<<','<<projected.singular_values[k]<<','<<projected.rank_threshold<<','<<(projected.singular_values[k]>projected.rank_threshold)<<'\n';
    for(int r=0;r<N;++r)for(int c=0;c<2*N;++c)if(B(r,c)!=0)logs.matrix<<name<<','<<r<<','<<c<<','<<B(r,c)<<'\n';
    const double idempotence=maxDifference(second.velocity,projected.velocity),correction=maxDifference(projected.velocity,wstar),maxDiv=pf::max_abs(divergenceAfter)/(hx*hy);
    const double minC=*std::min_element(g.c.begin(),g.c.end()),maxC=*std::max_element(g.c.begin(),g.c.end());
    const bool pass=projected.nullity==1&&second.nullity==1&&projected.constraint_residual<1e-9&&projected.impulse_residual<1e-9&&projected.energy_identity_residual<1e-9&&second.constraint_residual<1e-9&&second.impulse_residual<1e-9&&second.energy_identity_residual<1e-9&&momentumNorm[0]<1e-9&&momentumNorm[1]<1e-9&&massPartitionNorm<=1e-9&&idempotence<1e-9&&projected.kinetic_after<=projected.kinetic_before+1e-9*std::max(1.,projected.kinetic_before)&&(!nonuniform?commonError<1e-8:true)&&bounded(g,g.c)&&fractionSame&&massSame&&advectorSame&&inputMomentumSame;
    const double runtime=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    logs.summary<<name<<','<<densityRatio<<','<<n<<','<<nc<<','<<steps<<','<<dt<<','<<cumulativeCourant(g,dt)<<','<<N<<','<<2*N<<','<<projected.rank<<','<<projected.nullity<<','<<projected.rank_threshold<<','<<projected.sigma_max<<','<<projected.sigma_min_retained<<','<<projected.compatibility_raw_norm<<','<<projected.rejected_compatibility<<','<<projected.constraint_raw_norm<<','<<projected.constraint_residual<<','<<projected.impulse_raw_norm<<','<<projected.impulse_residual<<','<<projected.energy_raw<<','<<projected.energy_identity_residual<<','<<projected.kinetic_before<<','<<projected.kinetic_after<<','<<projected.dissipation<<','<<pBefore[0]<<','<<pAfter[0]<<','<<momentumRaw[0]<<','<<momentumNorm[0]<<','<<pBefore[1]<<','<<pAfter[1]<<','<<momentumRaw[1]<<','<<momentumNorm[1]<<','<<maxDiv<<','<<maxFaceMismatch<<','<<massPartitionRaw<<','<<massPartitionNorm<<','<<idempotence<<','<<second.constraint_residual<<','<<second.impulse_residual<<','<<second.energy_identity_residual<<','<<commonError<<','<<correction<<','<<minC<<','<<maxC<<','<<fractionSame<<','<<massSame<<','<<advectorSame<<','<<inputMomentumSame<<','<<flow.prolongationError<<','<<flow.divergenceError<<','<<runtime<<','<<(pass?"PASS":"FAIL")<<'\n';
    std::printf("P1-PF adapter %s steps=%d constraint=%.3g impulse=%.3g energy=%.3g momentum=%.3g idempotence=%.3g common=%.3g nullity=%d unchanged=(%d,%d,%d) runtime=%.5gs %s\n",name.c_str(),steps,projected.constraint_residual,projected.impulse_residual,projected.energy_identity_residual,std::max(momentumNorm[0],momentumNorm[1]),idempotence,commonError,projected.nullity,fractionSame,massSame,advectorSame,runtime,pass?"PASS":"FAIL");
    require(pass,"adapter projection/reference numerical gate: "+name);
}
static int execute(){try{std::filesystem::create_directories("evidence/p1pf/current");Logs logs;int cases=0;for(double ratio:{1.,1000.,1000000.})for(bool nonuniform:{false,true}){runOne(ratio,nonuniform,logs);++cases;}std::printf("P1-PF adapters PASS: %d cases, %d actual transport timesteps, %d snapshot projections + %d idempotence solves; no advector feedback or moving geometry\n",cases,cases*12,cases,cases);return 0;}catch(const std::exception&e){std::fprintf(stderr,"P1-PF adapters STOP: %s\n",e.what());return 1;}}
} // namespace p1pf_adapters
static int runP1PFAdapters(){return p1pf_adapters::execute();}
} // namespace r1p1
