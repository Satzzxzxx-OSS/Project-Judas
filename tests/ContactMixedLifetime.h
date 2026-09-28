#pragma once
#include "ContactAllocationInstrumentation.h"
namespace allocation_evidence {
template<class Stats>struct MixedMeter {
 const char* name;Point begin{},setupEnd{},stepBegin{},teardownBegin{};Span steps[120]{};Stats statistics[120]{};
 explicit MixedMeter(const char*n):name(n){}
 void built(){setupEnd=Point{};}void start(){stepBegin=Point{};}
 void finish(int i,const Stats&s){steps[i]=between(stepBegin,Point{});statistics[i]=s;}
 void teardown(){teardownBegin=Point{};}
 ~MixedMeter(){const Point end{};const auto whole=between(begin,end),setup=between(begin,setupEnd),destroy=between(teardownBegin,end);
  double simulation=0;for(const auto&s:steps)simulation+=s.ms;
  std::printf("{\"kind\":\"mixed_partition\",\"fixture\":\"%s\",",name);print("whole",whole);std::printf(",");print("construction",setup);std::printf(",");print("teardown",destroy);
  std::printf(",\"all_steps_ms\":%.12g,\"other_instrumentation_ms\":%.12g,\"steps\":[",simulation,whole.ms-setup.ms-destroy.ms-simulation);
  for(int i=0;i<120;++i){std::printf("%s{\"index\":%d,",i?",":"",i);print("outer",steps[i]);std::printf(",");Cache<Stats>::print(statistics[i]);Bytes<Stats>::print(statistics[i]);std::printf("}");}std::printf("]}\n");
 }
};
}
