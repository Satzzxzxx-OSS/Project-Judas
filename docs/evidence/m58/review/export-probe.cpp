// Uses the ordinary project export function, without creating a second package path.
#include "ProjectExporter.h"
#include <cstdio>
int main(int argc,char** argv){if(argc!=5)return 2;Project p;std::string error;ProjectExportResult r;ProjectExportOptions o;o.destination=argv[2];o.runtimeExecutable=argv[3];o.engineDataRoot=argv[4];if(!p.Load(argv[1],error)||!ExportProject(p,o,r,error)){std::fprintf(stderr,"FAIL %s\n",error.c_str());return 1;}std::printf("PASS package=%s assets=%zu scenes=%zu bytes=%llu seconds=%.6f\n",r.packageDirectory.c_str(),r.assetCount,r.sceneCount,(unsigned long long)r.bytes,r.seconds);return 0;}
