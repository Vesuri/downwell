// Reuse the source-backed room/owner replay fixture, not its main test driver.
#define main scene_runtime_test_main
#include "AmigaSceneRuntimeTest.cpp"
#undef main
#include "platform/amiga/AmigaGameplayDrawList.h"
#include "core/CurrencySmokeComposition.h"
#include "core/DebrisDraw.h"
#include <cstring>

int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}

int main(int argc,char** argv){
    bool census=false,overflowTape=false;
    for(int i=1;i<argc;++i){
        if(std::strcmp(argv[i],"--census")==0&&!census)census=true;
        else if(std::strcmp(argv[i],"--overflow")==0&&!overflowTape)overflowTape=true;
        else require(false,"usage: amiga-currency-promotion-test [--census] [--overflow]");
    }
    const auto rb=readFile("generated/downwell-rooms.dwrm"),ob=readFile("generated/downwell-objects.dwob"),
        pb=readFile("generated/downwell-builder-patterns.dwbp"),ab=readFile("generated/amiga-game.dwap");
    downwell::RoomPack rooms(rb.data(),rb.size());downwell::ObjectPack objects(ob.data(),ob.size());
    downwell::BuilderPatternPack patterns(pb.data(),pb.size());downwell::PackedRoom room;
    require(rooms.findRoom(11,room),"main room");
    downwell::StartupRuntime host(downwell::PlatformProfile::nonDesktop,&rooms,&objects,overflowTape?8:2,&patterns);
    host.skipToGameForTesting();downwell::GameplayTerrainInstances terrain;
    AmigaPortRuntime game(300,false,1201-176,true,{},{},{},&terrain);game.initializeEntryInput(false);
    LiveSceneWorld live(patterns,objects);live.bind(game,rooms,room,false,&host);
    downwell::EffectMaskAssets assets;require(assets.bind(ab.data(),ab.size()),"effect assets");
    AmigaActorFrames frames;require(frames.prepare(ab.data(),ab.data()+ab.size()),"hardware frames");
    const auto word=[](const UBYTE* p){return unsigned(p[0])*256+p[1];};
    const auto signedWord=[&](const UBYTE* p){const unsigned n=word(p);return n<32768?int(n):int(n)-65536;};
    // Inspect the actual offline DMA payload, including origin/crop metadata.
    // Pen zero is transparent in hardware; opaque black must remain pen three.
    for(unsigned large=0;large<2;++large)for(unsigned frame=0;frame<4;++frame){
        const auto& art=*assets.currency(large,frame);
        const auto* prepared=frames.find(large?134:133,frame);
        require(prepared&&prepared->partCount[0],"currency DMA parts");
        for(int y=-32;y<32;++y)for(int x=-32;x<32;++x){
            unsigned expected=0,actual=0;
            const int ax=x+art.originX,ay=y+art.originY;
            if(ax>=0&&ay>=0&&unsigned(ax)<art.mask.width&&unsigned(ay)<art.mask.height){
                const unsigned at=ay*art.mask.stride+ax/8,bit=128u>>(ax&7);
                const bool opaque=art.mask.data[at]&bit;
                expected=unsigned(bool(art.low[at]&bit))+2*unsigned(bool(art.high[at]&bit));
                require(opaque==bool(expected),"DMA opacity preserves black");
            }
            for(unsigned p=0;p<prepared->partCount[0];++p){
                const auto* r=prepared->parts[0][p];
                const int dx=x+signedWord(r+4),dy=y+signedWord(r+6);
                if(dx<0||dx>=16||dy<0||unsigned(dy)>=word(r+10))continue;
                const unsigned offset=word(r+16)*65536u+word(r+18);
                const auto* row=ab.data()+offset+dy*4;
                const unsigned pen=unsigned(bool(word(row)&(32768u>>dx)))+
                    2*unsigned(bool(word(row+2)&(32768u>>dx)));
                if(pen)actual=pen;
            }
            require(actual==expected,"offline currency DMA pixels and anchors");
        }
    }
    AmigaGameplayDrawList list;downwell::CurrencySmokeComposition oracle;
    downwell::PaletteEffectSurface<> projected;unsigned promoted=0;
    unsigned empty=0,raster=0,mixed=0,overflow=0,covered=0,shaded=0,maxSmoke=0,maxDebris=0;
    if(census)std::puts("frame,visible_gems,smoke,debris,entry,promoted,parts,shade_pixels_changed,dead");
    for(unsigned tick=1;tick<=1800;++tick){
        auto input=downwell::gameplayContactProfileInput(game.tick(),240+game.player().offsetX(),
            176+game.player().offsetY(),downwell::gameplayContactProfilePhase);
        if(overflowTape)require(downwell::gameplayEncounterProfileInput(game.tick(),240+game.player().offsetX(),
            176+game.player().offsetY(),4,input),"ordinary seed-8 route-4 inputs");
        game.step(downwell::packReplayInput(input));host.step(input,false);parityFrame=tick-1;
        compareInitialGameplay(game,host);
        require(game.effectsSucceeded()&&game.generationSucceeded(),"runtime");
        const int vx=160+game.cameraOffsetX(),vy=34+game.cameraOffsetY();
        require(list.collect(game,ab.data(),ab.data()+ab.size(),vx,vy,true,&frames),"native draw-list census");
        const unsigned firstCurrency=list.count;
        unsigned visible=0,parts=0,debris=0;
        for(unsigned i=0;i<game.gemSlotEnd();++i){const auto& g=game.gemAt(i);
            if(!g.active||!g.motion.lifetime.visible()||
                !downwell::EffectSurfacePlacement::pixelInView(240+g.x,176+g.y,{vx,vy,160,284}))continue;
            ++visible;const auto* art=frames.find(g.large?134:133,g.motion.animation.frame());
            require(art,"census currency frame");parts+=art->partCount[0];
        }
        require(game.visitDebrisDraw([&](const auto&){++debris;return true;}),"debris census");
        const unsigned smoke=game.smokeEffects().size();
        if(smoke>maxSmoke)maxSmoke=smoke;if(debris>maxDebris)maxDebris=debris;
        const auto checkpoint=game.checkpoint();
        const bool hardware=list.promoteCurrency(game,vx,vy,assets,frames);
        require(game.checkpoint()==checkpoint,"promotion is read-only, including fallback");
        using Real=downwell::MotionBinary64;
        const downwell::EffectSurfacePlacement::View view{Real::fromInteger(vx),Real::fromInteger(vy),
            Real::fromInteger(160),Real::fromInteger(284)};
        downwell::CurrencySmokeComposition::Settings settings;settings.noControl=game.shops().noControl;
        require(oracle.prepareWithLateEffects([&](auto emit){
            for(unsigned i=game.gemSlotEnd();i;--i){const auto& g=game.gemAt(i-1);
                if(g.active&&g.motion.lifetime.visible())emit(g.large,g.motion.animation.frame(),
                    std::int32_t(240+g.x),std::int32_t(176+g.y));}
        },game.smokeEffects(),assets,view,settings,[&](auto emit){return game.visitDebrisDraw([&](const auto& d){
            return downwell::DebrisDraw::surface(d,assets,view,emit);});
        },[&](auto emit){for(unsigned i=0;i<game.enemyCount();++i){const auto& e=game.enemyAt(i);
            if(e.active&&e.awake)emit(std::int32_t(240+e.x),std::int32_t(176+e.y));}
        },[](Real n,std::int32_t& out){return n.roundPixel(out);}),"compositor oracle");
        // Independently compose the currency before shade subtraction. Only
        // classify changed pixels on gem-only, live frames; rejection alone
        // does not establish that a shade actually erases visible content.
        projected.clear();
        for(unsigned i=game.gemSlotEnd();i;--i){const auto& g=game.gemAt(i-1);
            if(!g.active||!g.motion.lifetime.visible()||
                !downwell::EffectSurfacePlacement::pixelInView(240+g.x,176+g.y,{vx,vy,160,284}))continue;
            const auto& a=*assets.currency(g.large,g.motion.animation.frame());
            projected.add({{a.mask.data,a.mask.bytes,a.mask.width,a.mask.height,a.mask.stride},a.low,a.high},
                240+g.x-vx-a.originX,176+g.y-vy-a.originY);
        }
        const bool shadeChanged=!game.dead()&&!smoke&&!debris&&!list.entryCoverVisible&&
            !projected.samePixels(oracle.surface());
        if(shadeChanged){++shaded;require(!hardware,"shade-subtracted currency stays raster");}
        if(census)std::printf("%u,%u,%u,%u,%u,%u,%u,%u,%u\n",tick,visible,smoke,debris,
            unsigned(list.entryCoverVisible),unsigned(hardware),parts,unsigned(shadeChanged),unsigned(game.dead()));
        if(!hardware){
            require(list.count==firstCurrency,"fallback preserves the existing actor prefix");
            if(!visible&&!smoke&&!debris&&!list.entryCoverVisible)++empty;
            else ++raster;
            if(smoke||debris)++mixed;
            if(parts>8)++overflow;
            if(list.entryCoverVisible)++covered;
            continue;
        }
        ++promoted;
        projected.clear();unsigned channels=0;
        for(unsigned i=firstCurrency;i<list.count;++i){const auto& bob=list.actors[i];
            const auto& a=*assets.currency(bob.sprite==134,bob.frame);channels+=bob.prepared->partCount[0];
            projected.add({{a.mask.data,a.mask.bytes,a.mask.width,a.mask.height,a.mask.stride},a.low,a.high},
                bob.x-downwellPlayfieldX-a.originX,bob.y-a.originY);
        }
        require(channels<=8&&projected.samePixels(oracle.surface()),"promoted final alpha and colours");
        list.count=AmigaGameplayDrawList::capacity;
        require(!list.promoteCurrency(game,vx,vy,assets,frames)&&list.count==AmigaGameplayDrawList::capacity,
            "full draw list fallback");
        list.count=0;list.entryCoverVisible=true;
        require(!list.promoteCurrency(game,vx,vy,assets,frames)&&!list.count,"entry-cover fallback");
    }
    if(overflowTape){
        require(overflow>0,"natural seed-8 route-4 tape exercises sprite-channel overflow");
        require(shaded>0,"natural tape exercises actual shade subtraction");
    }
    else require(promoted>50,"contact tape exercises promotion");
    std::printf("Currency promotion: %u exact frames in 1800-frame source-backed replay\n",promoted);
    require(promoted+empty+raster==1800,"every frame has a presentation class");
    std::printf("Effect coverage: hardware=%u empty=%u raster=%u; fallback mixed=%u overflow=%u entry=%u shade=%u; max live smoke=%u debris=%u\n",
        promoted,empty,raster,mixed,overflow,covered,shaded,maxSmoke,maxDebris);
}
