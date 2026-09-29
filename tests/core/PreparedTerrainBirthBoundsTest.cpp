#include "core/PreparedTerrainBirthBounds.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
static std::vector<unsigned char> read(const char* path){
    std::ifstream f(path,std::ios::binary);assert(f);
    return {std::istreambuf_iterator<char>(f),{}};
}
int main(){
    using namespace downwell;
    const auto ob=read("generated/downwell-objects.dwob"),sp=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sp.data(),sp.size());
    PreparedObjectDefaults<386> defaults;assert(defaults.prepare(objects,sprites));
    PreparedTerrainBirthBounds grid;assert(grid.prepare(defaults,sprites));
    unsigned checks=0;
    for(int object:{60,93,99}){
        const auto* d=defaults.get(object);assert(d);
        PackedSpriteCollision resource;assert(sprites.collisionSprite(d->spriteId,d->maskId,resource));
        for(int y=0;y<8192;y+=16)for(int x=-256;x<768;x+=16){
            RunnerImageValue px,py,rx,ry;RunnerSpatialSplit::Box result,reference;bool precise=false;
            rx.assignInteger(x);ry.assignInteger(y);
            assert(grid.at(object,x,y,px,py,result,precise));
            assert(px.bits==rx.bits&&py.bits==ry.bits&&precise==(resource.collisionMode==1));
            assert(RunnerSpriteBounds::fromSpriteBinary32(resource,rx,ry,{0x3f800000u},
                {0x3f800000u},false,{},{},true,[](auto v,std::int32_t& n){return v.roundPixel(n);},reference));
            assert(result.left.bits==reference.left.bits&&result.right.bits==reference.right.bits&&
                result.top.bits==reference.top.bits&&result.bottom.bits==reference.bottom.bits);
            ++checks;
        }
    }
    for(int object:{-1,35,60,93,99,386})for(int x:{-272,-256,-255,0,1,752,768})
        for(int y:{-16,0,1,8176,8192}){
            RunnerImageValue px{123},py{456};RunnerSpatialSplit::Box box{{1},{2},{3},{4}};bool precise=true;
            const bool expected=(object==60||object==93||object==99)&&x>=-256&&x<768&&y>=0&&y<8192&&
                !(unsigned(x)&15u)&&!(unsigned(y)&15u);
            assert(grid.at(object,x,y,px,py,box,precise)==expected);
            if(!expected)assert(px.bits==123&&py.bits==456&&box.left.bits==1&&box.top.bits==2&&
                box.right.bits==3&&box.bottom.bits==4&&precise);
        }
    assert(!grid.prepare(defaults,SpriteCollisionPack(nullptr,0)));
    RunnerImageValue x,y;RunnerSpatialSplit::Box b;bool p;
    assert(!grid.at(60,0,0,x,y,b,p));
    std::printf("Prepared terrain birth: %u binary32 oracle comparisons, bounds and invalidation passed; storage %zu bytes\n",checks,sizeof(grid));
}
