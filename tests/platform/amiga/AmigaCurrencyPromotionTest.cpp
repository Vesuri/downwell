// Reuse the source-backed room/owner replay fixture, not its main test driver.
#define main scene_runtime_test_main
#include "AmigaSceneRuntimeTest.cpp"
#undef main
#include "platform/amiga/AmigaGameplayDrawList.h"
#include "core/CurrencySmokeComposition.h"
#include "core/DebrisDraw.h"
#include "platform/amiga/AmigaOverflowProfileEntry.h"
#include "platform/amiga/AmigaOverflowWitness.h"
#include <cstring>

int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}

// Count publication boundaries on the actual replay-selected programs, without
// changing execution or consuming RNG. Groups cross row ends, never event cells.
// This is an upper bound on callback elimination, NOT a performance result or
// permission to merge failure prefixes. Pillar rows retain their builder-y
// publication boundaries.
static void generationCensus(unsigned frame,const downwell::BuilderPatternPack& patterns,
    const AmigaTerrainFragment& fragment){
    unsigned segments=0,groups=0,events=0,draws=0,cells=0,words=0,open=0,longest=0;
    auto barrier=[&]{if(open>longest)longest=open;open=0;};
    auto row=[&](const std::uint8_t* p){
        require(p!=nullptr,"generation census prepared row");
        while(*p!=255){
            if(*p){++events;barrier();p+=2;}
            else{
                if(!open)++groups;
                ++open;++segments;draws+=p[1];cells+=p[3];words+=p[4];
                p+=6+unsigned(p[3])*2+unsigned(p[4])*4;
            }
        }
    };
    for(unsigned y=0;y<fragment.pattern.rowCount;++y)
        row(patterns.preparedRow(fragment.pattern,y,fragment.reversed));
    barrier();
    if(fragment.appendPillars)for(unsigned y=0;y<3;++y){
        row(patterns.preparedPillars(fragment.reversed));barrier();
    }
    std::printf("GENERATION frame=%u family=%u pattern=%u y=%d reversed=%u segments=%u groups=%u events=%u longest=%u discarded_draws=%u cells=%u words=%u\n",
        frame,unsigned(fragment.pattern.family),fragment.pattern.sourceIndex,fragment.y,
        unsigned(fragment.reversed),segments,groups,events,longest,draws,cells,words);
}

