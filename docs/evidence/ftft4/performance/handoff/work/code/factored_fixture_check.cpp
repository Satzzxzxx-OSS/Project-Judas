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

namespace prepared {
#include "sat_workspace.hpp"
}
Verdict classify_factored(Primitive a,Primitive b,float margin){
 using namespace prepared;
 if(a.kind=="box"&&b.kind=="sphere")std::swap(a,b);
 Pair<Iv> ip(a.p,b.p);SatWorkspace<Iv> ws(ip);std::unique_ptr<Pair<Exact>> ep;
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
   Iv s=ws.Evaluate(a.half,b.half,axis,ax);
   int sg=sign(s,[&]{return Sat(exact(),a.half,b.half,axis);});v.sign=std::max(v.sign,sg);
   if(sg>0){if(margin==0){v.hit=false;break;}Iv m{double(margin)};
    if(sign(s*s-m*m*ip.denominator*ip.denominator*len2,[&]{const auto&e=exact();auto t=Axis(e,axis);auto es=Sat(e,a.half,b.half,axis);Exact em{double(margin)};return es*es-em*em*e.denominator*e.denominator*Dot(t,t);})>0){v.hit=false;break;}
   }
  }
 }
 return v;
}

int main(int argc,char**argv){if(argc!=2)return 2;std::ifstream in(argv[1]);if(!in)return 2;std::string line;size_t count=0,pairs=0,oldFall=0,newFall=0,oracleFails=0,changedCounts=0;
while(std::getline(in,line)){if(line.empty())continue;std::istringstream is(line);std::string id;float margin;int world,n;is>>id>>margin>>world>>n;std::vector<int>expect(n),sg(n);for(int&x:expect)is>>x;for(int&x:sg)is>>x;ShapeInput a,b;if(!readShape(is,a)||!readShape(is,b))return 2;
auto aa=primitives(a),bb=primitives(b);int j=0;for(auto&x:aa)for(auto&y:bb){auto p=classify_original(x,y,margin),q=classify_factored(x,y,margin);check(p.hit==q.hit&&p.sign==q.sign);if(q.hit!=bool(expect.at(j)))++oracleFails;oldFall+=p.fallbacks;newFall+=q.fallbacks;if(p.fallbacks!=q.fallbacks)++changedCounts;++pairs;++j;}++count;}
std::cout<<"{\"fixtures\":"<<count<<",\"primitive_pairs\":"<<pairs<<",\"verdict_difference_count\":"<<failures<<",\"oracle_hit_failures\":"<<oracleFails<<",\"original_fallbacks\":"<<oldFall<<",\"factored_fallbacks\":"<<newFall<<",\"pairs_with_changed_fallback_count\":"<<changedCounts<<"}\n";return (failures||oracleFails)?1:0;}
