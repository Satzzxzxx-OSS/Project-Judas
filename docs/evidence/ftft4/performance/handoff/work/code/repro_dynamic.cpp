#include <iostream>
#include <random>
#include <cstring>
#include "value_carriers.hpp"
#include "../generated/original.h"
#include "../generated/sign_selected.h"
double val(std::uint64_t x){double v;std::memcpy(&v,&x,8);return v;}
int main(){std::mt19937_64 r(88789);int fails=0;
 for(int i=0;i<100000;++i){auto f=[&]{auto x=r();x=(x&~(0x7ffULL<<52))|((x%2047)<<52);return val(x);};double al=f(),ah=f(),bl=f(),bh=f();if(al>ah)std::swap(al,ah);if(bl>bh)std::swap(bl,bh);original::Iv a{al,ah},b{bl,bh};sign_selected::Iv c{al,ah},d{bl,bh};auto x=a*b;auto y=c*d;
 if(x.lo!=y.lo||x.hi!=y.hi){if(fails++<5)std::cout<<std::hexfloat<<al<<' '<<ah<<' '<<bl<<' '<<bh<<" = "<<x.lo<<' '<<x.hi<<' '<<y.lo<<' '<<y.hi<<'\n';}}
 std::cout<<fails<<" failures\n";}
