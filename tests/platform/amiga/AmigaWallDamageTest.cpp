#include "platform/amiga/AmigaWallDamage.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    // Compare strip rejection with independent wide-coordinate rectangles,
    // including the outer eight rows of the 32-pixel Limbo artwork.
    for(int sprite:{51,52,53,55,57,70,72,74,76}){
        const long long radius=sprite==70?16:8;
        for(int y=-40;y<=40;++y)for(int first=-50;first<=50;++first)
            for(int height=0;height<=32;++height){
                const bool expected=height>0&&y+radius>first&&y-radius<first+height;
                assert(downwell::wallIntersectsRows(sprite,y,first,first+height)==expected);
            }
        for(int y:{(-2147483647-1),-2147483640,2147483640,2147483647})
            for(int first:{(-2147483647-1),-2147483640,0,2147483630}){
                const int end=first+16;
                assert(downwell::wallIntersectsRows(sprite,y,first,end)==
                    (static_cast<long long>(y)+radius>first&&static_cast<long long>(y)-radius<end));
            }
    }
    assert(!downwell::wallIntersectsRows(-1,0,-1,1));
    assert(downwell::wallIntersectsRows(999,0,100,101));
    AmigaWallDamage<64> damage;bool expected[512]{};
    unsigned random=123;
    for(unsigned i=0;i<4000;++i){
        random=random*1664525u+1013904223u;const int first=int(random%496u);
        const int end=first+int((random>>24)%16u)+1;
        assert(damage.mark(first,end));for(int y=first;y<end;++y)expected[y]=true;
        bool actual[512]{};int previous=-1;
        assert(damage.visit([&](int a,int b){assert(a>previous&&a<b);previous=b;
            for(int y=a;y<b;++y)actual[y]=true;return true;}));
        for(unsigned y=0;y<512;++y)assert(actual[y]==expected[y]);
        if(i%31==30){damage.clear();for(auto& bit:expected)bit=false;}
    }
    damage.clear();assert(damage.changed(0,32,51,70)&&damage.mark(100,116));
    unsigned calls=0;assert(!damage.repair([&](int a,int b){++calls;
        if(calls==1){assert(a==16&&b==48);return true;}assert(a==100&&b==116);return false;}));
    assert(damage.size()==1&&damage.repair([](int a,int b){return a==100&&b==116;}));
    assert(damage.size()==0&&damage.changed(0,0,-1,-1)&&damage.size()==0);
    assert(!damage.changed(0,0,999,-1)&&!damage.valid());
    damage.clear();assert(!damage.changed(0,2147483647,70,-1));
    AmigaWallDamage<1> limited;assert(limited.mark(0,16)&&!limited.mark(32,48)&&limited.size()==1);
    std::puts("Wall row damage: 4000 sparse-union checks, footprints, partial repair and failures passed");
}
