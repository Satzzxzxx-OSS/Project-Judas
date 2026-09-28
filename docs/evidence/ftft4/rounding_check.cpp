// Independent equivalence check for the production adjacent-binary64 operation.
#include "Contacts.h"
#include "ContactGeometryInternal.h"
#include <cstdio>
#include <cstring>
#include <random>
int main() {
    std::uint64_t checks=0, failures=0;
    auto check=[&](double x) {
        for(int direction: {-1,1}) {
            const double expected=std::nextafter(x,direction*std::numeric_limits<double>::infinity());
            const double observed=direction<0?contact_geometry::Down(x):contact_geometry::Up(x);
            std::uint64_t a,b;std::memcpy(&a,&expected,8);std::memcpy(&b,&observed,8);
            ++checks;if(!(std::isnan(expected)&&std::isnan(observed))&&a!=b)++failures;
        }
    };
    const double e[]={0.0,-0.0,1.0,-1.0,std::numeric_limits<double>::min(),-std::numeric_limits<double>::min(),
        std::numeric_limits<double>::denorm_min(),-std::numeric_limits<double>::denorm_min(),
        std::numeric_limits<double>::max(),-std::numeric_limits<double>::max(),
        std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()};
    for(double x:e)check(x);
    std::mt19937_64 random(0x4f4654463441ULL);
    for(int i=0;i<1000000;++i){const auto bits=random();double x;std::memcpy(&x,&bits,8);check(x);}
    std::printf("adjacent-binary64 checks=%llu failures=%llu\n",(unsigned long long)checks,(unsigned long long)failures);
    return failures?1:0;
}
