// Independent binary32 probe of the REPORTED gamma8 rule; not imported Judas code.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
int main(){
    static_assert(std::numeric_limits<float>::is_iec559,"requires IEEE float");
    const float u=0x1p-24f;
    const float gamma=(8*u)/(1-8*u);
    std::cout << "translation,stored_gap,float_gap,promoted_gap,gamma8_bound,collapsed\n";
    for(float p:{0.f,137.f,1024.f,32768.f,1048576.f}){
        float b=p+1.f;
        b=std::nextafter(b,std::numeric_limits<float>::infinity());
        b=std::nextafter(b,std::numeric_limits<float>::infinity());
        float s=(b-p)-1.f;
        double exact=double(b)-double(p)-1.; // exact for this selected dyadic family
        float c=(p+b)*.5f;
        float eps=gamma*(std::abs(p)+std::abs(c-p)+std::abs(b)+std::abs(c-b));
        if(!(s>0 && double(s)==exact && s<=eps))return 1;
        std::cout<<std::setprecision(17)<<p<<','<<exact<<','<<s<<','<<exact<<','<<eps<<",true\n";
    }
}
