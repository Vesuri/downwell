#include "core/StartupRuntime.h"
#include "core/RoomPack.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>
namespace {
void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
}
int main(int argc,char** argv){
    require(argc==2,"supply original generated room pack");
    std::ifstream input(argv[1],std::ios::binary);require(input.good(),"room pack opens");
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    downwell::RoomPack pack(bytes.data(),bytes.size());require(pack.valid(),"valid room pack");
    for(auto profile:{downwell::PlatformProfile::desktop,downwell::PlatformProfile::nonDesktop}){
        auto runtime=std::make_unique<downwell::StartupRuntime>(profile,&pack);
        require(runtime->roomLoadSucceeded(),"live initial room loads");
        require(runtime->sourceLayerIds().loadedMaximum==131&&runtime->sourceLayerIds().dynamicMaximum==0,
                "whole catalog seeded before initial room");
        runtime->reset(profile);
        require(runtime->roomLoadSucceeded()&&runtime->sourceLayerIds().loadedMaximum==131&&
                runtime->sourceLayerIds().dynamicMaximum==0,"reset preserves catalog identity floor");
    }
    downwell::RoomPack invalid(nullptr,0);
    auto failed=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::desktop,&invalid);
    require(!failed->roomLoadSucceeded()&&!failed->effectsSucceeded(),"invalid catalog stops startup loudly in state");
    std::cout<<"Live startup catalog identity seeding verified for both platform profiles\n";
}
