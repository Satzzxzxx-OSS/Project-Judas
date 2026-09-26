#pragma once
// Included inside r1p1 after the accepted fraction operator declarations.
// No PLIC/phase-flux reconstruction occurs in this file.
struct DualGrid {
    std::vector<std::vector<size_t>> cells;
    std::vector<int> owner;
    std::vector<double> volume;
    DualGrid()=default;
    DualGrid(const Grid&g,int q):owner(g.c.size(),-1){
        if(g.nx%2||g.ny%2)throw std::invalid_argument("even fine counts required");
        int nx=g.nx/2+(q==0&&!g.periodicX),ny=g.ny/2+(q==1&&!g.periodicY);
        for(int J=0;J<ny;++J)for(int I=0;I<nx;++I){
            std::vector<size_t> part;
            for(int dj=0;dj<2;++dj)for(int di=0;di<2;++di){
                int i=2*I-(q==0)+di,j=2*J-(q==1)+dj;
                if(g.periodicX)i=(i%g.nx+g.nx)%g.nx;
                if(g.periodicY)j=(j%g.ny+g.ny)%g.ny;
                if(i<0||i>=g.nx||j<0||j>=g.ny)continue;
                size_t k=g.C(i,j);if(owner[k]!=-1)throw std::logic_error("dual overlap");
                owner[k]=int(cells.size());part.push_back(k);
            }
            volume.push_back(part.size()*g.dx*g.dy);cells.push_back(part);
        }
        for(int k:owner)if(k<0)throw std::logic_error("dual partition gap");
    }
};
static std::vector<double> fineMass(const Grid&g,const std::vector<double>&c,double rl,double rg){
    std::vector<double> m(c.size());
    for(size_t k=0;k<c.size();++k)m[k]=g.dx*g.dy*(rg+(rl-rg)*c[k]);
    return m;
}
static std::vector<double> restrictValues(const DualGrid&d,const std::vector<double>&a){
    std::vector<double> out(d.cells.size());
    for(size_t k=0;k<out.size();++k)for(size_t f:d.cells[k])out[k]+=a[f];
    return out;
}
static double sumValues(const std::vector<double>&a){double out=0;for(double x:a)out+=x;return out;}
static double unsignedSum(const std::vector<double>&a){double out=0;for(double x:a)out+=std::abs(x);return out;}
static double segmentMomentum(double fm,double leftVelocity,double rightVelocity){
    return fm>0?fm*leftVelocity:(fm<0?fm*rightVelocity:0.);
}
struct MomentumState {
    double rho_l,rho_g,Uref=1,initialMass=0,accumulatedMassBoundary=0,accumulatedMassBoundaryAbs=0;
    std::array<DualGrid,2> layout;
    std::array<std::vector<double>,2> P,initialVelocity;
    std::array<bool,2> isConstant{{false,false}};
    std::array<double,2> boundaryVelocity{{0,0}},initialP{{0,0}},initialUnsignedP{{0,0}},accumulatedPBoundary{{0,0}},accumulatedPBoundaryAbs{{0,0}};
    MomentumState(const Grid&g,double rl,double rg):rho_l(rl),rho_g(rg),layout{{DualGrid(g,0),DualGrid(g,1)}}{
        if(!(rl>0&&rg>0&&std::isfinite(rl)&&std::isfinite(rg)))throw std::invalid_argument("positive finite phase densities required");
    }
    void initialize(const Grid&g,const std::array<std::vector<double>,2>&w){
        auto fm=fineMass(g,g.c,rho_l,rho_g);initialMass=sumValues(fm);initialVelocity=w;
        for(int q=0;q<2;++q){
            auto m=restrictValues(layout[q],fm);if(w[q].size()!=m.size())throw std::invalid_argument("velocity shape");
            P[q].resize(m.size());isConstant[q]=true;
            for(size_t k=0;k<m.size();++k){
                if(!(m[k]>0&&std::isfinite(w[q][k])))throw std::invalid_argument("invalid initial mass/velocity");
                P[q][k]=m[k]*w[q][k];Uref=std::max(Uref,std::abs(w[q][k]));
                isConstant[q]=isConstant[q]&&(w[q][k]==w[q][0]);
            }
            initialP[q]=sumValues(P[q]);initialUnsignedP[q]=unsignedSum(P[q]);
        }
    }
};
struct MomentumDiagnostics {
    bool valid=false;std::string failure;
    double fineMassRaw=0,fineMassNorm=0,dualMassRaw=0,dualMassNorm=0,combinationMassRaw=0,combinationMassNorm=0;
    double massResidualRaw=0,massResidualNorm=0,massBoundary=0,massBoundaryAbs=0,massSource=0;
    std::array<double,2> momentumResidualRaw{{0,0}},momentumResidualNorm{{0,0}},pBoundary{{0,0}},pBoundaryAbs{{0,0}},pSource{{0,0}},commonVectorError{{0,0}};
    std::array<double,2> velocityMin{{INFINITY,INFINITY}},velocityMax{{-INFINITY,-INFINITY}};
    double kineticBefore=0,kineticAfter=0,meanBranchEnergy=0,combinationEnergyRaw=0,combinationEnergyNorm=0,fullStepEnergyGrowthNorm=0;
    double minMass=INFINITY,wrongOldMassError=0,wrongVelocityMeanError=0,sourceCancellationRaw=0;
};
struct MassStageRecord {
    Axis axis;std::vector<double> massIn,massOut,source,transfer;
};
// Traverse unique fine-face segments. Periodic endpoint duplicates are skipped.
// Callback receives physical fine cells or -1 for the exterior (not physical mass).
template<class F> static void fineFaces(const Grid&g,Axis axis,F f){
    if(axis==Axis::X){
        for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx+(g.periodicX?0:1);++i){
            int l=i-1,r=i;if(g.periodicX){l=(l+g.nx)%g.nx;r%=g.nx;}
            f(g.U(i,j),l>=0?int(g.C(l,j)):-1,r<g.nx?int(g.C(r,j)):-1);
        }
    }else{
        for(int j=0;j<g.ny+(g.periodicY?0:1);++j)for(int i=0;i<g.nx;++i){
            int b=j-1,t=j;if(g.periodicY){b=(b+g.ny)%g.ny;t%=g.ny;}
            f(g.V(i,j),b>=0?int(g.C(i,b)):-1,t<g.ny?int(g.C(i,t)):-1);
        }
    }
}
static double prolongMAC(Grid&g,const std::vector<double>&U,const std::vector<double>&V){
    int nx=g.nx/2,ny=g.ny/2;
    if(g.nx%2||g.ny%2||U.size()!=size_t((nx+1)*ny)||V.size()!=size_t(nx*(ny+1)))throw std::invalid_argument("coarse MAC shape");
    auto ui=[&](int i,int j){return U[size_t(j*(nx+1)+i)];};
    auto vi=[&](int i,int j){return V[size_t(j*nx+i)];};
    for(int j=0;j<g.ny;++j)for(int i=0;i<=g.nx;++i)
        g.u[g.U(i,j)]=i%2?.5*(ui(i/2,j/2)+ui(i/2+1,j/2)):ui(i/2,j/2);
    for(int j=0;j<=g.ny;++j)for(int i=0;i<g.nx;++i)
        g.v[g.V(i,j)]=j%2?.5*(vi(i/2,j/2)+vi(i/2,j/2+1)):vi(i/2,j/2);
    double error=0;
    for(int j=0;j<g.ny;++j)for(int i=0;i<g.nx;++i){
        double coarse=(ui(i/2+1,j/2)-ui(i/2,j/2))/(2*g.dx)+(vi(i/2,j/2+1)-vi(i/2,j/2))/(2*g.dy);
        double fine=(g.u[g.U(i+1,j)]-g.u[g.U(i,j)])/g.dx+(g.v[g.V(i,j+1)]-g.v[g.V(i,j)])/g.dy;
        error=std::max(error,std::abs(fine-coarse));
    }
    return error;
}
static MomentumDiagnostics advanceMomentum(const Grid&g,const TransportRecord&r,double dt,MomentumState&m){
    MomentumDiagnostics d;
    const double V=g.dx*g.dy,domain=g.nx*g.ny*V;
    auto fail=[&](const std::string&reason){d.failure=reason;return d;};
    if(r.cn.size()!=g.c.size())return fail("missing actual phase transfers");
    auto initialFine=fineMass(g,r.cn,m.rho_l,m.rho_g),finalFine=fineMass(g,r.cfinal,m.rho_l,m.rho_g);
    std::array<std::vector<double>,2> M0,frozenW;
    for(int q=0;q<2;++q){
        M0[q]=restrictValues(m.layout[q],initialFine);frozenW[q].resize(M0[q].size());
        if(m.P[q].size()!=M0[q].size())return fail("momentum not initialized");
        for(size_t k=0;k<M0[q].size();++k){
            if(!(M0[q][k]>0))return fail("nonpositive initial dual mass");
            frozenW[q][k]=m.P[q][k]/M0[q][k];
            d.kineticBefore+=.5*m.P[q][k]*frozenW[q][k];
        }
    }
    struct Branch {
        std::array<std::vector<double>,2> P,M,absoluteTransfers;
        double massBoundary=0,massBoundaryAbs=0,massSource=0;
        std::array<double,2> pBoundary{{0,0}},pBoundaryAbs{{0,0}},pSource{{0,0}};
    };
    std::array<Branch,2> branches;
    for(int b=0;b<2;++b){
        auto&branch=branches[b];branch.P=m.P;
        for(int q=0;q<2;++q)branch.absoluteTransfers[q].assign(M0[q].size(),0);
        for(int stage=0;stage<2;++stage){
            Axis axis=(b==0?(stage==0?Axis::X:Axis::Y):(stage==0?Axis::Y:Axis::X));
            const auto& cin=stage==0?r.cn:(b==0?r.cx:r.cy);
            const auto& cout=stage==0?(b==0?r.cx:r.cy):(b==0?r.xThenY:r.yThenX);
            const Fluxes& phase=stage==0?(b==0?r.fxn:r.fyn):(b==0?r.fyAfterX:r.fxAfterY);
            const auto& ql=axis==Axis::X?phase.x:phase.y;
            MassStageRecord rec{axis,fineMass(g,cin,m.rho_l,m.rho_g),fineMass(g,cout,m.rho_l,m.rho_g),std::vector<double>(g.c.size()),std::vector<double>(ql.size())};
            std::vector<double> net(g.c.size(),0),absolute(g.c.size(),0),netVolume(g.c.size(),0);
            fineFaces(g,axis,[&](size_t f,int l,int rr){
                double qt=(axis==Axis::X?g.u[f]:g.v[f])*dt*(axis==Axis::X?g.dy:g.dx);
                double fm=m.rho_l*ql[f]+m.rho_g*(qt-ql[f]);rec.transfer[f]=fm;
                if(l>=0){net[size_t(l)]+=fm;absolute[size_t(l)]+=std::abs(fm);netVolume[size_t(l)]+=qt;}
                if(rr>=0){net[size_t(rr)]-=fm;absolute[size_t(rr)]+=std::abs(fm);netVolume[size_t(rr)]-=qt;}
                if(l<0||rr<0){branch.massBoundary+=(rr<0?fm:-fm);branch.massBoundaryAbs+=std::abs(fm);}
            });
            for(size_t k=0;k<g.c.size();++k){
                // rho_g is part of the frozen source density, even in pure gas.
                rec.source[k]=V*(m.rho_g+(m.rho_l-m.rho_g)*r.frozen[k])*(netVolume[k]/V);
                branch.massSource+=rec.source[k];
                double raw=std::abs(rec.massOut[k]-(rec.massIn[k]-net[k]+rec.source[k]));
                double scale=std::max({m.rho_g*V,std::abs(rec.massIn[k]),std::abs(rec.massOut[k]),absolute[k],std::abs(rec.source[k])});
                d.fineMassRaw=std::max(d.fineMassRaw,raw);d.fineMassNorm=std::max(d.fineMassNorm,raw/scale);
                d.minMass=std::min(d.minMass,rec.massOut[k]);
                if(!(rec.massOut[k]>0&&std::isfinite(rec.massOut[k])))return fail("nonpositive/nonfinite fine mass");
                if(!(raw/scale<=1e-12))return fail("fine directional mass ledger branch="+std::to_string(b)+" stage="+std::to_string(stage)+" cell="+std::to_string(k));
            }
            for(int q=0;q<2;++q){
                const auto&layout=m.layout[q];
                auto before=restrictValues(layout,rec.massIn),after=restrictValues(layout,rec.massOut),source=restrictValues(layout,rec.source);
                std::vector<double> netM(before.size(),0),absM(before.size(),0),netP(before.size(),0),velocity(before.size());
                for(size_t k=0;k<before.size();++k)velocity[k]=branch.P[q][k]/before[k];
                fineFaces(g,axis,[&](size_t f,int l,int rr){
                    int left=l<0?-1:layout.owner[size_t(l)],right=rr<0?-1:layout.owner[size_t(rr)];
                    if(left==right)return; // Internal segment cancels inside this dual volume.
                    double fm=rec.transfer[f];
                    double wl=left<0?m.boundaryVelocity[q]:velocity[size_t(left)];
                    double wr=right<0?m.boundaryVelocity[q]:velocity[size_t(right)];
                    // Per-segment upwind FIRST; never upwind the summed dual-face mass.
                    double fp=segmentMomentum(fm,wl,wr);
                    if(left>=0){netM[size_t(left)]+=fm;absM[size_t(left)]+=std::abs(fm);netP[size_t(left)]+=fp;}
                    if(right>=0){netM[size_t(right)]-=fm;absM[size_t(right)]+=std::abs(fm);netP[size_t(right)]-=fp;}
                    if(left<0||right<0){branch.pBoundary[q]+=(right<0?fp:-fp);branch.pBoundaryAbs[q]+=std::abs(fp);}
                });
                for(size_t k=0;k<before.size();++k){
                    double raw=std::abs(after[k]-(before[k]-netM[k]+source[k]));
                    double scale=std::max({m.rho_g*layout.volume[k],std::abs(before[k]),std::abs(after[k]),absM[k],std::abs(source[k])});
                    d.dualMassRaw=std::max(d.dualMassRaw,raw);d.dualMassNorm=std::max(d.dualMassNorm,raw/scale);
                    branch.absoluteTransfers[q][k]+=absM[k]+std::abs(source[k]);
                    if(!(raw/scale<=1e-12))return fail("dual directional mass ledger branch="+std::to_string(b)+" stage="+std::to_string(stage)+" q="+std::to_string(q)+" dual="+std::to_string(k));
                    double sp=frozenW[q][k]*source[k];branch.pSource[q]+=sp;
                    branch.P[q][k]=branch.P[q][k]-netP[k]+sp;
                    double w=branch.P[q][k]/after[k];
                    if(!(after[k]>0&&std::isfinite(w)))return fail("nonpositive/nonfinite dual state");
                    d.velocityMin[q]=std::min(d.velocityMin[q],w);d.velocityMax[q]=std::max(d.velocityMax[q],w);
                    if(m.isConstant[q])d.commonVectorError[q]=std::max(d.commonVectorError[q],std::abs(w-m.initialVelocity[q][0]));
                }
                branch.M[q]=after;
            }
        }
    }
    auto newP=m.P;
    for(int q=0;q<2;++q){
        auto finalM=restrictValues(m.layout[q],finalFine);
        for(size_t k=0;k<finalM.size();++k){
            double meanM=.5*(branches[0].M[q][k]+branches[1].M[q][k]);
            double meanP=.5*(branches[0].P[q][k]+branches[1].P[q][k]);
            double raw=std::abs(meanM-finalM[k]);
            double scale=std::max({m.rho_g*m.layout[q].volume[k],std::abs(M0[q][k]),std::abs(finalM[k]),.5*(branches[0].absoluteTransfers[q][k]+branches[1].absoluteTransfers[q][k])});
            d.combinationMassRaw=std::max(d.combinationMassRaw,raw);d.combinationMassNorm=std::max(d.combinationMassNorm,raw/scale);
            if(!(raw/scale<=1e-12))return fail("final restricted mass / branch mean q="+std::to_string(q)+" dual="+std::to_string(k));
            if(!(finalM[k]>0))return fail("nonpositive final dual mass");
            double w=meanP/finalM[k];if(!std::isfinite(w))return fail("nonfinite final velocity");
            newP[q][k]=meanP;
            d.kineticAfter+=.5*meanP*w;
            for(int b=0;b<2;++b)d.meanBranchEnergy+=.25*branches[b].P[q][k]*branches[b].P[q][k]/branches[b].M[q][k];
            d.velocityMin[q]=std::min(d.velocityMin[q],w);d.velocityMax[q]=std::max(d.velocityMax[q],w);
            if(m.isConstant[q])d.commonVectorError[q]=std::max(d.commonVectorError[q],std::abs(w-m.initialVelocity[q][0]));
            d.wrongOldMassError=std::max(d.wrongOldMassError,std::abs(meanP/M0[q][k]-w));
            double wrong=.5*(branches[0].P[q][k]/branches[0].M[q][k]+branches[1].P[q][k]/branches[1].M[q][k]);
            d.wrongVelocityMeanError=std::max(d.wrongVelocityMeanError,std::abs(wrong-w));
        }
        d.pBoundary[q]=.5*(branches[0].pBoundary[q]+branches[1].pBoundary[q]);
        d.pBoundaryAbs[q]=.5*(branches[0].pBoundaryAbs[q]+branches[1].pBoundaryAbs[q]);
        d.pSource[q]=.5*(branches[0].pSource[q]+branches[1].pSource[q]);
        d.sourceCancellationRaw=std::max(d.sourceCancellationRaw,std::abs(d.pSource[q]));
        d.momentumResidualRaw[q]=sumValues(newP[q])-m.initialP[q]+m.accumulatedPBoundary[q]+d.pBoundary[q];
        double scale=std::max({m.initialUnsignedP[q],m.accumulatedPBoundaryAbs[q]+d.pBoundaryAbs[q],m.rho_g*domain*m.Uref});
        d.momentumResidualNorm[q]=std::abs(d.momentumResidualRaw[q])/scale;
    }
    d.massBoundary=.5*(branches[0].massBoundary+branches[1].massBoundary);
    d.massBoundaryAbs=.5*(branches[0].massBoundaryAbs+branches[1].massBoundaryAbs);
    d.massSource=.5*(branches[0].massSource+branches[1].massSource);
    d.massResidualRaw=sumValues(finalFine)-m.initialMass+m.accumulatedMassBoundary+d.massBoundary;
    d.massResidualNorm=std::abs(d.massResidualRaw)/std::max({m.initialMass,m.accumulatedMassBoundaryAbs+d.massBoundaryAbs,m.rho_g*domain});
    double energyScale=std::max(d.meanBranchEnergy,m.rho_g*domain*m.Uref*m.Uref);
    d.combinationEnergyRaw=d.kineticAfter-d.meanBranchEnergy;
    d.combinationEnergyNorm=d.combinationEnergyRaw/energyScale;
    d.fullStepEnergyGrowthNorm=(d.kineticAfter-d.kineticBefore)/std::max(d.kineticBefore,m.rho_g*domain*m.Uref*m.Uref);
    if(!(d.massResidualNorm<=1e-11&&d.momentumResidualNorm[0]<=1e-11&&d.momentumResidualNorm[1]<=1e-11))return fail("global mass/momentum budget");
    if(!(d.combinationEnergyNorm<=1e-12))return fail("branch combination energy");
    for(int q=0;q<2;++q)if(d.commonVectorError[q]>1e-8*m.Uref)return fail("constant-vector transport");
    if(g.periodicX&&g.periodicY&&d.fullStepEnergyGrowthNorm>1e-11)return fail("closed full-step energy growth requires investigation");
    m.P=newP;m.accumulatedMassBoundary+=d.massBoundary;m.accumulatedMassBoundaryAbs+=d.massBoundaryAbs;
    for(int q=0;q<2;++q){m.accumulatedPBoundary[q]+=d.pBoundary[q];m.accumulatedPBoundaryAbs[q]+=d.pBoundaryAbs[q];}
    d.valid=true;return d;
}
