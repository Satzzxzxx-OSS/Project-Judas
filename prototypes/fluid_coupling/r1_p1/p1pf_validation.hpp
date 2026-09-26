#pragma once
// Independent physical fixtures for the frozen/resolved component. Analytical
// expectations below never enter the assembled pressure or rigid velocity solve.
#include "frozen_projection.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace pfvalidation {
using namespace pf;
constexpr double dt=.01, grav=9.81;
inline void require(bool ok,const std::string& what){if(!ok)throw std::runtime_error(what);}
inline Vec minus(const Vec&a,const Vec&b){require(a.size()==b.size(),"vector sizes");Vec d=a;for(size_t i=0;i<d.size();++i)d[i]-=b[i];return d;}
inline double cross2(V2 a,V2 b){return a[0]*b[1]-a[1]*b[0];}
inline std::string number(double v){std::ostringstream s;s<<v;return s.str();}
inline Config uniform(int nx,int ny,double density=1000){Config c;c.nx=nx;c.ny=ny;c.labels.assign(nx*ny,-1);c.fine_density.assign(4*nx*ny,density);return c;}
inline void rectangle(Config&c,int x0,int x1,int y0,int y1,int id=0){for(int i=x0;i<x1;++i)for(int j=y0;j<y1;++j)c.labels[i*c.ny+j]=id;}
inline void hold(Config&c,int id=0){for(int q=0;q<3;++q)c.supports.emplace_back(id,q);}
inline void openAll(Config&c){for(auto side:{"left","right","bottom","top"})c.boundary[side]="pressure";}
inline std::map<std::string,Vec> randomInputs(){
    std::ifstream f("fixtures/p1pf_random_inputs.csv");require(bool(f),"Missing reproducible seeded input CSV");
    std::map<std::string,Vec> all;std::string line;std::getline(f,line);
    while(std::getline(f,line)){std::istringstream s(line);std::string group,index,value;std::getline(s,group,',');std::getline(s,index,',');std::getline(s,value,',');require(std::stoul(index)==all[group].size(),"random input index");all[group].push_back(std::stod(value));}return all;
}

