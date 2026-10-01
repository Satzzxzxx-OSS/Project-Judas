// Export actual defaults for authored projects; optionally time the same core.
#include "InputSystem.h"
#include <iostream>
#include <chrono>
int main(int argc,char**){
    if(argc==1){std::cout<<InputMap::Defaults().Serialize();return 0;}
    InputSystem input;float checksum=0;const auto start=std::chrono::steady_clock::now();
    for(int i=0;i<10000;++i){input.BeginFrame();input.SetPhysical("key:Space",float(i&1));checksum+=input.Action("jump").held;checksum+=input.Axis("look_x");}
    std::size_t bindings=0;for(const auto& e:input.Map().entries)bindings+=e.bindings.size();
    std::cout<<"normal_project_entries="<<input.Map().entries.size()<<" bindings="<<bindings<<" update_query_mean_us="<<std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/10000<<" checksum="<<checksum<<'\n';
}
