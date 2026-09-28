#include <memory>
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <cmath>
#include <limits>
#include <functional>
#include <cfenv>
#include "value_carriers.hpp"
#include "../generated/original.h"
#include "../generated/exponent.h"
#include "../generated/hoisted.h"
#include "../generated/prepared.h"
#include "../generated/sign_selected.h"

std::uint64_t bits(double x){std::uint64_t v;std::memcpy(&v,&x,8);return v;}
double value(std::uint64_t x){double v;std::memcpy(&v,&x,8);return v;}
bool same(double a,double b){return bits(a)==bits(b)||(std::isnan(a)&&std::isnan(b));}
template<class A,class B> bool eqiv(const A&a,const B&b){return same(a.lo,b.lo)&&same(a.hi,b.hi);}
std::uint64_t checks=0,failures=0;
void check(bool ok){++checks;if(!ok)++failures;}
struct ShapeInput {std::string kind;ContactPose p;glm::vec3 half;float radius=0;std::vector<std::pair<glm::vec3,glm::vec3>> children;};
bool readShape(std::istream&in,ShapeInput&a){int n;in>>a.kind>>a.p.position.x>>a.p.position.y>>a.p.position.z>>a.p.orientation.w>>a.p.orientation.x>>a.p.orientation.y>>a.p.orientation.z>>a.radius>>a.half.x>>a.half.y>>a.half.z>>n;for(int i=0;i<n;++i){glm::vec3 c,h;in>>c.x>>c.y>>c.z>>h.x>>h.y>>h.z;a.children.push_back({c,h});}return bool(in);}
struct Primitive {std::string kind;ContactPose p;glm::vec3 half;float radius;};
std::vector<Primitive> primitives(const ShapeInput&a){if(a.children.empty())return {{a.kind,a.p,a.half,a.radius}};std::vector<Primitive> out;for(auto&c:a.children){auto p=a.p;p.localCenter=c.first;out.push_back({"box",p,c.second,0});}return out;}
struct Verdict {bool hit;int sign;std::uint64_t predicates=0,fallbacks=0;};

Verdict classify_original(Primitive a,Primitive b,float margin){
 using namespace original;
 if(a.kind=="box"&&b.kind=="sphere")std::swap(a,b);
 Pair<Iv> ip(a.p,b.p);std::unique_ptr<Pair<Exact>> ep;
 Verdict v{true,-1};
 auto exact=[&]()->const Pair<Exact>&{if(!ep)ep=std::make_unique<Pair<Exact>>(a.p,b.p);return *ep;};
 auto sign=[&](Iv x,auto f){++v.predicates;if(std::isfinite(x.lo)&&std::isfinite(x.hi)){if(x.lo>0)return 1;if(x.hi<0)return -1;if(x.lo==0&&x.hi==0)return 0;}++v.fallbacks;return f().Sign();};
 if(a.kind=="sphere"&&b.kind=="sphere"){
  Iv r=Iv(double(a.radius))+Iv(double(b.radius));
  v.sign=sign(SpherePredicate(ip,r),[&]{return SpherePredicate(exact(),Exact(double(a.radius))+Exact(double(b.radius)));});
  if(v.sign>0)v.hit=sign(SpherePredicate(ip,r+Iv(double(margin))),[&]{return SpherePredicate(exact(),Exact(double(a.radius))+Exact(double(b.radius))+Exact(double(margin)));})<=0;
 }else if(a.kind=="sphere"){
  v.sign=sign(SphereBoxPredicate(ip,b.half,Iv(double(a.radius))),[&]{return SphereBoxPredicate(exact(),b.half,Exact(double(a.radius)));});
  if(v.sign>0)v.hit=sign(SphereBoxPredicate(ip,b.half,Iv(double(a.radius))+Iv(double(margin))),[&]{return SphereBoxPredicate(exact(),b.half,Exact(double(a.radius))+Exact(double(margin)));})<=0;
 }else{
  for(int axis=0;axis<15;++axis){
   auto ax=Axis(ip,axis);Iv len2=Dot(ax,ax);
   if(sign(len2,[&]{auto t=Axis(exact(),axis);return Dot(t,t);})==0)continue;
   Iv s=Sat(ip,a.half,b.half,axis);
   int sg=sign(s,[&]{return Sat(exact(),a.half,b.half,axis);});v.sign=std::max(v.sign,sg);
   if(sg>0){if(margin==0){v.hit=false;break;}Iv m{double(margin)};
    if(sign(s*s-m*m*ip.denominator*ip.denominator*len2,[&]{const auto&e=exact();auto t=Axis(e,axis);auto es=Sat(e,a.half,b.half,axis);Exact em{double(margin)};return es*es-em*em*e.denominator*e.denominator*Dot(t,t);})>0){v.hit=false;break;}
   }
  }
 }
 return v;
}

