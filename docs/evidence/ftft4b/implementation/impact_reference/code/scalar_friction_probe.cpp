// Standalone scalar specialization of the current accumulated-impulse loop.
// This is NOT a build of ContactSolver/PhysicsWorld and does NOT exercise GLM.
// Normal n=(0,1,0), tangent=(1,0,0), mass=1, Izz=2/3, r=(1,-1,0).
// An impulse (Pt,Pn) changes spin by 1.5*(Pt+Pn).
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

template<class T> void run(int count, const char* name) {
    T vx=T(-.1), vy=T(-1), w=T(0), pn=T(0), pt=T(0);
    const T inverseInertia=T(1.5), mu=T(.8), target=T(1), k=T(2.5);
    auto apply=[&](T x,T y){vx+=x;vy+=y;w+=inverseInertia*(x+y);};
    const long double K0=(static_cast<long double>(vx)*vx+static_cast<long double>(vy)*vy)/2;
    for(int i=0;i<count;++i) {
        T vn=vy+w;
        T next=std::max(T(0),pn+(target-vn)/k);
        apply(T(0),next-pn); pn=next;
        T slip=vx+w;
        T nxt=pt;
        if(std::abs(slip)>T(1e-6)) nxt-=slip/k;
        T limit=mu*pn;
        if(std::abs(nxt)>limit) nxt=std::copysign(limit,nxt);
        apply(nxt-pt,T(0));pt=nxt;
    }
    const long double K1=(static_cast<long double>(vx)*vx+static_cast<long double>(vy)*vy)/2+
                         static_cast<long double>(w)*w/3;
    std::cout << "  \"" << name << "\":{\"iterations\":" << count
       << ",\"energy_before\":" << K0 << ",\"energy_after\":" << K1
       << ",\"energy_change\":" << K1-K0
       << ",\"vx\":" << vx << ",\"vy\":" << vy << ",\"omega_z\":" << w
       << ",\"point_vn\":" << vy+w << ",\"point_vt\":" << vx+w
       << ",\"normal_impulse\":" << pn << ",\"tangent_impulse\":" << pt << "}";
}
int main(){
    std::cout<<std::setprecision(20)<<"{\n";
    run<float>(10,"float10");std::cout<<",\n";
    run<double>(10,"double10");std::cout<<",\n";
    run<float>(100,"float100");std::cout<<",\n";
    run<double>(100,"double100");std::cout<<"\n}\n";
}
