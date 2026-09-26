#include "core/RunnerPlaceQuery.h"
#include <cassert>
#include <cstdio>
#include <vector>
using Query=downwell::RunnerPlaceQuery;
using Real=Query::Real;
Real number(int n){Real value;assert(value.assign(downwell::MotionBinary64::fromInteger(n)));return value;}
int main(){
    for(auto path:{Query::Path::none,Query::Path::list,Query::Path::spatial,Query::Path::failed})
    for(unsigned dirty=0;dirty<2;++dirty)for(unsigned failure=0;failure<8;++failure){
        Real x=number(1),y=number(2);std::uint32_t flags=dirty?8:0;
        Query::Box cached{number(3),number(4),number(5),number(6)};
        std::vector<unsigned> tape;unsigned moves=0,refreshes=0,preciseCalls=0;
        Query::Result output{true,999};
        const bool success=Query::run(x,y,flags,cached,number(10),number(20),
            [&]{tape.push_back(1);return path;},
            [&]{tape.push_back(2);return failure!=1;},
            [&](bool publish){tape.push_back(publish?3:5);++refreshes;
                if(failure==2)return false;
                cached={x,y,x,y};flags&=~8u;return true;},
            [&](Real nx,Real ny){tape.push_back(++moves==1?4:9);x=nx;y=ny;flags|=8;
                return !(failure==5&&moves==1)&&!(failure==6&&moves==2);},
            [&](auto visit){tape.push_back(6);for(unsigned id:{7,8,9})if(!visit(id))break;return failure!=4;},
            [&](Query::Box box,auto visit){tape.push_back(7);
                assert(box.left.bits==number(10).bits&&box.top.bits==number(20).bits);
                for(unsigned id:{9,8,7})if(!visit(id))break;return failure!=4;},
            [&](unsigned id,bool publish,bool& hit){++preciseCalls;tape.push_back(8);
                assert(publish==(path==Query::Path::list));
                if(failure==3)return false;
                // List collision can recalculate the query's bounds itself.
                if(publish)cached={x,y,x,y};
                hit=id!=8&&failure!=7;return true;},output);
        if(path==Query::Path::none){assert(success&&!output.found&&tape.size()==1);continue;}
        if(path==Query::Path::failed){assert(!success&&output.identity==999&&tape.size()==1);continue;}
        const bool indexed=path==Query::Path::spatial;
        const bool expected=!(failure==3||failure==4||failure==5||failure==6||(indexed&&(failure==1||failure==2)));
        assert(success==expected);
        assert(x.bits==number(1).bits&&y.bits==number(2).bits);
        if(moves){
            assert(moves==2&&tape.back()==9&&(flags&8));
            if(indexed)assert(cached.left.bits==number(dirty?1:3).bits);
        }
        if(success){
            if(failure==7)assert(!output.found&&output.identity==0&&preciseCalls==3);
            else {assert(output.found&&output.identity==(indexed?9u:7u));assert(preciseCalls==1);}
        }
        else assert(output.identity==999);
        if(!indexed){assert(refreshes==0);assert(tape[1]==4&&tape[2]==(failure==5?9u:6u));}
        else {assert(tape[1]==2);if(success){assert(refreshes==1+dirty);assert(tape[2]==(dirty?3u:4u));}}
    }
    std::puts("instance_place orchestration: branch order, publication mode, restoration and failures passed");
}