int main(int argc,char** argv){
    bool census=false,overflowTape=false,witness=false,generation=false,pixelWitness=false;
    for(int i=1;i<argc;++i){
        if(std::strcmp(argv[i],"--census")==0&&!census)census=true;
        else if(std::strcmp(argv[i],"--overflow")==0&&!overflowTape)overflowTape=true;
        else if(std::strcmp(argv[i],"--witness")==0&&!witness)witness=true;
        else if(std::strcmp(argv[i],"--generation-census")==0&&!generation)generation=true;
        else if(std::strcmp(argv[i],"--pixel-witness")==0&&!pixelWitness)pixelWitness=true;
        else require(false,"usage: amiga-currency-promotion-test [--census] [--overflow] [--witness] [--generation-census] [--pixel-witness]");
    }
    const auto rb=readFile("generated/downwell-rooms.dwrm"),ob=readFile("generated/downwell-objects.dwob"),
        pb=readFile("generated/downwell-builder-patterns.dwbp"),ab=readFile("generated/amiga-game.dwap");
    downwell::RoomPack rooms(rb.data(),rb.size());downwell::ObjectPack objects(ob.data(),ob.size());
    downwell::BuilderPatternPack patterns(pb.data(),pb.size());downwell::PackedRoom room;
    require(rooms.findRoom(11,room),"main room");
    downwell::StartupRuntime host(downwell::PlatformProfile::nonDesktop,&rooms,&objects,overflowTape?8:2,&patterns);
    host.skipToGameForTesting();downwell::GameplayTerrainInstances terrain;
    if(overflowTape){
        AmigaAreaOneTerrainBuilder entry(nullptr,0);downwell::GameMakerRandom random;
        require(amigaOverflowProfileEntry(entry,random),"overflow native checkpoint");
        random.nextU32(); // Ordinary entrance Create, not an extra gameplay draw.
        require(random.drawCount()==host.randomDrawCount()&&
            random.stateIndex()==host.randomState().stateIndex(),"overflow entry RNG position");
        for(unsigned i=0;i<16;++i)require(random.stateWord(i)==host.randomState().stateWord(i),
            "overflow entry complete source RNG state");
        const auto& b=host.mainBuilderState();
        require(b.y()==704&&b.generationCount()==-30&&b.rightRoomCount()==2&&
            b.rightRoom(0)==13&&b.rightRoom(1)==4&&!b.rowsGenerated()&&!b.spawnerMade()&&!b.endReached(),
            "overflow entry source builder checkpoint");
    }
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
        // The same offline artwork must support the ordinary BOB route.
        // Compare against effect alpha/colour data, not another asset draw.
        // Nonzero backgrounds expose lost opaque black; the last ring row
        // and viewport corners exercise clipping and circular publication.
        for(unsigned origin:{0u,319u})for(unsigned corner=0;corner<3;++corner){
            std::vector<UBYTE> bitmap(downwellGameRingBytes,0x55),hud(downwellLayerBytes);
            AmigaRenderer renderer;amigaRendererInit(&renderer,bitmap.data(),hud.data());
            amigaRendererSetGameOrigin(&renderer,origin);
            amigaRendererSetClipRect(&renderer,downwellPlayfieldX,0,160,284);
            const int cx=corner==0?0:corner==1?79:159,cy=corner==0?0:corner==1?141:283;
            amigaRendererDrawAsset(&renderer,ab.data(),ab.data()+ab.size(),large?134:133,frame,
                downwellPlayfieldX+cx,cy,0);
            for(int y=0;y<284;++y)for(int x=0;x<384;++x){
                unsigned expected=(x&1)?3:0;
                const int ax=x-downwellPlayfieldX-cx+art.originX,ay=y-cy+art.originY;
                if(x>=downwellPlayfieldX&&x<downwellPlayfieldX+160&&ax>=0&&ay>=0&&
                    unsigned(ax)<art.mask.width&&unsigned(ay)<art.mask.height){
                    const unsigned at=ay*art.mask.stride+ax/8,bit=128u>>(ax&7);
                    if(art.mask.data[at]&bit)expected=unsigned(bool(art.low[at]&bit))+2*unsigned(bool(art.high[at]&bit));
                }
                const unsigned at=((origin+y)%downwellGameRingHeight)*downwellBytesPerPlaneLine+x/8,bit=128u>>(x&7);
                const unsigned actual=unsigned(bool(bitmap[at]&bit))+
                    2*unsigned(bool(bitmap[at+downwellGameRingPlaneBytes]&bit));
                require(actual==expected,"currency BOB colours/alpha, clipped ring placement and untouched margins");
            }
        }
    }
    AmigaGameplayDrawList list;downwell::CurrencySmokeComposition oracle,remainder;
    downwell::PaletteEffectSurface<> projected;unsigned promoted=0;
    unsigned empty=0,raster=0,mixed=0,overflow=0,covered=0,shaded=0,maxSmoke=0,maxDebris=0;
    unsigned partial=0,shadePartial=0;
    if(census)std::puts("frame,visible_gems,smoke,debris,entry,promoted,parts,shade_pixels_changed,dead");
    for(unsigned tick=1;tick<=1800;++tick){
        const unsigned fragmentsBefore=live.builder.fragmentCount();
        auto input=downwell::gameplayContactProfileInput(game.tick(),240+game.player().offsetX(),
            176+game.player().offsetY(),downwell::gameplayContactProfilePhase);
        if(overflowTape)require(downwell::gameplayEncounterProfileInput(game.tick(),240+game.player().offsetX(),
            176+game.player().offsetY(),4,input),"ordinary seed-8 route-4 inputs");
        game.step(downwell::packReplayInput(input));host.step(input,false);parityFrame=tick-1;
        compareInitialGameplay(game,host);
        if(generation)for(unsigned i=fragmentsBefore;i<live.builder.fragmentCount();++i)
            generationCensus(tick,patterns,live.fragments[i]);
        require(game.effectsSucceeded()&&game.generationSucceeded(),"runtime");
        if(witness){
            std::uint64_t values[5];amigaOverflowWitness(game,values);
            std::printf("WITNESS %u",tick);
            for(auto value:values)std::printf(" %08x %08x",unsigned(value>>32),unsigned(value));
            std::puts("");
        }
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
        AmigaGameplayDrawList complete;
        require(complete.collect(game,ab.data(),ab.data()+ab.size(),vx,vy,true,&frames),"reference actor prefix");
        const bool referenceHardware=complete.promoteCurrencyComplete(game,vx,vy,assets,frames);
        const bool hardware=list.promoteCurrency(game,vx,vy,assets,frames);
        if(pixelWitness&&tick==1397)for(unsigned i=firstCurrency;i<list.count;++i){
            const auto& b=list.actors[i];
            std::printf("CURRENCY_BOB %u %u %u %d %d %d\n",tick,b.sprite,b.frame,b.x,b.y,b.flip);
        }
        if(referenceHardware)require(hardware&&!list.currencyRasterRequired&&list.count==complete.count,
            "complete promotion remains admitted with identical suffix length");
        for(unsigned i=0;i<(referenceHardware?list.count:firstCurrency);++i){
            const auto& a=list.actors[i];const auto& b=complete.actors[i];
            require(a.sprite==b.sprite&&a.frame==b.frame&&a.x==b.x&&a.y==b.y&&a.flip==b.flip&&a.prepared==b.prepared,
                "partition preserves actor prefix and complete-promotion painter order");
        }
        require(game.checkpoint()==checkpoint,"promotion is read-only, including fallback");
        using Real=downwell::MotionBinary64;
        const downwell::EffectSurfacePlacement::View view{Real::fromInteger(vx),Real::fromInteger(vy),
            Real::fromInteger(160),Real::fromInteger(284)};
        downwell::CurrencySmokeComposition::Settings settings;settings.noControl=game.shops().noControl;
        const auto compose=[&](downwell::CurrencySmokeComposition& target,unsigned first){
        require(target.prepareWithLateEffects([&](auto emit){
            for(unsigned i=game.gemSlotEnd();i>first;--i){const auto& g=game.gemAt(i-1);
                if(g.active&&g.motion.lifetime.visible())emit(g.large,g.motion.animation.frame(),
                    std::int32_t(240+g.x),std::int32_t(176+g.y));}
        },game.smokeEffects(),assets,view,settings,[&](auto emit){return game.visitDebrisDraw([&](const auto& d){
            return downwell::DebrisDraw::surface(d,assets,view,emit);});
        },[&](auto emit){for(unsigned i=0;i<game.enemyCount();++i){const auto& e=game.enemyAt(i);
            if(e.active&&e.awake)emit(std::int32_t(240+e.x),std::int32_t(176+e.y));}
        },[](Real n,std::int32_t& out){return n.roundPixel(out);}),"compositor oracle");
        };
        compose(oracle,0);
        if(pixelWitness&&(tick==1303||tick==1308||(tick>=1397&&tick<=1404)))
            oracle.surface().visit([&](unsigned x,unsigned y,unsigned alpha,unsigned low,unsigned high){
                std::printf("PIXELS %u %u %u %04x %04x %04x\n",tick,x,y,alpha,low,high);
            });
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
        if(shadeChanged){++shaded;require(!hardware||list.currencyRasterRequired,"shade-subtracted currency stays raster");}
        if(parts>8)++overflow;
        if(census)std::printf("%u,%u,%u,%u,%u,%u,%u,%u,%u\n",tick,visible,smoke,debris,
            unsigned(list.entryCoverVisible),unsigned(hardware),parts,unsigned(shadeChanged),unsigned(game.dead()));
        if(!hardware){
            require(list.count==firstCurrency,"fallback preserves the existing actor prefix");
            if(!visible&&!smoke&&!debris&&!list.entryCoverVisible)++empty;
            else ++raster;
            if(smoke||debris)++mixed;
            if(list.entryCoverVisible)++covered;
            continue;
        }
        ++promoted;
        if(list.currencyRasterRequired)++partial;
        if(shadeChanged)++shadePartial;
        compose(remainder,list.currencyRasterFirst);
        unsigned channels=0;
        for(unsigned i=firstCurrency;i<list.count;++i){const auto& bob=list.actors[i];
            const auto& a=*assets.currency(bob.sprite==134,bob.frame);channels+=bob.prepared->partCount[0];
            remainder.surface().add({{a.mask.data,a.mask.bytes,a.mask.width,a.mask.height,a.mask.stride},a.low,a.high},
                bob.x-downwellPlayfieldX-a.originX,bob.y-a.originY);
        }
        require((!list.currencyRasterRequired||channels<=8)&&remainder.surface().samePixels(oracle.surface()),
            "partitioned final alpha and colours, including planar overflow");
        if(overflowTape&&tick==1397){
            // Projection-only adversarial fixture, not natural tape coverage:
            // overlap all visible gems at a clipped, non-word-aligned origin.
            // Restore published positions before the next source Step.
            int saved[GameplayWorld::gemCapacity][2];
            for(unsigned i=0;i<game.gemSlotEnd();++i){
                auto& g=const_cast<GameplayWorld::GemState&>(game.gemAt(i));
                saved[i][0]=g.x;saved[i][1]=g.y;
                if(g.active&&g.motion.lifetime.visible()&&
                    downwell::EffectSurfacePlacement::pixelInView(240+g.x,176+g.y,{vx,vy,160,284})){
                    g.x=vx+3-240;g.y=vy+3-176;
                }
            }
            list.count=firstCurrency;
            require(list.promoteCurrency(game,vx,vy,assets,frames),"overlapping nine-gem admission");
#if defined(DOWNWELL_REFERENCE_CURRENCY_BOBS) || defined(DOWNWELL_REFERENCE_SHADE_PARTITION)
            require(list.currencyRasterRequired&&list.currencySpriteCount==8,"reference nine-gem suffix partition");
#else
            require(!list.currencyRasterRequired&&list.currencySpriteCount==9,"unshaded nine-gem actor composition");
#endif
            compose(oracle,0);compose(remainder,list.currencyRasterFirst);
            for(unsigned i=firstCurrency;i<list.count;++i){const auto& bob=list.actors[i];
                const auto& a=*assets.currency(bob.sprite==134,bob.frame);
                remainder.surface().add({{a.mask.data,a.mask.bytes,a.mask.width,a.mask.height,a.mask.stride},a.low,a.high},
                    bob.x-downwellPlayfieldX-a.originX,bob.y-a.originY);
            }
            require(remainder.surface().samePixels(oracle.surface()),"overlapping clipped gems preserve alpha, black and painter order");
            for(unsigned i=0;i<game.gemSlotEnd();++i){
                auto& g=const_cast<GameplayWorld::GemState&>(game.gemAt(i));
                g.x=saved[i][0];g.y=saved[i][1];
            }
        }
        list.count=AmigaGameplayDrawList::capacity;
        require(!list.promoteCurrency(game,vx,vy,assets,frames)&&list.count==AmigaGameplayDrawList::capacity,
            "full draw list fallback");
        list.count=0;list.entryCoverVisible=true;
        require(!list.promoteCurrency(game,vx,vy,assets,frames)&&!list.count,"entry-cover fallback");
    }
    if(overflowTape){
        require(overflow>0,"natural seed-8 route-4 tape exercises sprite-channel overflow");
        require(shaded>0,"natural tape exercises actual shade subtraction");
        require(partial>0,"natural tape exercises partial hardware promotion");
#ifndef DOWNWELL_REFERENCE_SHADE_PARTITION
        require(shadePartial>0,"natural tape promotes an unshaded suffix above shaded raster currency");
#endif
    }
    else require(promoted>50,"contact tape exercises promotion");
    std::printf("Currency promotion: %u exact frames in 1800-frame source-backed replay\n",promoted);
    require(promoted+empty+raster==1800,"every frame has a presentation class");
    std::printf("Effect coverage: hardware=%u empty=%u raster=%u; fallback mixed=%u overflow=%u entry=%u shade=%u; max live smoke=%u debris=%u\n",
        promoted,empty,raster,mixed,overflow,covered,shaded,maxSmoke,maxDebris);
    std::printf("Partial hardware/raster frames: %u\n",partial);
    std::printf("Shade-subtracted frames with a hardware suffix: %u\n",shadePartial);
}