Verdict classify_hoisted(Primitive a,Primitive b,float margin){
 using namespace hoisted;
 if(a.kind=="box"&&b.kind=="sphere")std::swap(a,b);
 Pair<Iv> ip(a.p,b.p);std::unique_ptr<Pair<Exact>> ep;
 Verdict v{true,-1};
 auto exact=[&]()->const Pair<Exact>&{if(!ep)ep=std::make_unique<Pair<Exact>>(a.p,b.p);return *ep;};
 auto sign=[&](Iv x,auto f){++v.predicates;if(std::isfinite(x.lo)&&std::isfinite(x.hi)){if(x.lo>0)return 1;if(x.hi<0)return -1;if(x.lo==0&&x.hi==0)return 0;}++v.fallbacks;return f().Sign();};
 if(a.kind=="sphere"&&b.kind=="sphere"){
  Iv r=Iv(double(a.radius))+Iv(double(b.radius));
  v.sign=sign(SpherePredicate(ip,r),[&]{return SpherePredicate(exact(),Exact(double(a.radius))+Exact(double(b.radius)));});
  if(v.sign>0)v.hit=sign(SpherePredicate(ip,r+Iv(double(margin))),[&]{return SpherePredicate(exact(),Exact(double(a.radius))+Exact(double(b.radius))+Exact(double(margin)));})<=0;
 }else if(a.kind=="sphere"){
  v.sign=sign(SphereBoxPredicate(ip,b.half,Iv(double(a.radius))),[&]{return SphereBoxPredicate(exact(),b.half,Exact(double(a.radius)));});
  if(v.sign>0)v.hit=sign(SphereBoxPredicate(ip,b.half,Iv(double(a.radius))+Iv(double(margin))),[&]{return SphereBoxPredicate(exact(),b.half,Exact(double(a.radius))+Exact(double(margin)));})<=0;
 }else{
  for(int axis=0;axis<15;++axis){
   auto ax=Axis(ip,axis);Iv len2=Dot(ax,ax);
   if(sign(len2,[&]{auto t=Axis(exact(),axis);return Dot(t,t);})==0)continue;
   Iv s=Sat(ip,a.half,b.half,axis);
   int sg=sign(s,[&]{return Sat(exact(),a.half,b.half,axis);});v.sign=std::max(v.sign,sg);
   if(sg>0){if(margin==0){v.hit=false;break;}Iv m{double(margin)};
    if(sign(s*s-m*m*ip.denominator*ip.denominator*len2,[&]{const auto&e=exact();auto t=Axis(e,axis);auto es=Sat(e,a.half,b.half,axis);Exact em{double(margin)};return es*es-em*em*e.denominator*e.denominator*Dot(t,t);})>0){v.hit=false;break;}
   }
  }
 }
 return v;
}
Verdict classify_sign_selected(Primitive a,Primitive b,float margin){
 using namespace sign_selected;
 if(a.kind=="box"&&b.kind=="sphere")std::swap(a,b);
 Pair<Iv> ip(a.p,b.p);std::unique_ptr<Pair<Exact>> ep;
 Verdict v{true,-1};
 auto exact=[&]()->const Pair<Exact>&{if(!ep)ep=std::make_unique<Pair<Exact>>(a.p,b.p);return *ep;};
 auto sign=[&](Iv x,auto f){++v.predicates;if(std::isfinite(x.lo)&&std::isfinite(x.hi)){if(x.lo>0)return 1;if(x.hi<0)return -1;if(x.lo==0&&x.hi==0)return 0;}++v.fallbacks;return f().Sign();};
 if(a.kind=="sphere"&&b.kind=="sphere"){
  Iv r=Iv(double(a.radius))+Iv(double(b.radius));
  v.sign=sign(SpherePredicate(ip,r),[&]{return SpherePredicate(exact(),Exact(double(a.radius))+Exact(double(b.radius)));});
  if(v.sign>0)v.hit=sign(SpherePredicate(ip,r+Iv(double(margin))),[&]{return SpherePredicate(exact(),Exact(double(a.radius))+Exact(double(b.radius))+Exact(double(margin)));})<=0;
 }else if(a.kind=="sphere"){
  v.sign=sign(SphereBoxPredicate(ip,b.half,Iv(double(a.radius))),[&]{return SphereBoxPredicate(exact(),b.half,Exact(double(a.radius)));});
  if(v.sign>0)v.hit=sign(SphereBoxPredicate(ip,b.half,Iv(double(a.radius))+Iv(double(margin))),[&]{return SphereBoxPredicate(exact(),b.half,Exact(double(a.radius))+Exact(double(margin)));})<=0;
 }else{
  for(int axis=0;axis<15;++axis){
   auto ax=Axis(ip,axis);Iv len2=Dot(ax,ax);
   if(sign(len2,[&]{auto t=Axis(exact(),axis);return Dot(t,t);})==0)continue;
   Iv s=Sat(ip,a.half,b.half,axis);
   int sg=sign(s,[&]{return Sat(exact(),a.half,b.half,axis);});v.sign=std::max(v.sign,sg);
   if(sg>0){if(margin==0){v.hit=false;break;}Iv m{double(margin)};
    if(sign(s*s-m*m*ip.denominator*ip.denominator*len2,[&]{const auto&e=exact();auto t=Axis(e,axis);auto es=Sat(e,a.half,b.half,axis);Exact em{double(margin)};return es*es-em*em*e.denominator*e.denominator*Dot(t,t);})>0){v.hit=false;break;}
   }
  }
 }
 return v;
}
struct Work {ContactPose a,b;glm::vec3 ha,hb;};
std::vector<Work> works;
struct Saved {prepared::Rotation<prepared::Iv> a,b;Saved(const Work&w):a(w.a.orientation),b(w.b.orientation){}};
std::vector<Saved> saved;

