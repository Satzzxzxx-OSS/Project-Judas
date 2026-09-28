// Separate DIAGNOSTIC linkage only. Timing acceptance never links this file.
#include "ContactAllocationInstrumentation.h"
#include <cstdlib>
#include <cstdint>
#include <new>
namespace {
allocation_evidence::Counts totals;
struct Header { void* raw; std::size_t bytes; };
void* allocate(std::size_t n,std::size_t alignment){
 alignment=alignment<alignof(Header)?alignof(Header):alignment;
 if(n>std::size_t(-1)-alignment-sizeof(Header))throw std::bad_alloc();
 void* raw=std::malloc(n+alignment+sizeof(Header));if(!raw)throw std::bad_alloc();
 auto address=(reinterpret_cast<std::uintptr_t>(raw)+sizeof(Header)+alignment-1)&~(std::uintptr_t(alignment)-1);
 auto* header=reinterpret_cast<Header*>(address)-1;header->raw=raw;header->bytes=n;
 ++totals.calls;totals.bytes+=n;totals.live+=n;if(totals.live>totals.peak)totals.peak=totals.live;
 return reinterpret_cast<void*>(address);
}
void release(void*p)noexcept{if(!p)return;auto*h=static_cast<Header*>(p)-1;totals.live-=h->bytes;std::free(h->raw);}
}
namespace allocation_evidence { Counts counts(){return totals;} }
void*operator new(std::size_t n){return allocate(n,alignof(std::max_align_t));}
void*operator new[](std::size_t n){return ::operator new(n);}
void*operator new(std::size_t n,std::align_val_t a){return allocate(n,std::size_t(a));}
void*operator new[](std::size_t n,std::align_val_t a){return ::operator new(n,a);}
void operator delete(void*p)noexcept{release(p);} void operator delete[](void*p)noexcept{release(p);}
void operator delete(void*p,std::size_t)noexcept{release(p);} void operator delete[](void*p,std::size_t)noexcept{release(p);}
void operator delete(void*p,std::align_val_t)noexcept{release(p);} void operator delete[](void*p,std::align_val_t)noexcept{release(p);}
void operator delete(void*p,std::size_t,std::align_val_t)noexcept{release(p);} void operator delete[](void*p,std::size_t,std::align_val_t)noexcept{release(p);}
void*operator new(std::size_t n,const std::nothrow_t&)noexcept{try{return ::operator new(n);}catch(...){return nullptr;}}
void*operator new[](std::size_t n,const std::nothrow_t&)noexcept{try{return ::operator new(n);}catch(...){return nullptr;}}
void operator delete(void*p,const std::nothrow_t&)noexcept{release(p);}void operator delete[](void*p,const std::nothrow_t&)noexcept{release(p);}
