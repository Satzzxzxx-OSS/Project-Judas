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
#include <memory>
#include <cfenv>
#include "value_carriers.hpp"
#include "../generated/original.h"
#include "../generated/prepared.h"
namespace prepared {
#include "sat_workspace.hpp"
}
struct Row {ContactPose a,b;glm::vec3 ha,hb;};
struct Saved {prepared::Rotation<prepared::Iv>a,b;Saved(const Row&r):a(r.a.orientation),b(r.b.orientation){}};
std::uint64_t checks=0,failures=0,strictOriginal=0,strictNew=0,exactZeros=0,rangeFailures=0;
void check(bool b){++checks;if(!b)++failures;}
int intervalSign(double lo,double hi){if(!std::isfinite(lo)||!std::isfinite(hi))return 2;if(lo>0)return 1;if(hi<0)return -1;if(lo==0&&hi==0)return 0;return 2;}
std::vector<Row>rows;std::vector<Saved>saved;
__attribute__((noinline)) double reference_kernel(int count){double s=0;for(int i=0;i<count;++i){const auto&r=rows[i%rows.size()];original::Pair<original::Iv>p(r.a,r.b);for(int axis=0;axis<15;++axis){auto t=original::Axis(p,axis);auto len=original::Dot(t,t);auto x=original::Sat(p,r.ha,r.hb,axis);s+=x.lo+x.hi+len.lo+len.hi;}}return s;}
__attribute__((noinline)) double prepared_kernel(int count,bool cached){double s=0;for(int i=0;i<count;++i){const auto j=i%rows.size();const auto&r=rows[j];auto p=cached?prepared::Pair<prepared::Iv>(r.a,r.b,saved[j].a,saved[j].b):prepared::Pair<prepared::Iv>(r.a,r.b);prepared::SatWorkspace<prepared::Iv>q(p);for(int axis=0;axis<15;++axis){auto t=prepared::Axis(p,axis);auto len=prepared::Dot(t,t);auto x=q.Evaluate(r.ha,r.hb,axis,t);s+=x.lo+x.hi+len.lo+len.hi;}}return s;}
void validate(const Row&r){
 original::Pair<original::Iv>a(r.a,r.b);prepared::Pair<prepared::Iv>b(r.a,r.b);prepared::SatWorkspace<prepared::Iv>qw(b);
 original::Pair<original::Exact>ae(r.a,r.b);prepared::Pair<prepared::Exact>be(r.a,r.b);prepared::SatWorkspace<prepared::Exact>eqw(be);
 for(int axis=0;axis<15;++axis){try{
  auto t=prepared::Axis(b,axis);auto old=original::Sat(a,r.ha,r.hb,axis);auto now=qw.Evaluate(r.ha,r.hb,axis,t);
  auto oe=original::Sat(ae,r.ha,r.hb,axis);auto ne=eqw.Evaluate(r.ha,r.hb,axis,prepared::Axis(be,axis));
  // Compare exact expansion difference: equivalent polynomial, not merely sign.
  original::Exact diff=oe;for(double v:ne.e)diff.Grow(-v);check(diff.Sign()==0);
  const int truth=oe.Sign(),sa=intervalSign(old.lo,old.hi),sb=intervalSign(now.lo,now.hi);
  if(truth==0)++exactZeros;
  if(sa!=2){check(sa==truth);++strictOriginal;}
  if(sb!=2){check(sb==truth);++strictNew;}
  // Exact dyadic inequality against candidate endpoints, not conversion via long double.
  if(std::isfinite(now.lo)){original::Exact d=oe-original::Exact(now.lo);check(d.Sign()>=0);}
  if(std::isfinite(now.hi)){original::Exact d=original::Exact(now.hi)-oe;check(d.Sign()>=0);}
 }catch(const original::ArithmeticFailure&){++rangeFailures;}catch(const prepared::ArithmeticFailure&){++rangeFailures;}}
}
int main(){if(std::fegetround()!=FE_TONEAREST)return 2;std::cout<<std::setprecision(17);std::mt19937_64 rng(0xf4af4a);
 for(int i=0;i<1200;++i){auto f=[&]{return float(double(int(rng()%20001)-10000)/1000.);};Row r;r.a.position={f()*100,f()*100,f()*100};r.b.position={f()*100,f()*100,f()*100};r.ha={std::abs(f())+.01f,std::abs(f())+.01f,std::abs(f())+.01f};r.hb=r.ha;r.a.localCenter={f(),f(),f()};r.b.localCenter={f(),f(),f()};
  if(i%3==0){r.a.orientation={1,0,0,0};r.b.orientation=r.a.orientation;}
  if(i%3==1){r.a.orientation={1,f()*1e-7f,f()*1e-7f,f()*1e-7f};r.b.orientation={1,f()*1e-7f,f()*1e-7f,f()*1e-7f};}
  if(i%3==2){r.a.orientation={f(),f(),f(),f()};r.b.orientation={f(),f(),f(),f()};}
  validate(r);
 }
 std::cout<<"{\"kind\":\"sat_validation\",\"pairs\":1200,\"checks\":"<<checks<<",\"failures\":"<<failures<<",\"range_failures\":"<<rangeFailures<<",\"original_resolved\":"<<strictOriginal<<",\"factored_resolved\":"<<strictNew<<",\"exact_zeros\":"<<exactZeros<<"}\n";
 if(failures||rangeFailures)return 1;
 volatile double sink=0;
 for(int w=0;w<3;++w){rows.clear();saved.clear();for(int i=0;i<1536;++i){auto f=[&]{return float(double(int(rng()%20001)-10000)/10000.);};Row r;r.a.position={1.5f*float(i%50)-37.f,.499f,1.5f*float(i/50)-22.f};r.b.position={0,-1,0};r.ha={.5,.5,.5};r.hb={100,1,100};
  if(w==1)r.a.orientation={1,f()*1e-6f,f()*1e-6f,f()*1e-6f};
  if(w==2){r.a.orientation={f(),f(),f(),f()};r.b.orientation={f(),f(),f(),f()};r.a.localCenter={f(),f(),f()};r.b.localCenter={f(),f(),f()};}
  rows.push_back(r);saved.emplace_back(r);}
  sink=sink+reference_kernel(1536);sink=sink+prepared_kernel(1536,false);sink=sink+prepared_kernel(1536,true);
  for(int trial=0;trial<7;++trial)for(int k=0;k<3;++k){const int v=(k+trial)%3;auto start=std::chrono::steady_clock::now();double x=v==0?reference_kernel(6144):prepared_kernel(6144,v==2);auto stop=std::chrono::steady_clock::now();sink=sink+x;std::cout<<"{\"kind\":\"sat_benchmark\",\"workload\":"<<w<<",\"variant\":"<<v<<",\"trial\":"<<trial<<",\"pairs\":6144,\"ms\":"<<std::chrono::duration<double,std::milli>(stop-start).count()<<",\"checksum\":"<<x<<"}\n";}
 }
 if(!std::isfinite(sink))return 3;
}