struct Evidence {
    std::ofstream summary,checks,rows,dofs,bodies,negative,matrix;
    int positives=0,negatives=0,idempotence=0,solve_calls=0;std::string active;
    Evidence(){
        std::filesystem::create_directories("evidence/p1pf/current");
        auto out=[&](std::ofstream&f,const char*n){f.open(std::string("evidence/p1pf/current/")+n);f<<std::setprecision(17);require(bool(f),"open evidence file");};
        out(summary,"projection_summary.csv");out(checks,"projection_oracles.csv");out(rows,"projection_rows.csv");out(dofs,"projection_dofs.csv");out(bodies,"projection_bodies.csv");out(negative,"negative_controls.csv");out(matrix,"projection_matrix.csv");
        summary<<"name,nx,ny,fluid_cells,fluid_dofs,bodies,rows,columns,dt,mass_min,mass_max,rank,nullity,sigma_max,sigma_min_retained,rank_threshold,condition,constraint_raw,constraint_residual,impulse_raw,impulse_residual,energy_raw,energy_identity_residual,compatibility_raw,compatibility_residual,momentum_raw_x,momentum_raw_y,momentum_raw_angular,momentum_budget_residual,mass_partition_error,normal_closure,moment_closure,kinetic_before,kinetic_after,work,dissipation,velocity_min,velocity_max,pressure_min,pressure_max,solve_seconds,status\n";
        checks<<"name,quantity,actual,expected,error,tolerance,status\n";
        rows<<"name,row,kind,first,second,rhs,multiplier,constraint_raw,divergence_or_normal_mismatch\n";
        dofs<<"name,dof,axis,i,j,local_x,local_y,owner,mass,star,projected,momentum_before,momentum_after,impulse_mismatch\n";
        bodies<<"name,body,component,mass_or_inertia,star,projected,pressure_impulse,support_impulse,total_impulse,impulse_mismatch\n";
        negative<<"name,actual,expected,error,detection_threshold,status\n";
        matrix<<"name,row,column,value\n";
    }
    void check(const std::string&key,double actual,double expected,double tolerance){double error=std::abs(actual-expected);bool ok=std::isfinite(error)&&error<tolerance;checks<<active<<','<<key<<','<<actual<<','<<expected<<','<<error<<','<<tolerance<<','<<(ok?"PASS":"FAIL")<<'\n';checks.flush();require(ok,active+" "+key+" failed: "+number(error));}
    void gate(const std::string&key,double value,double tolerance){check(key,value,0.,tolerance);}
    void detect(const std::string&name,double actual,double expected,double threshold){double err=std::abs(actual-expected);bool ok=std::isfinite(err)&&err>threshold;negative<<name<<','<<actual<<','<<expected<<','<<err<<','<<threshold<<','<<(ok?"DETECTED":"FAIL")<<'\n';negative.flush();require(ok,"Negative control undetected: "+name);++negatives;}
    Projection checked(const std::string&name,const FrozenMAC&g,const Vec&w){
        active="\""+name+"\"";auto start=std::chrono::steady_clock::now();++solve_calls;Projection p=g.project(w);double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        Vec ext(g.H.size(),0.);for(auto r:g.wall_rows)for(int k=0;k<g.B.cols;++k)ext[k]+=g.B(r,k)*p.impulses[r];for(auto r:g.support_rows)for(int k=0;k<g.B.cols;++k)ext[k]+=g.B(r,k)*p.impulses[r];
        Vec delta=minus(p.velocity,w),vext=ext;for(size_t k=0;k<vext.size();++k)vext[k]/=g.H[k];Vec budget=minus(mul(g.moment,delta),mul(g.moment,vext));
        double normalClosure=0.,momentClosure=0.;
        for(size_t r=0;r<g.cells.size();++r){auto [i,j]=g.cells[r];V2 sum{0.,0.};double angular=0.;for(const auto&f:g.faces){for(auto a:f.adjacent)if(a[0]==i&&a[1]==j){V2 n=g.rotate(f.axis==0?V2{double(a[2]),0.}:V2{0.,double(a[2])});sum[0]+=f.area*n[0];sum[1]+=f.area*n[1];angular+=f.area*cross2(g.rotate(f.xy),n);}}normalClosure=std::max(normalClosure,std::hypot(sum[0],sum[1]));momentClosure=std::max(momentClosure,std::abs(angular));}
        double mom=g.momentum_budget(p,w),mass=g.mass_partition_error();
        double pmin=1e300,pmax=-1e300;for(int i=0;i<g.nc;++i){pmin=std::min(pmin,p.impulses[i]/dt);pmax=std::max(pmax,p.impulses[i]/dt);}
        summary<<active<<','<<g.nx<<','<<g.ny<<','<<g.nc<<','<<g.nf<<','<<g.bodies.size()<<','<<g.B.rows<<','<<g.B.cols<<','<<dt<<','<<*std::min_element(g.H.begin(),g.H.end())<<','<<*std::max_element(g.H.begin(),g.H.end())<<','<<p.rank<<','<<p.nullity<<','<<p.sigma_max<<','<<p.sigma_min_retained<<','<<p.rank_threshold<<','<<p.whitened_condition<<','<<p.constraint_raw_norm<<','<<p.constraint_residual<<','<<p.impulse_raw_norm<<','<<p.impulse_residual<<','<<p.energy_raw<<','<<p.energy_identity_residual<<','<<p.compatibility_raw_norm<<','<<p.rejected_compatibility<<','<<budget[0]<<','<<budget[1]<<','<<budget[2]<<','<<mom<<','<<mass<<','<<normalClosure<<','<<momentClosure<<','<<p.kinetic_before<<','<<p.kinetic_after<<','<<p.prescribed_boundary_work<<','<<p.dissipation<<','<<*std::min_element(p.velocity.begin(),p.velocity.end())<<','<<*std::max_element(p.velocity.begin(),p.velocity.end())<<','<<pmin<<','<<pmax<<','<<seconds<<",SOLVED\n";summary.flush();
        for(int r=0;r<g.B.rows;++r){auto m=g.meta[r];double physical=p.raw_constraint[r];if(m.kind=="cell")physical/=(g.hx*g.hy);if(m.kind=="surface")physical/=g.faces[m.first].area;rows<<active<<','<<r<<','<<m.kind<<','<<m.first<<','<<m.second<<','<<g.rhs[r]<<','<<p.impulses[r]<<','<<p.raw_constraint[r]<<','<<physical<<'\n';for(int k=0;k<g.B.cols;++k)if(g.B(r,k)!=0)matrix<<active<<','<<r<<','<<k<<','<<g.B(r,k)<<'\n';}
        for(int k=0;k<g.nf;++k){const auto&f=g.faces[k];dofs<<active<<','<<k<<','<<f.axis<<','<<f.i<<','<<f.j<<','<<f.xy[0]<<','<<f.xy[1]<<','<<f.owner<<','<<g.H[k]<<','<<w[k]<<','<<p.velocity[k]<<','<<g.H[k]*w[k]<<','<<p.momenta[k]<<','<<p.raw_impulse[k]<<'\n';}
        for(const auto&[id,b]:g.bodies){(void)b;int o=g.body_offset.at(id);for(int q=0;q<3;++q){double pressure=0,support=0;for(int r:g.body_rows.at(id))pressure+=g.B(r,o+q)*p.impulses[r];for(int r:g.support_rows)support+=g.B(r,o+q)*p.impulses[r];double actual=g.H[o+q]*delta[o+q];bodies<<active<<','<<id<<','<<q<<','<<g.H[o+q]<<','<<w[o+q]<<','<<p.velocity[o+q]<<','<<pressure<<','<<support<<','<<actual<<','<<actual-pressure-support<<'\n';}}
        gate("constraint_residual",p.constraint_residual,1e-9);gate("impulse_residual",p.impulse_residual,1e-9);gate("energy_identity_residual",p.energy_identity_residual,1e-9);gate("momentum_budget_residual",mom,1e-9);gate("mass_partition_error",mass,1e-9);gate("normal_closure",normalClosure,1e-12);gate("moment_closure",momentClosure,1e-12);
        if(max_abs(g.rhs)==0.)require(p.kinetic_after<=p.kinetic_before+1e-9*std::max(1.,p.kinetic_before),name+" homogeneous energy grew");
        ++positives;std::cout<<"P1-PF "<<name<<" algebra PASS rows="<<g.B.rows<<" cols="<<g.B.cols<<" nullity="<<p.nullity<<" time="<<seconds<<"s\n";return p;
    }
    void repeat(const FrozenMAC&g,const Projection&p){++idempotence;++solve_calls;auto again=g.project(p.velocity);gate("idempotence_error",max_abs(minus(again.velocity,p.velocity)),1e-9);}
};

