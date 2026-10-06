// Bounded CPU sanity measurement, not an engineering certification or stress run.
#include "Deformable.h"
#include "UniformGravity.h"
#include <chrono>
#include <cstdlib>
#include <new>
#include <cstdio>
#include <algorithm>
static size_t allocations=0,allocatedBytes=0;static bool measuring=false;
void* operator new(size_t n){if(measuring){++allocations;allocatedBytes+=n;}if(auto p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}
int main(){PhysicsWorld physics;physics.Init();UniformGravity zero({0,0,0});std::string error;
 for(unsigned count:{30u,90u}){auto asset=std::make_shared<DeformableAsset>(MakeFractureBlock({count,1,1},{double(count),1,1},false));asset->fracture->tension=200;asset->fracture->shear=200;asset->fracture->compliance=1e-7;
  DeformableSettings cfg;cfg.selfContact=false;cfg.substeps=4;cfg.iterations=4;cfg.material.density=10;cfg.material.damping=1;cfg.material.shearCompliance=1e-6;cfg.material.volumeCompliance=1e-7;DeformableInstance s;s.Initialize(asset,cfg,glm::dmat4(1),count);
  for(const char* mode:{"intact","failing","settled","no-fracture"}){if(std::string(mode)=="failing")s.PartLoad(count/2,{30,0,0},true);if(std::string(mode)=="settled")for(int i=0;i<90;++i){s.Step(1./60,physics,zero,{});s.fracture.Commit(*asset->fracture);}allocations=allocatedBytes=0;std::vector<double> times;times.reserve(30);
   for(unsigned step=0;step<30;++step){auto start=std::chrono::steady_clock::now();measuring=true;if(std::string(mode)!="no-fracture"){s.Step(1./60,physics,zero,{});s.fracture.Commit(*asset->fracture);}else physics.Step(1.f/60);measuring=false;times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}
   std::sort(times.begin(),times.end());unsigned broken=0;for(auto b:s.fracture.broken)broken+=b!=0;
   // Known owned numeric storage only. Map/list allocator overhead and shared
   // immutable cook storage are intentionally excluded from this lower bound.
   size_t retained=s.positions.capacity()*sizeof(glm::dvec3)+s.previous.capacity()*sizeof(glm::dvec3)+s.velocities.capacity()*sizeof(glm::dvec3)+s.plasticRest.capacity()*sizeof(glm::dmat3)+s.fracture.multipliers.capacity()*sizeof(glm::dvec3)+s.fracture.broken.capacity()+s.fracture.removed.capacity()+s.fracture.component.capacity()*sizeof(unsigned);
   std::printf("CPU mode=%s parts=%u nodes=%zu tets=%zu interfaces=%zu broken=%u median_ms=%.6f p95_ms=%.6f max_ms=%.6f allocations=%zu allocated_bytes=%zu retained_numeric_bytes=%zu selfContact=false\n",mode,count,s.positions.size(),asset->solids.size(),asset->fracture->bonds.size(),broken,times[15],times[28],times.back(),allocations,allocatedBytes,retained);if(!s.error.empty()){std::fprintf(stderr,"%s\n",s.error.c_str());return 1;}
  }
 }
 return 0;
}