#if defined(__GNUC__)
__attribute__((noinline))
#endif
double kernel_original(int count){
 using namespace original;double sum=0;
 for(int i=0;i<count;++i){const std::size_t j=std::size_t(i)%works.size();const auto&w=works[j];
 Pair<Iv> p(w.a,w.b);
 for(int axis=0;axis<15;++axis){auto t=Axis(p,axis);auto l=Dot(t,t);auto s=Sat(p,w.ha,w.hb,axis);sum+=s.lo+s.hi+l.lo+l.hi;}
 }
 return sum;
}

#if defined(__GNUC__)
__attribute__((noinline))
#endif
double kernel_exponent(int count){
 using namespace exponent;double sum=0;
 for(int i=0;i<count;++i){const std::size_t j=std::size_t(i)%works.size();const auto&w=works[j];
 Pair<Iv> p(w.a,w.b);
 for(int axis=0;axis<15;++axis){auto t=Axis(p,axis);auto l=Dot(t,t);auto s=Sat(p,w.ha,w.hb,axis);sum+=s.lo+s.hi+l.lo+l.hi;}
 }
 return sum;
}

#if defined(__GNUC__)
__attribute__((noinline))
#endif
double kernel_hoisted(int count){
 using namespace hoisted;double sum=0;
 for(int i=0;i<count;++i){const std::size_t j=std::size_t(i)%works.size();const auto&w=works[j];
 Pair<Iv> p(w.a,w.b);
 for(int axis=0;axis<15;++axis){auto t=Axis(p,axis);auto l=Dot(t,t);auto s=Sat(p,w.ha,w.hb,axis);sum+=s.lo+s.hi+l.lo+l.hi;}
 }
 return sum;
}