inline void malformedInputChecks(){
    std::ofstream log("evidence/p1pf/current/input_rejections.csv");log<<"name,status,message\n";
    auto rejected=[&](const std::string&name,const std::function<void()>&f){bool caught=false;try{f();}catch(const std::invalid_argument&x){caught=true;log<<name<<",REJECTED,\""<<x.what()<<"\"\n";}require(caught,"Malformed input accepted: "+name);};
    Matrix B(1,1);B(0,0)=1;Vec mass{1},rhs{0},w{0};const double nan=std::numeric_limits<double>::quiet_NaN();
    rejected("zero_mass",[&]{(void)pf::project(B,Vec{0},rhs,w);});
    rejected("negative_mass",[&]{(void)pf::project(B,Vec{-1},rhs,w);});
    rejected("nonfinite_mass",[&]{(void)pf::project(B,Vec{nan},rhs,w);});
    rejected("nonfinite_velocity",[&]{(void)pf::project(B,mass,rhs,Vec{nan});});
    rejected("nonfinite_rhs",[&]{(void)pf::project(B,mass,Vec{nan},w);});
    rejected("nonfinite_operator",[&]{Matrix bad=B;bad(0,0)=nan;(void)pf::project(bad,mass,rhs,w);});
    rejected("structural_zero_row",[&]{(void)pf::project(Matrix(1,1),mass,rhs,w);});
    rejected("dimension_mismatch",[&]{(void)pf::project(B,Vec{1,1},rhs,w);});
    rejected("zero_body_inertia",[&]{Config c=uniform(2,2);c.labels[0]=0;c.bodies[0]={1,0,{.25,.25}};FrozenMAC g(c);});
}
inline int runProjectionValidation(){
    Evidence e;auto version=pf::lapack_version();{std::ofstream info("evidence/p1pf/current/lapack_version.txt");info<<version[0]<<'.'<<version[1]<<'.'<<version[2]<<'\n';}malformedInputChecks();auto input=randomInputs();auto start=std::chrono::steady_clock::now();
    for(double ratio:{1.,1e3,1e6}){
        Config c=uniform(8,8);require(input.at("random_fluid_rho").size()==c.fine_density.size(),"seeded rho size");for(size_t k=0;k<c.fine_density.size();++k)c.fine_density[k]=1+(ratio-1)*input.at("random_fluid_rho")[k];
        for(bool opened:{false,true}){if(opened)c.boundary["top"]="pressure";FrozenMAC g(c);const Vec&w=input.at(opened?"random_fluid_open_w":"random_fluid_closed_w");auto p=e.checked("random_fluid_"+number(ratio)+"_open"+(opened?"True":"False"),g,w);e.check("gauge_count",p.nullity,opened?0:1,.5);e.repeat(g,p);}
    }
    for(double ratio:{1.,1e3,1e6}){Config c=uniform(12,12);c.fine_density=layered_density(12,12,ratio,1,.5);c.boundary["top"]="pressure";FrozenMAC g(c);auto w=g.provisional(dt,{0.,-grav});auto p=e.checked("sharp_density_jump_"+(ratio==1?std::string("1.0"):ratio==1e3?std::string("1000.0"):std::string("1000000.0")),g,w);double err=0,scale=1;for(int k=0;k<g.nc;++k){double y=(g.cells[k].second+.5)/12;double exact=grav*(1-y+(ratio-1)*std::max(.5-y,0.));err=std::max(err,std::abs(p.impulses[k]/dt-exact));scale=std::max(scale,std::abs(exact));}e.gate("hydro_pressure_relative_error",err/scale,1e-9);e.gate("equilibrium_speed",max_abs(p.velocity),1e-8);}
    Config submerged=uniform(12,12);rectangle(submerged,5,7,3,5);submerged.fine_density=layered_density(12,12,1000,1,.5);submerged.boundary["top"]="pressure";
    for(double ratio:{.001,.5,1.,2.,1000.}){
        Config c=submerged;c.bodies[0]=geometry_body(c.nx,c.ny,c.labels,0,1000*ratio);FrozenMAC g(c);auto p=e.checked("submerged_free_density_ratio_"+number(ratio),g,g.provisional(dt,{0,-grav}));int o=g.body_offset.at(0);
        if(ratio==1.)e.gate("neutral_body_speed",max_abs(Vec(p.velocity.begin()+o,p.velocity.begin()+o+3)),1e-9);else{double direction=p.velocity[o+1]*(1-ratio);e.check("lighter_up_heavier_down",direction>0?1.:0.,1.,.5);}
        hold(c);FrozenMAC h(c);auto hp=e.checked("submerged_supported_density_ratio_"+number(ratio),h,h.provisional(dt,{0,-grav}));e.gate("supported_speed",max_abs(hp.velocity),1e-9);auto f=h.surface_force(hp,0,dt);double expected=1000*(2./12)*(2./12)*grav;for(int q=0;q<3;++q)e.check("hydro_force_"+std::to_string(q),f[q],q==1?expected:0.,1e-7);double err=0;for(int k=0;k<h.nc;++k){double y=(h.cells[k].second+.5)/12;double exact=grav*(1-y+999*std::max(.5-y,0.));err=std::max(err,std::abs(hp.impulses[k]/dt-exact));}e.gate("hydro_pressure_error",err,1e-7);
    }
    {
        Config c=uniform(12,12);rectangle(c,4,8,4,5);rectangle(c,4,5,5,8);c.bodies[0]=geometry_body(12,12,c.labels,0,800);c.fine_density=layered_density(12,12,1000,1,.5);c.boundary["top"]="pressure";hold(c);FrozenMAC g(c);auto p=e.checked("asymmetric_partial_immersion_hydrostatic_torque",g,g.provisional(dt,{0,-grav}));auto f=g.surface_force(p,0,dt);
        double fy=0,torque=0;for(auto r:std::vector<std::array<double,5>>{{4./12,8./12,4./12,5./12,1000},{4./12,5./12,5./12,6./12,1000},{4./12,5./12,6./12,8./12,1}}){double force=r[4]*(r[1]-r[0])*(r[3]-r[2])*grav;fy+=force;torque+=((r[0]+r[1])/2-c.bodies[0].centre[0])*force;}e.check("hydro_force_x",f[0],0,1e-7);e.check("hydro_force_y",f[1],fy,1e-7);e.check("hydro_torque",f[2],torque,1e-7);
    }
    {
        Config c=uniform(12,12,1);rectangle(c,3,9,2,3);rectangle(c,3,4,3,9);rectangle(c,8,9,3,9);for(int i=8;i<16;++i)for(int j=6;j<12;++j)c.fine_density[i*24+j]=1000;c.bodies[0]=geometry_body(12,12,c.labels,0,500);c.boundary["top"]="pressure";hold(c);FrozenMAC g(c);auto p=e.checked("supported_resolved_cup_load",g,g.provisional(dt,{0,-grav}));auto force=g.surface_force(p,0,dt);double expected=grav*(18./144-999*(4./12)*(3./12));double support=0;for(int r:g.support_rows)if(g.meta[r].second==1)support+=p.impulses[r]/dt;e.check("fluid_force_y",force[1],expected,1e-7);e.check("support_force_y",support,c.bodies[0].mass*grav-expected,1e-7);e.gate("equilibrium_speed",max_abs(p.velocity),1e-9);
    }
    Config piston=uniform(12,6,1);rectangle(piston,5,7,0,6);for(int i=0;i<10;++i)for(int j=0;j<12;++j)piston.fine_density[i*12+j]=1000;auto shape=geometry_body(12,6,piston.labels,0,1);
    for(double mass:{.01,.1,1.,80.,1000.,1e6}){Config c=piston;c.bodies[0]={mass,shape.inertia*mass/shape.mass,shape.centre};c.boundary["left"]=c.boundary["right"]="pressure";c.supports={{0,1},{0,2}};FrozenMAC g(c);auto w=g.provisional(dt,{},{{0,{100,0,0}}});auto p=e.checked("open_piston_mass_"+number(mass),g,w);double expected=100*dt/(mass+1000*(5./12)+5./12);double actual=p.velocity[g.body_offset.at(0)];e.gate("relative_velocity_error",std::abs(actual-expected)/expected,1e-8);e.check("piston_velocity",actual,expected,1e-9);double ferr=0;for(int k=0;k<g.nf;++k)if(g.faces[k].axis==0)ferr=std::max(ferr,std::abs(p.velocity[k]-expected));e.gate("fluid_velocity_error",ferr,1e-9);
        if(mass==80){Vec badH=g.H;for(int r:g.body_rows.at(0))badH[g.meta[r].first]*=.5;++e.solve_calls;auto bad=g.project(w,&badH);double wrong=bad.velocity[g.body_offset.at(0)];e.detect("halve_interface_half_dual_masses",(wrong-expected)/expected,0,1e-3);}
        if(mass==.01){double qstar=100*dt/mass;double wrong=qstar-(1000*(5./12)+5./12)*qstar/mass;e.detect("lagged_partitioned_piston",wrong,expected,1);}
    }
    {
        Config c=piston;c.fine_density.assign(24*12,1000);c.bodies[0]=geometry_body(12,6,c.labels,0,480);c.supports={{0,1},{0,2}};FrozenMAC g(c);auto w=g.provisional(dt,{},{{0,{100,0,0}}});auto p=e.checked("sealed_piston_pressure_difference",g,w);e.check("common_gauge_count",p.nullity,1,.5);double left=0,right=0;int nl=0,nr=0;for(int k=0;k<g.nc;++k){if(g.cells[k].first<5){left+=p.impulses[k]/dt;++nl;}else if(g.cells[k].first>=7){right+=p.impulses[k]/dt;++nr;}}e.check("pressure_difference",right/nr-left/nl,100,1e-8);e.gate("sealed_velocity",max_abs(p.velocity),1e-9);
        // Exact reference negative control: pin two pressure multipliers by
        // deleting the corresponding rows/columns from the Schur matrix.
        Matrix A(g.B.rows,g.B.rows);for(int i=0;i<g.B.rows;++i)for(int j=0;j<g.B.rows;++j)for(int k=0;k<g.B.cols;++k)A(i,j)+=g.B(i,k)*g.B(j,k)/g.H[k];
        Vec rhs=minus(g.rhs,mul(g.B,w));int pin0=g.cell_index.at({0,0}),pin1=g.cell_index.at({11,0});std::vector<int> keep;for(int i=0;i<g.B.rows;++i)if(i!=pin0&&i!=pin1)keep.push_back(i);Matrix reduced(int(keep.size()),int(keep.size()));Vec reduced_rhs;for(size_t i=0;i<keep.size();++i){reduced_rhs.push_back(rhs[keep[i]]);for(size_t j=0;j<keep.size();++j)reduced(int(i),int(j))=A(keep[i],keep[j]);}++e.solve_calls;Vec lm(g.B.rows,0),solved=pf::least_squares(reduced,reduced_rhs);for(size_t i=0;i<keep.size();++i)lm[keep[i]]=solved[i];Vec wrongw=w,imp=transpose_mul(g.B,lm);for(size_t k=0;k<w.size();++k)wrongw[k]+=imp[k]/g.H[k];double leak=max_abs(minus(mul(g.B,wrongw),g.rhs));e.detect("pin_each_disconnected_chamber",leak,0,1e-6);
    }
    Config rect=uniform(12,12);rectangle(rect,5,7,3,5);rect.bodies[0]=geometry_body(12,12,rect.labels,0,700);
    {
        Config c=rect;openAll(c);c.external_pressure=[](V2){return 1234.;};FrozenMAC g(c);auto p=e.checked("constant_ambient_pressure",g,g.provisional(dt));auto f=g.surface_force(p,0,dt);e.gate("ambient_velocity",max_abs(p.velocity),1e-9);for(int q=0;q<3;++q)e.check("ambient_force_"+std::to_string(q),f[q],0,1e-7);double error=0;for(int k=0;k<g.nc;++k)error=std::max(error,std::abs(p.impulses[k]/dt-1234));e.gate("constant_pressure_error",error,1e-7);
    }
    {
        Config c=rect;c.fine_density=layered_density(12,12,1000,1,.5);V2 U{.8,-.3},gvec{1.7,-grav};c.wall_velocity=[=](V2){return V2{U[0]+dt*gvec[0],U[1]+dt*gvec[1]};};FrozenMAC g(c);auto w=g.provisional(dt,gvec,{},U);auto p=e.checked("common_free_fall",g,w);e.gate("free_fall_correction",max_abs(minus(p.velocity,w)),1e-10);
    }
    {
        Config c=rect;require(input.at("covariance_rho_random").size()==c.fine_density.size(),"covariance rho count");for(size_t i=0;i<c.fine_density.size();++i)c.fine_density[i]=1+999*input.at("covariance_rho_random")[i];FrozenMAC base(c);const Vec&w=input.at("covariance_w");auto p=e.checked("covariance_reference",base,w);
        for(V2 origin:std::vector<V2>{{0,0},{1e9,-2e9}}){Config other=c;other.rotation=.731;other.origin=origin;FrozenMAC g(other);Vec rotated=w;int o=g.body_offset.at(0);V2 v=g.rotate({w[o],w[o+1]});rotated[o]=v[0];rotated[o+1]=v[1];auto pp=e.checked("covariance_angle0.731_origin"+number(origin[0]),g,rotated);Vec expected=p.velocity;v=g.rotate({expected[o],expected[o+1]});expected[o]=v[0];expected[o+1]=v[1];e.gate("covariance_velocity_error",max_abs(minus(pp.velocity,expected)),1e-9);}
        V2 shift{2.3,-1.2};c.wall_velocity=[=](V2){return shift;};FrozenMAC g(c);Vec add=g.provisional(0,{}, {},shift),shifted=w;for(size_t k=0;k<w.size();++k)shifted[k]+=add[k];auto pp=e.checked("galilean_covariance",g,shifted);Vec err=minus(minus(pp.velocity,p.velocity),add);e.gate("covariance_velocity_error",max_abs(err),1e-9);
    }
    {
        Config c=uniform(8,6);c.boundary["left"]=c.boundary["right"]="pressure";FrozenMAC g(c);auto w=g.provisional(0,{}, {},{1.7,0});auto p=e.checked("normal_only_free_slip",g,w);e.gate("tangential_velocity_error",max_abs(minus(p.velocity,w)),1e-10);
    }
    {
        FrozenMAC g(uniform(6,6));Vec bad=g.rhs;for(int r:g.wall_rows)if(g.faces[g.meta[r].first].side=="left"){bad[r]=.1;break;}bool caught=false;try{++e.solve_calls;(void)g.project(Vec(g.H.size(),0),nullptr,&bad);}catch(const IncompatibleConstraints&x){caught=true;std::ofstream detail("evidence/p1pf/current/incompatible_input.txt");detail<<"Sealed 6x6 grid; first left-wall row RHS changed from 0 to 0.1.\n"<<x.what()<<'\n';}e.detect("incompatible_closed_wall_flux",caught?1.:0.,0,.5);
    }
    {
        Config c=uniform(10,10);rectangle(c,2,4,3,5,0);rectangle(c,6,8,6,8,1);c.bodies[0]=geometry_body(10,10,c.labels,0,1);c.bodies[1]=geometry_body(10,10,c.labels,1,1e4);FrozenMAC g(c);const auto&w=input.at("two_bodies_w");auto p=e.checked("two_bodies_mixed_mass",g,w);Vec correct=transpose_mul(g.B,p.impulses);for(size_t k=0;k<correct.size();++k)correct[k]/=g.H[k];Vec wrong=correct;for(auto[id,b]:c.bodies){(void)b;wrong[g.body_offset.at(id)+2]=0;}double leak=std::abs(mul(g.moment,minus(wrong,correct))[2]);e.detect("drop_rotational_reaction",leak,0,1e-5);
    }
    {
        Config c=rect;c.bodies[0]=geometry_body(12,12,c.labels,0,1000);openAll(c);for(V2 gravity:std::vector<V2>{{3.1,-9.81},{-7,2},{9.81,0}}){c.external_pressure=[=](V2 x){return 20000+1000*(gravity[0]*x[0]+gravity[1]*x[1]);};FrozenMAC g(c);auto p=e.checked(std::string("arbitrary_gravity_")+(gravity[0]==3.1?"[3.1, -9.81]":gravity[0]==-7?"[-7.0, 2.0]":"[9.81, 0.0]"),g,g.provisional(dt,gravity));auto force=g.surface_force(p,0,dt);e.gate("hydrostatic_speed",max_abs(p.velocity),1e-9);for(int q=0;q<2;++q)e.check("pressure_force_"+std::to_string(q),force[q],-c.bodies[0].mass*gravity[q],1e-7);e.check("pressure_torque",force[2],0,1e-7);}
    }
    Vec errors;
    for(int n:{6,12,24}){Config c=uniform(n,n);openAll(c);FrozenMAC g(c);Vec w(g.H.size(),0);const double pi=std::acos(-1.);for(int k=0;k<g.nf;++k){auto f=g.faces[k];double x=f.xy[0],y=f.xy[1];double derivative=f.axis==0?500*pi*std::cos(pi*x)*std::sin(pi*y):500*pi*std::sin(pi*x)*std::cos(pi*y);w[k]=dt*derivative/1000;}auto p=e.checked("smooth_pressure_refinement_"+std::to_string(n),g,w);double error=0;for(int k=0;k<g.nc;++k){auto[i,j]=g.cells[k];double expected=500*std::sin(pi*(i+.5)/n)*std::sin(pi*(j+.5)/n);double actual=p.impulses[k]/dt;error+=std::abs(actual-expected);e.checks<<e.active<<",pressure_cell_"<<i<<'_'<<j<<','<<actual<<','<<expected<<','<<std::abs(actual-expected)<<",-1,MEASURED\n";}error/=g.nc;errors.push_back(error);e.checks<<e.active<<",pressure_L1,"<<error<<",0,"<<error<<",-1,MEASURED\n";e.gate("projected_speed",max_abs(p.velocity),1e-9);}
    require(errors[2]<errors[1]&&errors[1]<errors[0],"pressure refinement error not decreasing");for(int k=0;k<2;++k){double ratio=errors[k]/errors[k+1];e.checks<<"pressure_refinement,ratio_"<<k<<','<<ratio<<",4,"<<std::abs(ratio-4)<<",-1,"<<(ratio>3.7&&ratio<4.4?"PASS":"FAIL")<<'\n';require(ratio>3.7&&ratio<4.4,"refinement ratio outside reference gate");}
    require(e.positives==42&&e.negatives==5,"reference case counts");std::ofstream count("evidence/p1pf/current/projection_counts.csv");count<<"positives,negative_controls,idempotence_calls,total_solver_calls,runtime_s,status\n"<<e.positives<<','<<e.negatives<<','<<e.idempotence<<','<<e.solve_calls<<','<<std::setprecision(17)<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<",PASS\n";std::cout<<"P1-PF projection PASS: 42 positive cases, 5 negative controls; idempotence calls="<<e.idempotence<<"\n";return 0;
}
} // namespace pfvalidation
