#pragma once
// P1-PF only: resolved, frozen-geometry MAC/body mass-weighted projection.
// Equations and ordering follow evidence/p1pf/handoff/reference/projection_checks.py.
// LAPACK DGESVD supplies the rank-revealing linear algebra. There is no timestep,
// moving occupancy, cut-cell approximation, or analytic fluid support in this file.
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

extern "C" {
void dgesvd_(const char*,const char*,const int*,const int*,double*,const int*,
             double*,double*,const int*,double*,const int*,double*,const int*,int*);
void ilaver_(int*,int*,int*);
}

namespace pf {
using Vec=std::vector<double>;
using V2=std::array<double,2>;
using V3=std::array<double,3>;
struct Matrix {
    int rows=0,cols=0;
    Vec data;
    Matrix()=default;
    Matrix(int r,int c,double value=0):rows(r),cols(c) {
        if(r<0||c<0) throw std::invalid_argument("Negative matrix dimension");
        data.assign(static_cast<size_t>(r)*static_cast<size_t>(c),value);
    }
    double& operator()(int i,int j){return data.at(static_cast<size_t>(i)*cols+j);}
    double operator()(int i,int j)const{return data.at(static_cast<size_t>(i)*cols+j);}
};
inline bool finite(const Vec&x){return std::all_of(x.begin(),x.end(),[](double a){return std::isfinite(a);});}
inline void require(bool valid,const std::string&message){if(!valid)throw std::invalid_argument(message);}
inline void validate(const Matrix&A){require(A.rows>0&&A.cols>0&&A.data.size()==static_cast<size_t>(A.rows)*A.cols&&finite(A.data),"Malformed/nonfinite or empty matrix");}
inline double norm(const Vec&x){double s=0;for(double a:x)s=std::hypot(s,a);return s;}
inline double max_abs(const Vec&x){double s=0;for(double a:x)s=std::max(s,std::abs(a));return s;}
inline double dot(const Vec&a,const Vec&b){require(a.size()==b.size(),"Dot-product shape mismatch");double s=0;for(size_t k=0;k<a.size();++k)s+=a[k]*b[k];return s;}
inline double dot(V2 a,V2 b){return a[0]*b[0]+a[1]*b[1];}
inline double cross(V2 a,V2 b){return a[0]*b[1]-a[1]*b[0];}
inline Vec mul(const Matrix&A,const Vec&x){require(x.size()==static_cast<size_t>(A.cols),"Matrix-vector shape mismatch");Vec y(A.rows);for(int i=0;i<A.rows;++i)for(int j=0;j<A.cols;++j)y[i]+=A(i,j)*x[j];return y;}
inline Vec transpose_mul(const Matrix&A,const Vec&x){require(x.size()==static_cast<size_t>(A.rows),"Transpose-vector shape mismatch");Vec y(A.cols);for(int i=0;i<A.rows;++i)for(int j=0;j<A.cols;++j)y[j]+=A(i,j)*x[i];return y;}
inline std::array<int,3> lapack_version(){std::array<int,3> v{};ilaver_(&v[0],&v[1],&v[2]);return v;}

struct ThinSVD { Matrix U,VT; Vec singular; };
inline ThinSVD svd(const Matrix&A){
    validate(A);
    const int m=A.rows,n=A.cols,k=std::min(m,n),lda=m,ldu=m,ldvt=k;
    Vec column_major(static_cast<size_t>(m)*n);
    for(int j=0;j<n;++j)for(int i=0;i<m;++i)column_major[static_cast<size_t>(j)*m+i]=A(i,j);
    Vec singular(k),u(static_cast<size_t>(m)*k),vt(static_cast<size_t>(k)*n);
    const char job='S';int info=0,lwork=-1;double query=0;
    dgesvd_(&job,&job,&m,&n,column_major.data(),&lda,singular.data(),u.data(),&ldu,vt.data(),&ldvt,&query,&lwork,&info);
    if(info!=0||!std::isfinite(query)||query<1||query>std::numeric_limits<int>::max())throw std::runtime_error("LAPACK DGESVD workspace query failed: info="+std::to_string(info));
    lwork=static_cast<int>(std::ceil(query));Vec work(lwork);
    // The query is not assumed to preserve the input array.
    for(int j=0;j<n;++j)for(int i=0;i<m;++i)column_major[static_cast<size_t>(j)*m+i]=A(i,j);
    dgesvd_(&job,&job,&m,&n,column_major.data(),&lda,singular.data(),u.data(),&ldu,vt.data(),&ldvt,work.data(),&lwork,&info);
    if(info!=0)throw std::runtime_error("LAPACK DGESVD failed: info="+std::to_string(info));
    if(!finite(singular)||!finite(u)||!finite(vt))throw std::runtime_error("LAPACK DGESVD returned nonfinite factors");
    ThinSVD out{Matrix(m,k),Matrix(k,n),singular};
    for(int i=0;i<m;++i)for(int j=0;j<k;++j)out.U(i,j)=u[static_cast<size_t>(j)*m+i];
    for(int i=0;i<k;++i)for(int j=0;j<n;++j)out.VT(i,j)=vt[static_cast<size_t>(j)*k+i];
    return out;
}
// General least-squares helper, used by the intentionally wrong pinned-pressure
// control. It does not certify constraint compatibility or implement projection.
// Default cutoff matches scipy.linalg.lstsq(cond=None): machine epsilon * sigma_max.
inline Vec least_squares(const Matrix&A,const Vec&b,double relative_cutoff=std::numeric_limits<double>::epsilon()){
    require(b.size()==static_cast<size_t>(A.rows)&&finite(b)&&std::isfinite(relative_cutoff)&&relative_cutoff>=0,"Malformed least-squares input");
    auto f=svd(A);Vec x(A.cols);double threshold=relative_cutoff*f.singular.front();
    for(int k=0;k<static_cast<int>(f.singular.size());++k)if(f.singular[k]>threshold){double z=0;for(int i=0;i<A.rows;++i)z+=f.U(i,k)*b[i];z/=f.singular[k];for(int j=0;j<A.cols;++j)x[j]+=f.VT(k,j)*z;}
    if(!finite(x))throw std::runtime_error("Nonfinite least-squares solution");
    return x;
}
struct IncompatibleConstraints:std::runtime_error{using std::runtime_error::runtime_error;};
struct Projection {
    Vec velocity,momenta,impulses,raw_constraint,raw_impulse,singular_values;
    int rank=0,nullity=0;
    double whitened_condition=0,constraint_residual=0,impulse_residual=0,energy_identity_residual=0;
    double kinetic_before=0,kinetic_after=0,prescribed_boundary_work=0,rejected_compatibility=0;
    double rank_threshold=0,sigma_max=0,sigma_min_retained=0;
    double constraint_raw_norm=0,impulse_raw_norm=0,energy_raw=0,dissipation=0,compatibility_raw_norm=0;
};
inline Projection project(const Matrix&B,const Vec&H,const Vec&rhs,const Vec&wstar){
    validate(B);const int m=B.rows,n=B.cols;
    require(H.size()==static_cast<size_t>(n)&&wstar.size()==H.size()&&rhs.size()==static_cast<size_t>(m),"Projection input dimensions disagree");
    require(finite(H)&&finite(rhs)&&finite(wstar),"Nonfinite projection input");
    Vec invroot(n);for(int j=0;j<n;++j){require(H[j]>0,"Positive masses required; no zero/negative mass inversion or mass floor");invroot[j]=1/std::sqrt(H[j]);}
    Matrix C(m,n);Vec scale(m),d(m),bwstar=mul(B,wstar);
    for(int i=0;i<m;++i){double rownorm=0;for(int j=0;j<n;++j){C(i,j)=B(i,j)*invroot[j];rownorm=std::hypot(rownorm,C(i,j));}
        require(std::isfinite(rownorm)&&rownorm>0,"Zero/nonfinite constraint row requires explicit input classification");
        scale[i]=1/rownorm;require(std::isfinite(scale[i]),"Constraint equilibration overflows");
        for(int j=0;j<n;++j)C(i,j)*=scale[i];
        d[i]=scale[i]*(rhs[i]-bwstar[i]);
    }
    require(finite(d),"Nonfinite equilibrated projection right-hand side");
    auto f=svd(C);Projection p;p.singular_values=f.singular;p.sigma_max=f.singular.front();
    p.rank_threshold=64*std::numeric_limits<double>::epsilon()*std::max(m,n)*p.sigma_max;
    for(double sigma:f.singular)if(sigma>p.rank_threshold)++p.rank;
    if(p.rank==0)throw std::runtime_error("No retained singular value in nonzero-row projection");
    p.nullity=m-p.rank;p.sigma_min_retained=f.singular[p.rank-1];p.whitened_condition=p.sigma_max/p.sigma_min_retained;
    Vec dy(n),lambda_unscaled(m);
    for(int k=0;k<p.rank;++k){double coeff=0;for(int i=0;i<m;++i)coeff+=f.U(i,k)*d[i];
        double a=coeff/f.singular[k],b=a/f.singular[k];
        for(int j=0;j<n;++j)dy[j]+=f.VT(k,j)*a;
        for(int i=0;i<m;++i)lambda_unscaled[i]+=f.U(i,k)*b;
    }
    Vec incompat=mul(C,dy);for(int i=0;i<m;++i)incompat[i]=d[i]-incompat[i];
    p.compatibility_raw_norm=norm(incompat);p.rejected_compatibility=p.compatibility_raw_norm/std::max(1.,norm(d));
    if(!std::isfinite(p.rejected_compatibility)||p.rejected_compatibility>1e-10){std::ostringstream s;s<<std::setprecision(17)<<"Incompatible prescribed volume/constraint flux: "<<p.rejected_compatibility;throw IncompatibleConstraints(s.str());}
    p.velocity.resize(n);p.momenta.resize(n);Vec delta(n),hdelta(n);p.impulses.resize(m);
    for(int i=0;i<m;++i)p.impulses[i]=scale[i]*lambda_unscaled[i];
    for(int j=0;j<n;++j){p.velocity[j]=wstar[j]+invroot[j]*dy[j];p.momenta[j]=H[j]*p.velocity[j];delta[j]=p.velocity[j]-wstar[j];hdelta[j]=H[j]*delta[j];
        p.kinetic_before+=.5*H[j]*wstar[j]*wstar[j];p.kinetic_after+=.5*H[j]*p.velocity[j]*p.velocity[j];p.dissipation+=.5*hdelta[j]*delta[j];}
    p.raw_constraint=mul(B,p.velocity);for(int i=0;i<m;++i)p.raw_constraint[i]-=rhs[i];
    p.constraint_raw_norm=norm(p.raw_constraint);p.constraint_residual=p.constraint_raw_norm/std::max({1.,norm(bwstar),norm(rhs)});
    Vec impulse=transpose_mul(B,p.impulses);p.raw_impulse.resize(n);for(int j=0;j<n;++j)p.raw_impulse[j]=hdelta[j]-impulse[j];
    p.impulse_raw_norm=norm(p.raw_impulse);p.impulse_residual=p.impulse_raw_norm/std::max(1.,norm(hdelta));
    p.prescribed_boundary_work=dot(p.impulses,rhs);
    p.energy_raw=p.kinetic_after-p.kinetic_before-p.prescribed_boundary_work+p.dissipation;
    p.energy_identity_residual=std::abs(p.energy_raw)/std::max({1.,p.kinetic_before,p.kinetic_after,std::abs(p.prescribed_boundary_work),p.dissipation});
    if(!finite(p.velocity)||!finite(p.momenta)||!finite(p.impulses)||!finite(p.raw_constraint)||!finite(p.raw_impulse)||!std::isfinite(p.energy_identity_residual))throw std::runtime_error("Nonfinite projection output");
    return p;
}

struct Body{double mass=0,inertia=0;V2 centre{0,0};};
struct Face{int axis=0,i=0,j=0;V2 xy{0,0};double area=0;std::vector<std::array<int,3>>adjacent;double mass=0;int owner=-2;std::string side;};
struct Meta{std::string kind;int first=-1,second=-1;};
struct Config{
    int nx=0,ny=0;std::vector<int>labels;Vec fine_density;std::map<int,Body>bodies;
    std::map<std::string,std::string>boundary;std::vector<std::pair<int,int>>supports;
    double rotation=0;V2 origin{0,0};std::function<V2(V2)>wall_velocity;std::function<double(V2)>external_pressure;
};
struct FrozenMAC{
    int nx=0,ny=0,nf=0,nc=0;double hx=0,hy=0;
    std::vector<int>labels;Vec rho,H,rhs;std::map<int,Body>bodies;
    std::map<std::string,std::string>boundary;std::vector<std::pair<int,int>>supports,cells;
    std::map<std::pair<int,int>,int>cell_index;std::map<std::tuple<int,int,int>,int>face_index;
    std::vector<Face>faces;std::map<int,int>body_offset;Matrix B,Q,moment;
    std::vector<int>wall_rows,support_rows;std::map<int,std::vector<int>>body_rows;std::vector<Meta>meta;
    V2 origin{0,0};std::function<V2(V2)>wall_velocity;std::function<double(V2)>external_pressure;
    explicit FrozenMAC(const Config&c):nx(c.nx),ny(c.ny),labels(c.labels),rho(c.fine_density),bodies(c.bodies),supports(c.supports),Q(2,2),origin(c.origin),wall_velocity(c.wall_velocity),external_pressure(c.external_pressure){
        require(nx>0&&ny>0&&labels.size()==static_cast<size_t>(nx)*ny&&rho.size()==static_cast<size_t>(2*nx)*(2*ny),"Malformed resolved-grid dimensions");
        require(finite(rho)&&std::all_of(rho.begin(),rho.end(),[](double x){return x>0;}),"All phase/fluid densities must be positive and finite");
        require(std::isfinite(c.rotation)&&std::isfinite(origin[0])&&std::isfinite(origin[1]),"Nonfinite rigid grid transform");
        for(const auto&entry:bodies){const Body&b=entry.second;require(entry.first>=0&&std::isfinite(b.mass)&&std::isfinite(b.inertia)&&b.mass>0&&b.inertia>0&&std::isfinite(b.centre[0])&&std::isfinite(b.centre[1]),"Positive finite body mass/inertia and finite centre required");body_rows[entry.first]={};}
        for(int label:labels)if(label>=0)require(bodies.count(label)!=0,"Undeclared solid label");
        hx=1./nx;hy=1./ny;Q(0,0)=Q(1,1)=std::cos(c.rotation);Q(1,0)=std::sin(c.rotation);Q(0,1)=-Q(1,0);
        boundary={{"left","wall"},{"right","wall"},{"bottom","wall"},{"top","wall"}};
        for(const auto&e:c.boundary){require(boundary.count(e.first)&& (e.second=="wall"||e.second=="pressure"),"Unknown boundary side or type");boundary[e.first]=e.second;}
        if(!wall_velocity)wall_velocity=[](V2){return V2{0,0};};
        if(!external_pressure)external_pressure=[](V2){return 0.;};
        for(int i=0;i<nx;++i)for(int j=0;j<ny;++j)if(labels[i*ny+j]<0){cell_index[{i,j}]=static_cast<int>(cells.size());cells.emplace_back(i,j);}
        require(!cells.empty(),"Resolved grid contains no fluid cells");
        for(int ax=0;ax<2;++ax)for(int i=0;i<nx+(ax==0);++i)for(int j=0;j<ny+(ax==1);++j){
            std::array<std::array<int,3>,2>pair=ax==0?std::array<std::array<int,3>,2>{{{i-1,j,1},{i,j,-1}}}:std::array<std::array<int,3>,2>{{{i,j-1,1},{i,j,-1}}};
            Face f;f.axis=ax;f.i=i;f.j=j;
            for(auto a:pair)if(cell_index.count({a[0],a[1]}))f.adjacent.push_back(a);
            if(f.adjacent.empty())continue;
            if(f.adjacent.size()==1){std::array<int,3>a=pair[0]==f.adjacent[0]?pair[1]:pair[0];
                if(a[0]>=0&&a[0]<nx&&a[1]>=0&&a[1]<ny)f.owner=labels[a[0]*ny+a[1]];
                else {f.owner=-1;f.side=a[0]<0?"left":a[0]>=nx?"right":a[1]<0?"bottom":"top";}}
            std::array<int,2>fi=ax==0?std::array<int,2>{2*i-1,2*i}:std::array<int,2>{2*i,2*i+1};
            std::array<int,2>fj=ax==0?std::array<int,2>{2*j,2*j+1}:std::array<int,2>{2*j-1,2*j};
            f.xy=ax==0?V2{i*hx,(j+.5)*hy}:V2{(i+.5)*hx,j*hy};f.area=ax==0?hy:hx;
            for(int a:fi)for(int b:fj)if(a>=0&&a<2*nx&&b>=0&&b<2*ny&&labels[(a/2)*ny+b/2]<0)f.mass+=rho[a*(2*ny)+b]*hx*hy/4;
            require(std::isfinite(f.mass)&&f.mass>0,"Nonpositive/nonfinite fluid dual mass");
            face_index[{ax,i,j}]=static_cast<int>(faces.size());faces.push_back(f);
        }
        nf=static_cast<int>(faces.size());for(const auto&e:bodies)body_offset[e.first]=nf+3*static_cast<int>(body_offset.size());
        const int n=nf+3*static_cast<int>(bodies.size());for(const Face&f:faces)H.push_back(f.mass);
        for(const auto&e:bodies){H.push_back(e.second.mass);H.push_back(e.second.mass);H.push_back(e.second.inertia);}
        std::vector<Vec>rows;
        for(auto cell:cells){int i=cell.first,j=cell.second;Vec row(n);
            for(auto f:std::array<std::array<int,4>,4>{{{0,i,j,-1},{0,i+1,j,1},{1,i,j,-1},{1,i,j+1,1}}}){int k=face_index.at({f[0],f[1],f[2]});row[k]=f[3]*faces[k].area;}
            rows.push_back(row);rhs.push_back(0);meta.push_back({"cell",i,j});}
        nc=static_cast<int>(rows.size());
        for(int k=0;k<nf;++k){const Face&f=faces[k];if(f.owner==-2||(f.owner==-1&&boundary.at(f.side)=="pressure"))continue;
            int sg=f.adjacent[0][2];V2 normal{sg*Q(0,f.axis),sg*Q(1,f.axis)};Vec row(n);row[k]=-sg*f.area;double val=0;
            if(f.owner>=0){int off=body_offset.at(f.owner);const auto&b=bodies.at(f.owner);V2 lever=rotate({f.xy[0]-b.centre[0],f.xy[1]-b.centre[1]});
                row[off]=f.area*normal[0];row[off+1]=f.area*normal[1];row[off+2]=f.area*cross(lever,normal);body_rows[f.owner].push_back(static_cast<int>(rows.size()));}
            else {V2 world=rotate(f.xy);world[0]+=origin[0];world[1]+=origin[1];V2 vel=wall_velocity(world);require(std::isfinite(vel[0])&&std::isfinite(vel[1]),"Nonfinite prescribed wall velocity");val=-f.area*dot(normal,vel);wall_rows.push_back(static_cast<int>(rows.size()));}
            rows.push_back(row);rhs.push_back(val);meta.push_back({"surface",k,-1});
        }
        for(auto s:supports){require(body_offset.count(s.first)&&s.second>=0&&s.second<3,"Invalid support body/component");Vec row(n);row[body_offset.at(s.first)+s.second]=1;
            support_rows.push_back(static_cast<int>(rows.size()));rows.push_back(row);rhs.push_back(0);meta.push_back({"support",s.first,s.second});}
        B=Matrix(static_cast<int>(rows.size()),n);for(int i=0;i<B.rows;++i)for(int j=0;j<n;++j)B(i,j)=rows[i][j];
        moment=Matrix(3,n);for(int k=0;k<nf;++k){const auto&f=faces[k];V2 e{Q(0,f.axis),Q(1,f.axis)},r=rotate(f.xy);moment(0,k)=f.mass*e[0];moment(1,k)=f.mass*e[1];moment(2,k)=f.mass*cross(r,e);}
        for(const auto&e:bodies){int off=body_offset.at(e.first);const Body&b=e.second;V2 r=rotate(b.centre);moment(0,off)=moment(1,off+1)=b.mass;moment(2,off)=-b.mass*r[1];moment(2,off+1)=b.mass*r[0];moment(2,off+2)=b.inertia;}
    }
    V2 rotate(V2 v)const{return {Q(0,0)*v[0]+Q(0,1)*v[1],Q(1,0)*v[0]+Q(1,1)*v[1]};}
    Vec provisional(double dt,V2 gravity={0,0},const std::map<int,V3>&body_force={},V2 uniform_velocity={0,0})const{
        require(std::isfinite(dt)&&dt>=0&&std::isfinite(gravity[0])&&std::isfinite(gravity[1])&&std::isfinite(uniform_velocity[0])&&std::isfinite(uniform_velocity[1]),"Malformed provisional update input");
        for(const auto&e:body_force)require(body_offset.count(e.first)&&std::all_of(e.second.begin(),e.second.end(),[](double x){return std::isfinite(x);}),"Unknown body or nonfinite prescribed force");
        V2 base{uniform_velocity[0]+dt*gravity[0],uniform_velocity[1]+dt*gravity[1]};Vec w(H.size());
        for(int k=0;k<nf;++k){const Face&f=faces[k];w[k]=Q(0,f.axis)*base[0]+Q(1,f.axis)*base[1];
            if(f.owner==-1&&boundary.at(f.side)=="pressure"){V2 world=rotate(f.xy);world[0]+=origin[0];world[1]+=origin[1];double p=external_pressure(world);require(std::isfinite(p),"Nonfinite external pressure");w[k]-=f.adjacent[0][2]*f.area*dt*p/H[k];}}
        for(const auto&e:bodies){int off=body_offset.at(e.first);w[off]=base[0];w[off+1]=base[1];auto f=body_force.find(e.first);if(f!=body_force.end())for(int k=0;k<3;++k)w[off+k]+=dt*f->second[k]/H[off+k];}
        require(finite(w),"Nonfinite provisional velocity");return w;
    }
    Projection project(const Vec&wstar,const Vec*H_override=nullptr,const Vec*rhs_override=nullptr)const{return pf::project(B,H_override?*H_override:H,rhs_override?*rhs_override:rhs,wstar);}
    V3 surface_force(const Projection&p,int body,double dt)const{
        require(body_offset.count(body)&&std::isfinite(dt)&&dt>0&&p.impulses.size()==static_cast<size_t>(B.rows),"Invalid surface-force request");
        V3 out{0,0,0};int off=body_offset.at(body);
        // Read the exact transpose entries already assembled for compatibility.
        for(int row:body_rows.at(body))for(int k=0;k<3;++k)out[k]+=B(row,off+k)*p.impulses[row]/dt;
        return out;
    }
    double momentum_budget(const Projection&p,const Vec&wstar)const{
        require(wstar.size()==H.size()&&p.velocity.size()==H.size()&&p.impulses.size()==static_cast<size_t>(B.rows),"Momentum-budget shape mismatch");
        Vec external(H.size()),delta(H.size());for(int r:wall_rows)for(int j=0;j<B.cols;++j)external[j]+=B(r,j)*p.impulses[r];
        for(int r:support_rows)for(int j=0;j<B.cols;++j)external[j]+=B(r,j)*p.impulses[r];
        for(size_t j=0;j<H.size();++j)delta[j]=p.velocity[j]-wstar[j]-external[j]/H[j];
        return norm(mul(moment,delta))/std::max({1.,norm(mul(moment,wstar)),norm(mul(moment,p.velocity))});
    }
    double mass_partition_error()const{
        double m=0,mx=0,my=0;for(auto cell:cells)for(int a=0;a<2;++a)for(int b=0;b<2;++b)m+=rho[(2*cell.first+a)*(2*ny)+(2*cell.second+b)]*hx*hy/4;
        for(const Face&f:faces)(f.axis==0?mx:my)+=f.mass;
        return std::max(std::abs(mx-m),std::abs(my-m))/std::max(1.,m);
    }
    // Physical linear/angular impulse for a unit multiplier in each assembled row.
    // Cell rows and dynamic interface rows must close; wall/support rows are external.
    Matrix operator_closure()const{
        Matrix out(B.rows,3);for(int r=0;r<B.rows;++r)for(int k=0;k<3;++k)for(int j=0;j<B.cols;++j)out(r,k)+=moment(k,j)*B(r,j)/H[j];return out;
    }
};
inline Body geometry_body(int nx,int ny,const std::vector<int>&labels,int body,double density){
    require(nx>0&&ny>0&&labels.size()==static_cast<size_t>(nx)*ny&&body>=0&&std::isfinite(density)&&density>0,"Invalid body geometry inputs");
    double hx=1./nx,hy=1./ny;std::vector<V2>pieces;V2 centre{0,0};
    for(int i=0;i<nx;++i)for(int j=0;j<ny;++j)if(labels[i*ny+j]==body){V2 r{(i+.5)*hx,(j+.5)*hy};pieces.push_back(r);centre[0]+=r[0];centre[1]+=r[1];}
    require(!pieces.empty(),"Body geometry contains no cells");centre[0]/=pieces.size();centre[1]/=pieces.size();double inertia=0;
    for(V2 r:pieces){r[0]-=centre[0];r[1]-=centre[1];inertia+=density*hx*hy*((hx*hx+hy*hy)/12+dot(r,r));}
    Body out{density*pieces.size()*hx*hy,inertia,centre};require(std::isfinite(out.mass)&&std::isfinite(out.inertia)&&out.mass>0&&out.inertia>0,"Body mass/inertia overflow or underflow");return out;
}
inline Vec layered_density(int nx,int ny,double rhol,double rhog,double yfill){
    require(nx>0&&ny>0&&std::isfinite(rhol)&&std::isfinite(rhog)&&rhol>0&&rhog>0&&std::isfinite(yfill),"Invalid analytical layered-density inputs");
    Vec out(static_cast<size_t>(2*nx)*(2*ny));double h=1./(2*ny);
    for(int i=0;i<2*nx;++i)for(int j=0;j<2*ny;++j){double length=std::max(0.,std::min((j+1)*h,yfill)-j*h);out[i*(2*ny)+j]=rhog+(rhol-rhog)*length/h;}
    return out;
}
} // namespace pf