#if defined(__GNUC__)
__attribute__((noinline))
#endif
double kernel_prepared(int count){
 using namespace prepared;double sum=0;
 for(int i=0;i<count;++i){const std::size_t j=std::size_t(i)%works.size();const auto&w=works[j];
 Pair<Iv> p(w.a,w.b,saved[j].a,saved[j].b);
 for(int axis=0;axis<15;++axis){auto t=Axis(p,axis);auto l=Dot(t,t);auto s=Sat(p,w.ha,w.hb,axis);sum+=s.lo+s.hi+l.lo+l.hi;}
 }
 return sum;
}
#if defined(__GNUC__)
__attribute__((noinline))
#endif
double kernel_sign_selected(int count){
 using namespace sign_selected;double sum=0;
 for(int i=0;i<count;++i){const std::size_t j=std::size_t(i)%works.size();const auto&w=works[j];
 Pair<Iv> p(w.a,w.b);
 for(int axis=0;axis<15;++axis){auto t=Axis(p,axis);auto l=Dot(t,t);auto s=Sat(p,w.ha,w.hb,axis);sum+=s.lo+s.hi+l.lo+l.hi;}
 }
 return sum;
}
int main(int argc,char**argv){
 std::cout<<std::setprecision(17);
 if(std::fegetround()!=FE_TONEAREST){std::cerr<<"round-to-nearest required\n";return 2;}
 std::mt19937_64 rng(0x7af4ab32);
 // Exhaustive normal exponent-field pairs. This is exact integer algebra, not
 // an exact-geometry test; compare the guard with the original exponent rule.
 for(unsigned a=1;a<2047;++a)for(unsigned b=1;b<2047;++b){
  double x=value(std::uint64_t(a)<<52),y=value(std::uint64_t(b)<<52);
  check(exponent::ResidualExponentGuard(x,y)==(int(a)+int(b)-2046>=-968));
 }
 const auto expChecks=checks;
 // Subnormals keep the source's ilogb path; compare signs and extreme mantissas.
 for(int i=0;i<10000;++i){
  double a=value((rng()&((1ULL<<52)-1))|1),b=value((rng()%2046+1)<<52);
  check(exponent::ResidualExponentGuard(a,b)==(std::ilogb(a)+std::ilogb(b)>=-968));
 }
 const auto guardChecks=checks;
 // Endpoint-equivalence of original and candidate interval operations.
 std::vector<double> edge={0.,-0.,1.,-1.,2.,.5,std::numeric_limits<double>::min(),std::numeric_limits<double>::denorm_min(),std::numeric_limits<double>::max()};
 for(int e:{-1022,-1021,-969,-968,-967,-512,-500,-100,-1,0,1,24,53,100,500,1023}){double d=std::ldexp(1.,e);edge.push_back(d);edge.push_back(std::nextafter(d,0.));edge.push_back(std::nextafter(d,INFINITY));}
 auto testOp=[&](double al,double ah,double bl,double bh){
  original::Iv a(al,ah),b(bl,bh);hoisted::Iv c(al,ah),d(bl,bh);
  check(eqiv(a*b,c*d));check(eqiv(a+b,c+d));
  sign_selected::Iv sa(al,ah),sb(bl,bh);
  auto dump=[&](const char* op,auto u,auto v){static int printed=0;if(printed++<12)std::cerr<<std::hexfloat<<op<<" a="<<al<<","<<ah<<" b="<<bl<<","<<bh<<" old="<<u.lo<<","<<u.hi<<" new="<<v.lo<<","<<v.hi<<"\n";};
  check(eqiv(a*b,sa*sb));if(!eqiv(a*b,sa*sb))dump("mul",a*b,sa*sb);
  if (!(bl<=0 && bh>=0)){check(eqiv(a/b,sa/sb));if(!eqiv(a/b,sa/sb))dump("div",a/b,sa/sb);}
 };
 for(double a:edge)for(double b:edge)testOp(a,a,b,b);
 for(int i=0;i<250000;++i){
  auto f=[&]{std::uint64_t q=rng();q=(q&~(0x7ffULL<<52))|((q%2047)<<52);return value(q);};
  double a=f(),b=f();if(i%2==0)testOp(a,a,b,b);
  else{double aa=f(),bb=f();testOp(std::min(a,aa),std::max(a,aa),std::min(b,bb),std::max(b,bb));}
 }
 const auto arithmeticChecks=checks-guardChecks;
 // Original quaternion expression versus hoisted products, for intervals,
 // ordinary numbers, and manageable exact expansions.
 for(int i=0;i<12000;++i){auto f=[&]{return float(std::ldexp(double(int(rng()%2049)-1024)/1024.,int(rng()%200)-100));};
  glm::quat q(f(),f(),f(),f());if(i==0)q={0,0,0,0};if(i==1)q={1,0,0,0};
  original::Rotation<original::Iv> a(q);hoisted::Rotation<hoisted::Iv>b(q);
  original::Rotation<double>c(q);hoisted::Rotation<double>d(q);
  check(eqiv(a.d,b.d));check(same(c.d,d.d));
  for(int j=0;j<3;++j)for(int k=0;k<3;++k){check(eqiv(a.n[j][k],b.n[j][k]));check(same(c.n[j][k],d.n[j][k]));}
  if(i<500){original::Rotation<original::Exact>e(q);hoisted::Rotation<hoisted::Exact>f(q);check(e.d.e==f.d.e);for(int j=0;j<3;++j)for(int k=0;k<3;++k)check(e.n[j][k].e==f.n[j][k].e);}
 }
 const auto rotationChecks=checks-guardChecks-arithmeticChecks;
 std::size_t fixtureRows=0,primitivePairs=0,originalFallbacks=0,candidateFallbacks=0,comparisonFailures=0,oracleVerdictFailures=0;
 if(argc>1){std::ifstream in(argv[1]);if(!in)return 2;std::string line;
  while(std::getline(in,line)){if(line.empty())continue;std::istringstream is(line);std::string id;float margin;int world,n;is>>id>>margin>>world>>n;std::vector<int>expect(n),sg(n);for(int&x:expect)is>>x;for(int&x:sg)is>>x;ShapeInput a,b;if(!readShape(is,a)||!readShape(is,b))return 2;
   auto aa=primitives(a),bb=primitives(b);int pair=0;
   for(auto&x:aa)for(auto&y:bb){auto p=classify_original(x,y,margin),q=classify_hoisted(x,y,margin);bool equal=p.hit==q.hit&&p.sign==q.sign&&p.predicates==q.predicates&&p.fallbacks==q.fallbacks;check(equal);if(!equal)++comparisonFailures;
    auto r=classify_sign_selected(x,y,margin);check(r.hit==p.hit&&r.sign==p.sign&&r.predicates==p.predicates&&r.fallbacks==p.fallbacks);
    // Only pair hit is a complete decision here; this harness does not build manifolds.
    if(q.hit!=bool(expect.at(pair)))++oracleVerdictFailures;
    originalFallbacks+=p.fallbacks;candidateFallbacks+=q.fallbacks;++primitivePairs;++pair;
   }
   ++fixtureRows;
  }
 }
 std::cout<<"{\"kind\":\"validation\",\"checks\":"<<checks<<",\"failures\":"<<failures<<",\"normal_exponent_pairs\":"<<expChecks<<",\"interval_endpoint_checks\":"<<arithmeticChecks<<",\"rotation_checks\":"<<rotationChecks<<",\"fixture_rows\":"<<fixtureRows<<",\"primitive_pairs\":"<<primitivePairs<<",\"predicate_comparison_failures\":"<<comparisonFailures<<",\"oracle_hit_failures\":"<<oracleVerdictFailures<<",\"original_fallbacks\":"<<originalFallbacks<<",\"candidate_fallbacks\":"<<candidateFallbacks<<"}\n";
 if(failures||oracleVerdictFailures)return 1;
 std::array<double(*)(int),5> fs={kernel_original,kernel_exponent,kernel_hoisted,kernel_prepared,kernel_sign_selected};
 const std::array<const char*,5>names={"original","exponent_guard","hoisted_products","prepared_rotations","sign_selected"};
 volatile double sink=0;
 for(int workload=0;workload<3;++workload){works.clear();saved.clear();
  for(int i=0;i<1536;++i){auto f=[&]{return float(double(int(rng()%20001)-10000)/10000.);};
   Work w;w.a.position={1.5f*float(i%50)-37.f,.499f,1.5f*float(i/50)-22.f};w.b.position={0,-1,0};w.ha=glm::vec3(.5f);w.hb={100,1,100};
   if(workload==1)w.a.orientation={1,f()*1e-6f,f()*1e-6f,f()*1e-6f};
   if(workload==2){w.a.orientation={f(),f(),f(),f()};w.b.orientation={f(),f(),f(),f()};w.a.localCenter={f(),f(),f()};w.b.localCenter={f(),f(),f()};}
   works.push_back(w);saved.emplace_back(w);
  }
  for(auto fn:fs)sink=sink+fn(1536);
  for(int trial=0;trial<7;++trial){for(int k=0;k<5;++k){int v=(k+trial)%5;auto t0=std::chrono::steady_clock::now();double x=fs[v](6144);auto t1=std::chrono::steady_clock::now();sink=sink+x;double ms=std::chrono::duration<double,std::milli>(t1-t0).count();
   std::cout<<"{\"kind\":\"microbenchmark\",\"workload\":"<<workload<<",\"trial\":"<<trial<<",\"variant\":\""<<names[v]<<"\",\"pairs\":6144,\"ms\":"<<ms<<",\"checksum\":"<<x<<"}\n";
  }}
 }
 if(!std::isfinite(sink))return 3;
}
