#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "hydrostatic.hpp"
namespace r1p1 {
struct PressureFace { int left{},right{-1};double area{},distance{},velocityStar{}; };
struct WetPatch { int cell{};Vec2 fluidOutwardNormal{};double area{}; };
struct ProjectionResult {std::vector<double> pressure,faceVelocity;Vec2 bodyVelocity{};CgResult cg;double maxConstraintResidual{},fluidImpulseX{},fluidImpulseY{},bodyImpulseX{},bodyImpulseY{};};
inline ProjectionResult coupledProjection(int n,const std::vector<PressureFace>&faces,const std::vector<WetPatch>&patches,double dt,double rho,double bodyMass,Vec2 bodyVelocityStar){
    std::vector<double> diag(static_cast<size_t>(n),0),rhs(static_cast<size_t>(n),0),p(static_cast<size_t>(n),0);
    std::vector<std::vector<std::pair<int,double>>> off(static_cast<size_t>(n));std::vector<Vec2> s(static_cast<size_t>(n),{0,0});
    for(const auto&w:patches)s[size_t(w.cell)]=s[size_t(w.cell)]+w.fluidOutwardNormal*w.area;
    for(const auto&f:faces){double flux=f.area*f.velocityStar;rhs[size_t(f.left)]-=flux;if(f.right>=0)rhs[size_t(f.right)]+=flux;double k=dt/rho*f.area/f.distance;diag[size_t(f.left)]+=k;if(f.right>=0){diag[size_t(f.right)]+=k;off[size_t(f.left)].push_back({f.right,-k});off[size_t(f.right)].push_back({f.left,-k});}}
    for(int i=0;i<n;++i){rhs[size_t(i)]-=dot(s[size_t(i)],bodyVelocityStar);for(int j=0;j<n;++j)diag[size_t(i)]+=dt/bodyMass*dot(s[size_t(i)],s[size_t(j)]);for(int j=0;j<n;++j)if(i!=j){double k=dt/bodyMass*dot(s[size_t(i)],s[size_t(j)]);if(k!=0)off[size_t(i)].push_back({j,k});}}
    // Merge duplicate sparse entries before PCG so the Jacobi diagonal and
    // matrix-vector product represent the same symmetric operator.
    for(auto&row:off){std::sort(row.begin(),row.end(),[](auto a,auto b){return a.first<b.first;});std::vector<std::pair<int,double>> merged;for(auto e:row){if(!merged.empty()&&merged.back().first==e.first)merged.back().second+=e.second;else merged.push_back(e);}row.swap(merged);}
    ProjectionResult out;out.cg=pcg(diag,off,rhs,p);out.pressure=p;out.faceVelocity.reserve(faces.size());Vec2 bodyDelta{0,0};for(int i=0;i<n;++i)bodyDelta=bodyDelta+ s[size_t(i)]*p[size_t(i)];out.bodyVelocity=bodyVelocityStar+bodyDelta*(dt/bodyMass);out.bodyImpulseX=0;out.bodyImpulseY=0;out.fluidImpulseX=0;out.fluidImpulseY=0;for(const auto&w:patches){double j=dt*p[size_t(w.cell)]*w.area;out.bodyImpulseX+=j*w.fluidOutwardNormal.x;out.bodyImpulseY+=j*w.fluidOutwardNormal.y;out.fluidImpulseX-=j*w.fluidOutwardNormal.x;out.fluidImpulseY-=j*w.fluidOutwardNormal.y;}
    std::vector<double> residual(static_cast<size_t>(n),0);for(size_t k=0;k<faces.size();++k){const auto&f=faces[k];double pr=f.right>=0?p[size_t(f.right)]:0;double vl=f.velocityStar-dt/rho*(pr-p[size_t(f.left)])/f.distance;out.faceVelocity.push_back(vl);double flux=f.area*vl;residual[size_t(f.left)]+=flux;if(f.right>=0)residual[size_t(f.right)]-=flux;}
    for(int i=0;i<n;++i)residual[size_t(i)]+=dot(s[size_t(i)],out.bodyVelocity);
    for(double r:residual)out.maxConstraintResidual=std::max(out.maxConstraintResidual,std::abs(r));
    return out;
}
}
