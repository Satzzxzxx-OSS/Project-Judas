// Isolated measurement of the exact M65 property-dependency helper called by
// active-region pin checks. No solver, scripts or streaming policy is changed.
#include "ScriptSystem.h"
#include <chrono>
#include <iostream>
int main(){
    for(const char* json:{"{}","{\"target\":{\"entity\":\"12\"}}"}){
        std::size_t found=0;for(int i=0;i<3;++i)ScriptSystem::PropertyEntities(json);
        const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<200;++i)found+=ScriptSystem::PropertyEntities(json).size();
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::cout<<"properties "<<json<<" calls 200 refs "<<found<<" total-ms "<<ms<<" per-call-ms "<<ms/200<<'\n';
    }
}
