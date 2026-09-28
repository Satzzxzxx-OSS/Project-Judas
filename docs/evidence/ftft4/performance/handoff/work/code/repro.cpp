#include <iostream>
#include "value_carriers.hpp"
#include "../generated/original.h"
#include "../generated/sign_selected.h"
int main(){
 double al=-0x1.810ae7fd12b45p+709,ah=-0x1.97a206614d0a9p-531,bl=-0x1.c710ac9010679p-709,bh=0x1.72d92dca4145cp+872;
 original::Iv a{al,ah},b{bl,bh};sign_selected::Iv c{al,ah},d{bl,bh};auto x=a*b;auto y=c*d;
 std::cout<<std::hexfloat<<x.lo<<' '<<x.hi<<' '<<y.lo<<' '<<y.hi<<" raw "<<al*bl<<' '<<al*bh<<' '<<std::isfinite(al)<<' '<<std::isfinite(bl)<<'\n';
}
