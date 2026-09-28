// Independent scalar extraction of source-normal impulse and orientation rules.
// NOT compiled against PhysicsWorld/GLM and NOT a report of current-engine execution.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
int main(){
 std::cout<<std::setprecision(17);
 for(int loops : {1,10,100,1000}) {
  double v[3]={1.,-3.,4.},m[3]={10.,.125,10.},lambda[2]={0,0};
  const double target[2]={4,0};
  const auto energy=[&](){double k=0;for(int i=0;i<3;++i)k+=m[i]*v[i]*v[i]*.5;return k;};
  const double before=energy();
  for(int k=0;k<loops;++k)for(int i=0;i<2;++i){
   // Same accumulated normal PGS update/sign-equivalent to the source.
   const double relative=v[i+1]-v[i];
   const double normalMass=1./(1./m[i]+1./m[i+1]);
   const double newNormal=std::max(lambda[i]+normalMass*(target[i]-relative),0.);
   const double increment=newNormal-lambda[i];lambda[i]=newNormal;
   v[i]-=increment/m[i];v[i+1]+=increment/m[i+1];
  }
  std::cout<<"normal_pgs,"<<loops<<","<<v[0]<<","<<v[1]<<","<<v[2]<<","<<before<<","<<energy()<<","<<energy()-before<<"\n";
 }
 for(double w:{10.,100.,1000.}){
  const double dt=1./60;
  std::cout<<"normalized_euler,"<<w<<","<<2*std::atan(w*dt/2)<<","<<4*std::atan(w*dt/4)<<","<<w*dt<<"\n";
 }
}
