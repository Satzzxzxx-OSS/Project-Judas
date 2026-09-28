#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include <cmath>
#include "value_carriers.hpp"
#include "../generated/original.h"
using namespace original;
struct Box {glm::quat q;glm::vec3 h,o;};
struct Bound {std::array<float,3>lo,hi;};
float LowerFloat(double x){float f=static_cast<float>(x);return double(f)>x?std::nextafter(f,-std::numeric_limits<float>::infinity()):f;}
float UpperFloat(double x){float f=static_cast<float>(x);return double(f)<x?std::nextafter(f,std::numeric_limits<float>::infinity()):f;}
// Same scalar arithmetic and association as supplied Narrowphase.cpp::BoxAabb.
Bound Uncached(const Box&b,const glm::vec3&position){Rotation<Iv>r(b.q);Bound out;for(int k=0;k<3;++k){Iv center(double(position[k])),extent(0);for(int j=0;j<3;++j){const Iv entry=r.n[j][k]/r.d;center=center+entry*Iv(double(b.o[j]));extent=extent+Abs(entry)*Iv(double(b.h[j]));}out.lo[k]=LowerFloat((center-extent).lo);out.hi[k]=UpperFloat((center+extent).hi);}return out;}
struct PreparedBox {
 std::array<std::array<Iv,3>,3>terms;
 std::array<Iv,3>extent;
 explicit PreparedBox(const Box&b){Rotation<Iv>r(b.q);for(int k=0;k<3;++k){Iv e(0);for(int j=0;j<3;++j){const Iv entry=r.n[j][k]/r.d;terms[k][j]=entry*Iv(double(b.o[j]));e=e+Abs(entry)*Iv(double(b.h[j]));}extent[k]=e;}}
 Bound At(const glm::vec3&position)const{Bound out;for(int k=0;k<3;++k){Iv center(double(position[k]));for(int j=0;j<3;++j)center=center+terms[k][j];out.lo[k]=LowerFloat((center-extent[k]).lo);out.hi[k]=UpperFloat((center+extent[k]).hi);}return out;}
};
bool same(const Bound&a,const Bound&b){return std::memcmp(&a,&b,sizeof a)==0;}
struct Work{Box b;glm::vec3 p;PreparedBox saved;Work(const Box&bb,glm::vec3 pp):b(bb),p(pp),saved(bb){}};
std::vector<Work>work;
__attribute__((noinline)) double Run(int n,bool cache){double s=0;for(int i=0;i<n;++i){const auto&w=work[i%work.size()];auto b=cache?w.saved.At(w.p):Uncached(w.b,w.p);s+=double(b.lo[0])+b.hi[1]+b.lo[2];}return s;}
int main(){std::mt19937_64 rng(0x1094bad);std::cout<<std::setprecision(17);std::size_t checks=0,failures=0;
 for(int i=0;i<10000;++i){auto f=[&]{return float(double(int(rng()%20001)-10000)/10000.);};Box b{{f(),f(),f(),f()},{std::abs(f())+.01f,std::abs(f())+.01f,std::abs(f())+.01f},{f()*10,f()*10,f()*10}};if(i%4==0)b.q={1,0,0,0};if(i%4==1)b.q={1,f()*1e-6f,f()*1e-6f,f()*1e-6f};if(i%3==0)b.o={0,0,0};PreparedBox cached(b);
  for(float scale:{0.f,1.f,137.f,1024.f,32768.f,1048576.f}){glm::vec3 p{scale+f(),-scale+f(),2*scale+f()};auto a=Uncached(b,p),c=cached.At(p);++checks;if(!same(a,c))++failures;}
 }
 std::cout<<"{\"kind\":\"bounds_cache_validation\",\"cases\":"<<checks<<",\"bitwise_mismatches\":"<<failures<<"}\n";if(failures)return 1;
 volatile double sink=0;
 for(int kind=0;kind<3;++kind){work.clear();for(int i=0;i<1536;++i){auto f=[&]{return float(double(int(rng()%20001)-10000)/10000.);};Box b; b.h={.5,.5,.5};if(kind==0){b.q={1,0,0,0};b.o={0,0,0};}if(kind==1){b.q={1,f()*1e-6f,f()*1e-6f,f()*1e-6f};b.o={0,0,0};}if(kind==2){b.q={f(),f(),f(),f()};b.o={f(),f(),f()};}
  work.emplace_back(b,glm::vec3{f()*100,.5f,f()*100});}
  sink=sink+Run(1536,false);sink=sink+Run(1536,true);
  for(int t=0;t<7;++t)for(int k=0;k<2;++k){int v=(t+k)%2;auto a=std::chrono::steady_clock::now();double x=Run(24576,v==1);auto z=std::chrono::steady_clock::now();sink=sink+x;std::cout<<"{\"kind\":\"bounds_microbenchmark\",\"workload\":"<<kind<<",\"cached\":"<<(v?"true":"false")<<",\"trial\":"<<t<<",\"queries\":24576,\"ms\":"<<std::chrono::duration<double,std::milli>(z-a).count()<<",\"checksum\":"<<x<<"}\n";}
 }
 if(!std::isfinite(sink))return 3;
}
