#pragma once
// Independent exact flow-map oracle, not a reconstruction or transport operator.
#include "transport_oracles.hpp"
#include <stdexcept>
namespace oracle {
// psi = a*(x-.5)^2*(y-.5), tau=a*t.
// s=s0/(1-tau*s0), q=q0*(1-tau*s0)^2; determinant = 1.
inline double nonlinearFraction(int n,int i,int j,Box initial,Real tau){
    if(tau==0)return rectangleFraction(n,i,j,initial);
    const Real sl=initial.x0-.5L,sr=initial.x1-.5L;
    const Real qb=initial.y0-.5L,qt=initial.y1-.5L;
    if(1-tau*sl<=0||1-tau*sr<=0)throw std::domain_error("flow map reached its singularity");
    const Box c=cell(n,i,j);
    const Real left=std::max(c.x0,.5L+sl/(1-tau*sl));
    const Real right=std::min(c.x1,.5L+sr/(1-tau*sr));
    if(left>=right)return 0;
    auto denominator=[&](Real x){return 1+tau*(x-.5L);};
    auto curve=[&](Real q,Real x){Real d=denominator(x);return .5L+q/(d*d);};
    std::vector<Real> cuts{left,right};
    for(Real q:{qb,qt})for(Real y:{c.y0,c.y1}){
        if(y==.5L||q/(y-.5L)<=0)continue;
        Real x=.5L+(std::sqrt(q/(y-.5L))-1)/tau;
        if(x>left&&x<right)cuts.push_back(x);
    }
    std::sort(cuts.begin(),cuts.end());
    Real area=0;
    for(size_t k=1;k<cuts.size();++k){
        const Real l=cuts[k-1],r=cuts[k],mid=(l+r)/2;
        const Real lower=curve(qb,mid),upper=curve(qt,mid);
        if(std::min(c.y1,upper)<=std::max(c.y0,lower))continue;
        auto integrate=[&](Real q){return .5L*(r-l)+q*(r-l)/(denominator(l)*denominator(r));};
        const Real topIntegral=upper<c.y1?integrate(qt):c.y1*(r-l);
        const Real bottomIntegral=lower>c.y0?integrate(qb):c.y0*(r-l);
        area+=topIntegral-bottomIntegral;
    }
    return double(area*n*n);
}
}
