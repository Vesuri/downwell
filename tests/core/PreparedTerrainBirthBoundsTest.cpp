#include "core/PreparedTerrainBirthBounds.h"
#include "core/RunnerSpatialJournal.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
static std::vector<unsigned char> read(const char* path){
    std::ifstream f(path,std::ios::binary);assert(f);
    return {std::istreambuf_iterator<char>(f),{}};
}
template<unsigned Nodes,unsigned Pending>
static void journalHistory(const downwell::PreparedTerrainBirthBounds& grid){
    using namespace downwell;
    using Math=RunnerSpatialSplit;
    RunnerSpatialJournal<Nodes,64,Pending> reference;
    RunnerSpatialJournal<Nodes,64,Pending,true> compact;
    assert(compact.bindGeometry(grid)&&!compact.bindGeometry(grid));
    assert(!compact.insertGeometry({0xffffffffu},0,{0,0,1,1})&&compact.valid()&&!compact.pending());
    bool present[64]{};Math::Box previous[64]{};
    unsigned random=47;
    for(unsigned step=0;step<2000;++step){
        random=random*1664525u+1013904223u;
        const unsigned id=(random>>12)&63u;
        bool a,b;
        if(present[id]){
            bool removedA=false,removedB=false;
            a=reference.remove(previous[id],id,true,removedA);
            b=compact.remove(previous[id],id,true,removedB);
            assert(removedA==removedB);if(a)present[id]=false;
        }else{
            const int objects[]={60,93,99,92};PreparedTerrainBirthBounds::Key key;
            assert(grid.key(objects[(random>>20)%4],int((random>>4)&31u)*16-128,
                int((random>>16)&31u)*16,key));
            assert(grid.bounds(key,previous[id],true));
            Math::PixelBox pixels;assert(Math::decodePixels(previous[id],pixels));
            a=reference.insertPrepared(previous[id],id,&pixels);
            b=compact.insertGeometry(key,id,pixels);
            // Operation owns a value key, never this caller's scratch.
            key={};pixels={};present[id]=a;
        }
        assert(a==b&&reference.valid()==compact.valid()&&reference.pending()==compact.pending()&&
            reference.deferred()==compact.deferred());
        if(!a){reference.clear();compact.clear();for(auto& p:present)p=false;continue;}
        if(step%11==0){assert(reference.service(2)==compact.service(2));assert(reference.pending()==compact.pending());}
        if(step%37==0){
            std::vector<unsigned> orderedA,orderedB;
            const auto bounds=Math::encodePixels({-256,-16,768,8192});
            assert(reference.query(bounds,[&](unsigned id){orderedA.push_back(id);return true;}));
            assert(compact.query(bounds,[&](unsigned id){orderedB.push_back(id);return true;}));
            assert(orderedA==orderedB&&reference.height()==compact.height()&&reference.nodeCount()==compact.nodeCount());
        }
    }
    assert(reference.height()==compact.height()&&reference.nodeCount()==compact.nodeCount());
}
static void journalSpans(const downwell::PreparedTerrainBirthBounds& grid){
    using namespace downwell;using Math=RunnerSpatialSplit;
    PreparedTerrainBirthBounds::Key keys[16];Math::PixelBox pixels[16];
    Math::PixelBox envelope{10000,10000,-10000,-10000};
    for(unsigned i=0;i<16;++i){
        bool precise;
        assert(grid.publication(i%2?93:60,int(i)*16,64,keys[i],pixels[i],precise));
        const auto& p=pixels[i];
        if(p.left<envelope.left)envelope.left=p.left;if(p.right>envelope.right)envelope.right=p.right;
        if(p.top<envelope.top)envelope.top=p.top;if(p.bottom>envelope.bottom)envelope.bottom=p.bottom;
    }
    for(bool invalidKey:{false,true})for(unsigned stop=0;stop<=16;++stop){
        RunnerSpatialJournal<512,64,32,true> scalar,batch;
        assert(scalar.bindGeometry(grid)&&batch.bindGeometry(grid));
        unsigned done=999,calls=0;
        bool admitted=false;
        const bool okay=batch.insertGeometrySpan(8,16,envelope,done,admitted,[&](unsigned i,auto& key){
            ++calls;if(i==stop){key={0xffffffffu};return invalidKey;}key=keys[i];return true;
        });
        assert(admitted&&okay==(stop==16)&&done==stop&&calls==(stop==16?16:stop+1));
        for(unsigned i=0;i<stop;++i)assert(scalar.insertGeometry(keys[i],8+i,pixels[i]));
        assert(scalar.pending()==batch.pending()&&batch.deferred()&&batch.valid());
        // Failure must retain the prefix's history, not the whole envelope.
        for(int x=-10000;x<10000;x+=499)for(int y=-5000;y<5000;y+=157)
            assert(scalar.admitsFreshGeometrySpan(32,1,{x,y,x+16,y+16})==
                   batch.admitsFreshGeometrySpan(32,1,{x,y,x+16,y+16}));
        assert(scalar.service(3)&&batch.service(3)&&scalar.pending()==batch.pending());
        std::vector<unsigned> a,b;
        const auto box=Math::encodePixels({-256,-256,768,8192});
        assert(scalar.query(box,[&](unsigned id){a.push_back(id);return true;}));
        assert(batch.query(box,[&](unsigned id){b.push_back(id);return true;}));
        assert(a==b&&scalar.height()==batch.height()&&scalar.nodeCount()==batch.nodeCount());
        calls=0;
        assert(batch.insertGeometrySpan(32,33,envelope,done,admitted,[&](unsigned,auto&){++calls;return true;}));
        assert(!admitted&&!calls&&!done&&batch.valid());
    }
}
int main(){
    using namespace downwell;
    const auto ob=read("generated/downwell-objects.dwob"),sp=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sp.data(),sp.size());
    PreparedObjectDefaults<386> defaults;assert(defaults.prepare(objects,sprites));
    PreparedTerrainBirthBounds grid;assert(grid.prepare(defaults,sprites));
    static_assert(sizeof(grid)<9000,"Terrain table must share coordinate offsets");
    journalHistory<512,8>(grid);
    journalHistory<16,8>(grid);
    journalSpans(grid);
    unsigned checks=0;
    for(int object:{60,93,99,92}){
        const auto* d=defaults.get(object);assert(d);
        PackedSpriteCollision resource;assert(sprites.collisionSprite(d->spriteId,d->maskId,resource));
        for(int y=0;y<8192;y+=16)for(int x=-256;x<768;x+=16){
            RunnerImageValue px,py,rx,ry;RunnerSpatialSplit::Box result,reference;bool precise=false;
            RunnerSpatialSplit::PixelBox pixels,decoded;
            rx.assignInteger(x);ry.assignInteger(y);
            assert(grid.at(object,x,y,px,py,result,precise,&pixels));
            PreparedTerrainBirthBounds::Key key;
            RunnerSpatialSplit::Box keyed;
            assert(grid.key(object,x,y,key)&&grid.bounds(key,keyed));
            RunnerSpatialSplit::PixelBox direct;
            assert(grid.pixels(key,direct));
            assert(direct.left==pixels.left&&direct.right==pixels.right&&
                direct.top==pixels.top&&direct.bottom==pixels.bottom);
            assert(keyed.left.bits==result.left.bits&&keyed.top.bits==result.top.bits&&
                keyed.right.bits==result.right.bits&&keyed.bottom.bits==result.bottom.bits);
            assert(grid.bounds(key,keyed,true));
            RunnerSpatialSplit::PixelBox keyedPixels;
            assert(RunnerSpatialSplit::decodePixels(keyed,keyedPixels));
            assert(keyedPixels.left==(pixels.left<pixels.right?pixels.left:pixels.right)&&
                keyedPixels.right==(pixels.left<pixels.right?pixels.right:pixels.left)&&
                keyedPixels.top==(pixels.top<pixels.bottom?pixels.top:pixels.bottom)&&
                keyedPixels.bottom==(pixels.top<pixels.bottom?pixels.bottom:pixels.top));
            assert(px.bits==rx.bits&&py.bits==ry.bits&&precise==(resource.collisionMode==1));
            assert(RunnerSpriteBounds::fromSpriteBinary32(resource,rx,ry,{0x3f800000u},
                {0x3f800000u},false,{},{},true,[](auto v,std::int32_t& n){return v.roundPixel(n);},reference));
            assert(result.left.bits==reference.left.bits&&result.right.bits==reference.right.bits&&
                result.top.bits==reference.top.bits&&result.bottom.bits==reference.bottom.bits);
            assert(RunnerSpatialSplit::decodePixels(reference,decoded));
            assert(pixels.left==decoded.left&&pixels.top==decoded.top&&
                pixels.right==decoded.right&&pixels.bottom==decoded.bottom);
            PreparedTerrainBirthBounds::Key publication;
            RunnerSpatialSplit::PixelBox published;bool publishedPrecise=false;
            assert(grid.publication(object,x,y,publication,published,publishedPrecise));
            assert(publishedPrecise==precise);
            if(decoded.right<decoded.left)std::swap(decoded.right,decoded.left);
            if(decoded.bottom<decoded.top)std::swap(decoded.bottom,decoded.top);
            assert(published.left==decoded.left&&published.right==decoded.right&&
                published.top==decoded.top&&published.bottom==decoded.bottom);
            RunnerSpatialSplit::Box publishedBox;assert(grid.bounds(publication,publishedBox,true));
            RunnerSpatialSplit::PixelBox publishedDecoded;
            assert(RunnerSpatialSplit::decodePixels(publishedBox,publishedDecoded));
            assert(published.left==publishedDecoded.left&&published.right==publishedDecoded.right&&
                published.top==publishedDecoded.top&&published.bottom==publishedDecoded.bottom);
            RunnerSpatialSplit::PixelBox dictionary;
            assert(grid.atReference(object,x,y,px,py,result,precise,&dictionary));
            assert(pixels.left==dictionary.left&&pixels.top==dictionary.top&&
                pixels.right==dictionary.right&&pixels.bottom==dictionary.bottom);
            ++checks;
        }
    }
    for(int object:{-1,35,60,93,99,92,386})for(int x:{-272,-256,-255,0,1,752,768})
        for(int y:{-16,0,1,8176,8192}){
            RunnerImageValue px{123},py{456};RunnerSpatialSplit::Box box{{1},{2},{3},{4}};bool precise=true;
            RunnerSpatialSplit::PixelBox pixels{11,22,33,44};
            const bool expected=(object==60||object==93||object==99||object==92)&&x>=-256&&x<768&&y>=0&&y<8192&&
                !(unsigned(x)&15u)&&!(unsigned(y)&15u);
            assert(grid.at(object,x,y,px,py,box,precise,&pixels)==expected);
            PreparedTerrainBirthBounds::Key key{0xffffffffu};
            assert(grid.key(object,x,y,key)==expected);
            if(!expected)assert(key.value==0xffffffffu);
            if(!expected)assert(px.bits==123&&py.bits==456&&box.left.bits==1&&box.top.bits==2&&
                box.right.bits==3&&box.bottom.bits==4&&precise);
            if(!expected)assert(pixels.left==11&&pixels.top==22&&pixels.right==33&&pixels.bottom==44);
        }
    // A resource with too many distinct offsets declines preparation without
    // consuming dictionary slots or preventing later shapes from preparing.
    auto altered=sp;
    const unsigned resource=SpriteCollisionPack::headerSize+585*SpriteCollisionPack::recordSize;
    altered[resource+8]=1;altered[resource+12]=2;
    altered[resource+16]=13;altered[resource+20]=14;
    SpriteCollisionPack unusual(altered.data(),altered.size());assert(unusual.valid());
    assert(grid.prepare(defaults,unusual));
    RunnerImageValue a,b;RunnerSpatialSplit::Box box;bool precise;
    assert(!grid.at(60,240,176,a,b,box,precise));
    assert(grid.at(93,240,176,a,b,box,precise)&&grid.at(99,240,176,a,b,box,precise));
    assert(grid.prepare(defaults,sprites));
    const auto copied=grid;
    for(unsigned invalid:{0u,1u,2u,3u,4095u,135168u,0xffffffffu}){
        RunnerSpatialSplit::Box untouched{{1},{2},{3},{4}};
        assert(!copied.bounds({invalid},untouched));
        assert(untouched.left.bits==1&&untouched.top.bits==2&&untouched.right.bits==3&&untouched.bottom.bits==4);
    }
    assert(!grid.prepare(defaults,SpriteCollisionPack(nullptr,0)));
    assert(copied.at(60,240,176,a,b,box,precise));
    RunnerSpatialSplit::PixelBox copiedPixels,referencePixels;
    assert(copied.at(60,240,176,a,b,box,precise,&copiedPixels)&&
        copied.atReference(60,240,176,a,b,box,precise,&referencePixels));
    assert(copiedPixels.left==referencePixels.left&&copiedPixels.top==referencePixels.top&&
        copiedPixels.right==referencePixels.right&&copiedPixels.bottom==referencePixels.bottom);
    RunnerImageValue x,y;RunnerSpatialSplit::Box missing;bool p;
    assert(!grid.at(60,0,0,x,y,missing,p));
    std::printf("Prepared terrain birth: %u binary32 oracle comparisons, bounds and invalidation passed; storage %zu bytes\n",checks,sizeof(grid));
}
