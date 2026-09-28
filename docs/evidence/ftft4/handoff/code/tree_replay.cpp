// Replays exact dyadic proxy boxes against Judas's actual DynamicAabbTree.
// NOT executed by ChatGPT: requires the operator's normal GLM headers.
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include "Broadphase.h"
int main(int argc,char**argv){
    if(argc!=2){std::cerr<<"usage: tree_replay replay.tsv\n";return 2;}
    std::ifstream f(argv[1]); if(!f) return 2;
    DynamicAabbTree tree; std::map<unsigned,int> ids; std::string line;
    unsigned checks=0,failed=0;
    while(std::getline(f,line)){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream s(line); char op; s>>op;
        if(op=='D') {unsigned id;s>>id;tree.DestroyProxy(ids.at(id));ids.erase(id);}
        else {
            unsigned id=0;if(op!='Q')s>>id;
            Aabb b;s>>b.min.x>>b.min.y>>b.min.z>>b.max.x>>b.max.y>>b.max.z;
            if(op=='C') ids[id]=tree.CreateProxy(b,id);
            else if(op=='M')tree.MoveProxy(ids.at(id),b);
            else if(op=='Q'){
                size_t n;s>>n;std::set<unsigned> expected,actual;
                for(size_t i=0;i<n;++i){unsigned x;s>>x;expected.insert(x);}
                tree.Query(b,[&](int p){actual.insert(tree.UserData(p));});
                ++checks;if(actual!=expected){++failed;std::cerr<<"query mismatch "<<checks<<'\n';}
            } else return 2;
        }
        if(!tree.Validate()){++failed;std::cerr<<"tree invalid\n";}
    }
    std::cout<<"queries="<<checks<<" failures="<<failed<<'\n';return failed?1:0;
}
