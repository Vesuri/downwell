#include "platform/amiga/AmigaGameplayDrawList.h"
#include "core/ShopFixtures.h"
#include "core/GameMakerRounding.h"
#include "core/GameplayCollisionOwner.h"
#include "core/CurrencySmokeComposition.h"
#include "core/TimeShardDraw.h"
#include "../../../generated/downwell-title-glyphs.h"
#include <fstream>
#include <iterator>
#include <memory>
#include <cassert>
#include <cstdio>
#include <vector>
static bool boundsOK=true;
int amigaRendererAssetBounds(const UBYTE*,const UBYTE*,UWORD sprite,UWORD,int x,int y,int,
    int* left,int* top,int* width,int* height) {
    if(sprite==gameplayDitherEdge){*left=x-160;*top=y;*width=320;*height=80;return boundsOK;}
    *left=x-8; *top=y-8; *width=*height=16; return boundsOK;
}
int main() {
    {
        auto read=[](const char* path){std::ifstream f(path,std::ios::binary);
            return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f),{});};
        const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc"),rb=read("generated/downwell-rooms.dwrm");
        downwell::RoomPack rooms(rb.data(),rb.size());
        downwell::ObjectPack objects(ob.data(),ob.size());
        downwell::SpriteCollisionPack sprites(sb.data(),sb.size());
        const auto mb=read("generated/downwell-sprite-masks.dwsm");downwell::SpriteMaskPack masks(mb.data(),mb.size());
        for(auto mode:{downwell::GameplayCollisionOwner::Ownership::reference,
            downwell::GameplayCollisionOwner::Ownership::native,downwell::GameplayCollisionOwner::Ownership::nativeScenery}){
            const bool native=mode!=downwell::GameplayCollisionOwner::Ownership::reference;
            auto owner=std::make_unique<downwell::GameplayCollisionOwner>(objects,sprites,
                downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain))&&owner->configureCorpseMasks(masks));
            {
                downwell::DeferredCreateInstance command(289,240,128);
                command.integerImageIndex=12;command.integerScaleX=-1;
                assert(owner->scattered().adopt(105189,command));
                const auto key=owner->scattered().handle(0);
                UBYTE pack[8]{};
                for(int sprite:{550,551}){
                    assert(owner->scattered().sprite(key,sprite));
                    for(bool composite:{false,true}){
                        AmigaGameplayDrawList list;assert(list.collect(*world,pack,pack+8,160,34,composite));
                        unsigned seen=0;
                        for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==sprite){
                            const auto& actor=list.actors[i];
                            assert(actor.frame==12&&actor.x==190&&actor.y==94&&actor.flip);++seen;
                        }
                        assert(seen==1);
                    }
                }
                assert(owner->scattered().markDestroyed(key));
                AmigaGameplayDrawList retired;assert(retired.collect(*world,pack,pack+8,160,34,true));
                for(unsigned i=0;i<retired.count;++i)assert(retired.actors[i].sprite!=551);
                assert(owner->scattered().clear());
            }
            {
                auto list=std::make_unique<AmigaGameplayDrawList>();UBYTE pack[8]{};
                downwell::InstanceHandle title;
                assert(owner->levelNumbers().create(105190,{}, {},[](auto,auto& state){
                    return state.create(1,downwell::MotionBinary64::fromInteger(8),[](unsigned,unsigned& token){token='A';return true;});
                },title));
                assert(!list->collect(*world,pack,pack+sizeof(pack),0,0,true));
                downwell::LevelNumberTextDrawState text{2,0,0,123,downwell::MotionBinary64::fromInteger(1)};
                assert(list->bindLevelNumberText(&downwell::title_glyphs::bank,&text));
                assert(list->collect(*world,pack,pack+sizeof(pack),0,0,true));
                assert(owner->levelNumbers().find(title)->state.characters[1].offset==-2); // Collection is read-only.
                auto surface=std::make_unique<downwell::CurrencySmokeComposition>();downwell::EffectMaskAssets art;
                using Real=downwell::MotionBinary64;
                const downwell::EffectSurfacePlacement::View camera{Real::fromInteger(1234),Real::fromInteger(5678),Real::fromInteger(160),Real::fromInteger(284)};
                downwell::CurrencySmokeComposition::Settings settings;settings.death=true;
                assert(surface->prepareWithSurfaceProducers([](auto){},world->smokeEffects(),art,camera,settings,
                    [](auto){assert(false);return false;},[&](auto emit){
                        return world->visitUnconditionalSurfaceDraw([&](auto kind,auto key,auto){
                            return kind==GameplayWorld::DrawActor::levelNumber&&list->drawLevelNumber(*world,key,camera,emit);
                        });
                    },[](auto){},[](Real n,std::int32_t& out){return n.roundPixel(out);}));
                assert(surface->visible()&&text.color==0xffffff&&owner->levelNumbers().find(title)->state.characters[1].offset==-1);
                unsigned black=0,white=0;
                std::vector<unsigned char> pixels(160*284),expected(160*284);
                surface->surface().visit([&](unsigned x,unsigned y,std::uint16_t alpha,std::uint16_t lo,std::uint16_t hi){
                    assert(x>=64&&x<96&&y>=81&&y<96);black+=(alpha&lo&hi)!=0;white+=(alpha&lo&~hi)!=0;
                    for(unsigned bit=0;bit<16;++bit)if(alpha&(0x8000>>bit))
                        pixels[y*160+x+bit]=((lo>>(15-bit))&1)|(((hi>>(15-bit))&1)<<1);
                });
                assert(black&&white);
                const int dx[]={1,-1,0,0,0},dy[]={0,0,1,-1,0};
                for(unsigned pass=0;pass<5;++pass){
                    downwell::EffectMaskAssets::Asset glyph;bool ink;
                    assert(downwell::title_glyphs::bank.resolve(2,'A',pass==4?0xffffff:0,glyph,ink)&&ink);
                    for(unsigned y=0;y<glyph.mask.height;++y)for(unsigned x=0;x<glyph.mask.width;++x)
                        if(glyph.mask.data[y*glyph.mask.stride+x/8]&(0x80>>(x%8)))
                            expected[(82+dy[pass]+y)*160+76+dx[pass]-glyph.originX+x]=pass==4?1:3;
                }
                assert(pixels==expected);
                assert(list->bindLevelNumberText(nullptr,nullptr)&&owner->levelNumbers().clear());
            }
            {
                downwell::InstanceHandle shard;
                assert(world->createLimboShardWithSourceId(105193,downwell::MotionBinary64::fromInteger(240),
                    downwell::MotionBinary64::fromInteger(128),false,shard));
                UBYTE pack[8]{};
                for(bool composite:{false,true}){
                    AmigaGameplayDrawList list;assert(list.collect(*world,pack,pack+8,160,34,composite));
                    unsigned seen=0;
                    for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==91){
                        assert(list.actors[i].x==190&&list.actors[i].y==94&&!list.actors[i].flip);++seen;
                    }
                    assert(seen==1); // Limbo Draw is direct even with surfaceFx composition.
                }
                assert(owner->limboShards().clear());
            }
            {
                downwell::InstanceHandle flash;downwell::GameMakerRandom random(23);
                assert(owner->registeredFlash().create(105194,downwell::MotionBinary64::fromInteger(240),
                    downwell::MotionBinary64::fromInteger(128),random,flash));
                const auto* r=owner->registeredFlash().find(flash);assert(r);
                UBYTE pack[8]{};
                for(bool composite:{false,true}){
                    AmigaGameplayDrawList list;assert(list.collect(*world,pack,pack+8,160,34,composite));
                    unsigned seen=0;
                    for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==96){
                        assert(list.actors[i].x==r->state.pixelX()-50&&list.actors[i].y==r->state.pixelY()-34&&
                            list.actors[i].frame==1&&!list.actors[i].flip);++seen;
                    }
                    assert(seen==1); // Registered flash stays direct, including surfaceFx mode.
                }
                assert(owner->registeredFlash().clear());
            }
            int blastRoomSpeed=60;assert(world->bindControllerRoomSpeed(blastRoomSpeed));
            {
                downwell::InstanceHandle blast;
                assert(owner->createBlast(700000,downwell::MotionBinary64::fromInteger(240),
                    downwell::MotionBinary64::fromInteger(128),blast));
                UBYTE pack[8]{};
                for(unsigned tick=0;tick<28;++tick){
                    const auto* r=owner->registeredBlasts().find(blast);assert(r);
                    for(bool composite:{false,true}){
                        AmigaGameplayDrawList list;assert(list.collect(*world,pack,pack+8,160,34,composite));
                        unsigned seen=0;
                        for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==471){
                            assert(list.actors[i].x==190&&list.actors[i].y==94&&list.actors[i].frame==tick/2&&
                                list.actors[i].flip==int(r->publishedScale.bits>>31));++seen;
                        }
                        assert(seen==1);
                    }
                    assert(owner->registeredBlasts().advanceAnimation(blast));
                }
                assert(owner->prepareDraw()&&!owner->registeredBlasts().find(blast));
            }
            auto& corpses=owner->registeredCorpses();downwell::GameMakerRandom random(2);
            const downwell::MotionBinary64 x{0x406f0800u,0},y{0x40601000u,0};
            downwell::DeadBodySource source{x,y,{},downwell::MotionBinary64::fromInteger(-1)};
            assert(corpses.createFromDeath(105187,source,206,{},102,false,random,
                [](auto,int,auto,auto,bool& hit){hit=false;return true;}));
            UBYTE pack[8]{};AmigaGameplayDrawList direct;
            assert(direct.collect(*world,pack,pack+8,160,34,false));unsigned seen=0;
            for(unsigned i=0;i<direct.count;++i)if(direct.actors[i].sprite==206){
                const auto& a=direct.actors[i];assert(a.frame==0&&a.x==198&&a.y==94&&a.flip==1);++seen;
            }
            assert(seen==1);AmigaGameplayDrawList composite;
            assert(composite.collect(*world,pack,pack+8,160,34,true));
            for(unsigned i=0;i<composite.count;++i)assert(composite.actors[i].sprite!=206);
            const auto pixels=read("generated/amiga-game.dwap");downwell::EffectMaskAssets art;
            assert(art.bind(pixels.data(),pixels.size()));downwell::EffectMaskAssets::Asset expected;
            assert(art.corpse(206,true,expected));unsigned surfaces=0;
            const downwell::EffectSurfacePlacement::View effectView{downwell::MotionBinary64::fromInteger(160),
                downwell::MotionBinary64::fromInteger(34),downwell::MotionBinary64::fromInteger(160),
                downwell::MotionBinary64::fromInteger(284)};
            assert(world->visitRegisteredEffectSurfaces(art,effectView,[&](const auto& asset,auto px,auto py){
                assert(asset.mask.data==expected.mask.data&&px.high==x.high&&px.low==x.low&&py.high==y.high&&py.low==y.low);
                ++surfaces;return true;
            })&&surfaces==1);
            assert(!world->visitRegisteredEffectSurfaces(art,effectView,[](const auto&,auto,auto){return false;}));
            assert(corpses.clear());
            struct Globals {
                downwell::MotionBinary64 grav{};
                bool gravity(downwell::MotionBinary64 v){grav=v;return true;}
                downwell::MotionBinary64 gravity(){return grav;}downwell::MotionBinary64 fallSpeed(){return {};}
            } globals;
            auto& particles=owner->consumptionParticles();downwell::InstanceHandle particle;
            assert(particles.create(105188,x,y,random,globals,particle));
            unsigned frameMask=0;
            for(unsigned tick=0;tick<30;++tick){
                unsigned frame;assert(particles.find(particle)->state.drawFrame(frame));frameMask|=1u<<frame;
                for(bool composite:{false,true}){
                    AmigaGameplayDrawList list;assert(list.collect(*world,pack,pack+8,160,34,composite));unsigned count=0;
                    for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==92){
                        const auto& a=list.actors[i];assert(a.frame==frame&&a.x==198&&a.y==94&&!a.flip);++count;
                    }
                    assert(count==1);
                }
                assert(particles.advanceAnimation(particle));
            }
            assert(frameMask==255&&particles.clear());
        }
        for(auto mode:{downwell::GameplayCollisionOwner::Ownership::reference,
            downwell::GameplayCollisionOwner::Ownership::native,
            downwell::GameplayCollisionOwner::Ownership::nativeScenery}){
            auto owner=std::make_unique<downwell::GameplayCollisionOwner>(objects,sprites,
                downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
            assert(owner->attach(*world));
            auto& expiry=owner->registeredExpiry();downwell::InstanceHandle local,shared;
            const downwell::MotionBinary64 x{0x406f0800u,0},y{0x40601000u,0}; // 248.25, 128.5
            assert(expiry.create(105187,x,y,local)&&
                downwell::GameplayCollisionView::encode(downwell::GameplayCollisionView::Kind::corpseExpiry,local,shared));
            downwell::GameplayCollisionView view(*world);downwell::GameplayCollisionView::Record resolved;
            assert(view.resolve(shared,resolved)&&resolved.corpseExpiry==expiry.find(local)&&resolved.corpseExpiry->sourceId==105187);
            unsigned projected=0;downwell::PackedObject definition;assert(objects.findObject(242,definition));
            assert(world->visitRoomActors([&](auto kind,auto key,auto depth){
                if(kind==GameplayWorld::DrawActor::corpseExpiry){
                    std::int32_t value;assert(key.slot==local.slot&&key.generation==local.generation&&
                        depth.promoted().roundPixel(value)&&value==definition.creationDepth);++projected;
                }
                return true;
            })&&projected==1);
            assert(world->corpseExpiryEffects().size()==0);
            downwell::CorpseExpiryEffect oracle;assert(oracle.create(x,y));UBYTE pack[8]{};
            for(unsigned tick=0;tick<40;++tick){
                AmigaGameplayDrawList list;assert(list.collect(*world,pack,pack+8,160,34));unsigned seen=0;
                for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==102){
                    const auto& a=list.actors[i];assert(a.frame==oracle.drawFrame()&&a.x==198&&a.y==94&&a.flip==0);++seen;
                }
                assert(seen==1&&oracle.advanceImage());world->step(0);assert(world->effectsSucceeded()&&owner->valid());
                const auto* r=expiry.find(local);
                if(!oracle.destroyRequested)assert(r&&r->state.timeline.bits==oracle.timeline.bits&&r->state.imageIndex.bits==oracle.imageIndex.bits);
                else assert(r&&r->destroyRequested&&view.contains(shared));
                assert(owner->prepareDraw());
                if(oracle.destroyRequested)assert(!expiry.find(local)&&!view.contains(shared));
            }
            AmigaGameplayDrawList ended;assert(oracle.destroyRequested&&ended.collect(*world,pack,pack+8,160,34));
            for(unsigned i=0;i<ended.count;++i)assert(ended.actors[i].sprite!=102);
            downwell::InstanceHandle beforeRetry,afterRetry;
            assert(expiry.create(105188,x,y,beforeRetry));
            assert(world->restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,300)==downwell::ResultsDestination::gameplay&&
                world->registeredExpiry()==&expiry&&!expiry.find(beforeRetry)&&!expiry.size()&&owner->valid());
            assert(expiry.create(105189,x,y,afterRetry)&&afterRetry.generation>beforeRetry.generation&&expiry.clear());
            auto& smoke=owner->registeredSmoke();downwell::GameMakerRandom random(2);downwell::InstanceHandle smokeKey;
            assert(smoke.create(105190,x,y,random,smokeKey));
            const auto pixels=read("generated/amiga-game.dwap");downwell::EffectMaskAssets art;
            assert(art.bind(pixels.data(),pixels.size()));
            const downwell::EffectSurfacePlacement::View effectView{downwell::MotionBinary64::fromInteger(160),
                downwell::MotionBinary64::fromInteger(34),downwell::MotionBinary64::fromInteger(160),
                downwell::MotionBinary64::fromInteger(284)};
            unsigned frames=0,frameMask=0;
            while(!smoke.find(smokeKey)->destroyRequested){
                const auto& state=smoke.find(smokeKey)->state;const unsigned frame=state.drawFrame();frameMask|=1u<<frame;
                AmigaGameplayDrawList direct;assert(direct.collect(*world,pack,pack+8,160,34,false));unsigned seen=0;
                for(unsigned i=0;i<direct.count;++i)if(direct.actors[i].sprite==84){
                    assert(direct.actors[i].frame==frame&&direct.actors[i].x==198&&direct.actors[i].y==94);++seen;
                }
                assert(seen==1);
                AmigaGameplayDrawList composited;assert(composited.collect(*world,pack,pack+8,160,34,true));
                for(unsigned i=0;i<composited.count;++i)assert(composited.actors[i].sprite!=84);
                unsigned surfaces=0;
                assert(world->visitRegisteredEffectSurfaces(art,effectView,[&](const auto& asset,auto px,auto py){
                    assert(asset.mask.data==art.smoke(frame)->mask.data&&px.high==x.high&&px.low==x.low&&
                        py.high==y.high&&py.low==y.low);++surfaces;return true;
                })&&surfaces==1);
                assert(!world->visitRegisteredEffectSurfaces(art,effectView,[](const auto&,auto,auto){return false;}));
                assert(smoke.advanceAnimation(smokeKey)&&++frames<200);
            }
            assert(frameMask==511&&owner->prepareDraw()&&!smoke.find(smokeKey));
            assert(world->visitRegisteredEffectSurfaces(art,effectView,[](const auto&,auto,auto){return false;}));
        }
        for(auto mode:{downwell::GameplayCollisionOwner::Ownership::reference,
            downwell::GameplayCollisionOwner::Ownership::native,
            downwell::GameplayCollisionOwner::Ownership::nativeScenery}){
            auto owner=std::make_unique<downwell::GameplayCollisionOwner>(objects,sprites,
                downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},&rooms,mode);
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
            assert(owner->attach(*world)&&world->createAuthoredDitherAbove());
            const auto* r=world->ditherAbove()->find(world->ditherAbove()->handle());
            int sourceX,sourceY;assert(r&&r->x.roundPixel(sourceX)&&r->y.roundPixel(sourceY));
            for(int x:{-400,-160,-159,0,80,160,319,320,560})
            for(int y:{-800,-80,-79,-50,0,1,284,720,1004}){
                UBYTE pack[8]{};AmigaGameplayDrawList list;
                assert(list.collect(*world,pack,pack+8,sourceX-x,sourceY-y));
                unsigned rects=0,edges=0;
                for(unsigned i=0;i<list.count;++i){const auto& a=list.actors[i];
                    if(a.sprite==gameplayBlackRectMarker){
                        ++rects;assert(a.x>=110&&a.x+a.frame<=270&&a.y>=0&&a.y+a.flip<=284);
                    }
                    if(a.sprite==gameplayDitherEdge){++edges;assert(a.x==110+x&&a.y==y);}
                }
                const bool horizontal=x+160>0&&x-160<160;
                assert(rects==unsigned(horizontal&&y>0&&y-720<284));
                assert(edges==unsigned(horizontal&&y+80>0&&y<284));
                assert(!list.entryCoverVisible&&r->x.promoted().roundPixel(sourceX)&&r->y.promoted().roundPixel(sourceY));
            }
        }
        {
            auto owner=std::make_unique<downwell::GameplayCollisionOwner>(objects,sprites,
                downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},&rooms);
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
            assert(owner->attach(*world));auto& entries=owner->entryDithers();
            UBYTE pack[8]{};AmigaGameplayDrawList list;
            assert(list.collect(*world,pack,pack+8,160,34)&&!list.entryCoverVisible);
            const unsigned bobs=list.count;unsigned sourceId=400001;
            auto create=[&](int x,int y){
                downwell::InstanceHandle key;
                return entries.create(sourceId++,
                    [&](auto& value){value=downwell::MotionBinary64::fromInteger(x);return true;},
                    [&](auto& value){value=downwell::MotionBinary64::fromInteger(y);return true;},key);
            };
            for(int y:{-116,320,400,683,684,800}){
                assert(entries.clear()&&create(240,y+34));
                const auto checkpoint=world->checkpoint();
                assert(list.collect(*world,pack,pack+8,160,34)&&list.entryCoverVisible&&list.entryCover.y==y);
                unsigned offset;assert(downwell::EntryDitherMask::offset(y,offset)&&list.entryCover.offset==offset);
                assert(list.count==bobs&&world->checkpoint()==checkpoint); // No BOB and no simulation mutation.
            }
            bool hardware=false;
            assert(list.entryCoverHardwareEligible(*world,160,34,hardware)&&hardware);
            assert(world->configureEnemy(0,112,163,0,0,0,0,5,14,0,0,-1,-1,0,0,true,true,true));
            assert(list.entryCoverHardwareEligible(*world,160,34,hardware)&&!hardware);
            assert(world->configureEnemy(0,112,163,0,1000,0,0,5,14,0,0,-1,-1,0,0,true,true,true));
            assert(list.entryCoverHardwareEligible(*world,160,34,hardware)&&hardware);
            assert(entries.clear()&&create(241,400)); // Off-center is not silently cropped.
            assert(!list.collect(*world,pack,pack+8,160,34));
            assert(entries.clear()&&create(240,-83)); // Finite bottom would be on screen.
            assert(!list.collect(*world,pack,pack+8,160,34));
            assert(entries.clear()&&create(240,400)&&create(240,400));
            assert(!list.collect(*world,pack,pack+8,160,34)); // No lossy cover coalescing.
            assert(entries.clear()&&list.collect(*world,pack,pack+8,160,34)&&!list.entryCoverVisible);
            downwell::EntryDitherPlacement untouched{17,23};
            assert(!downwell::EntryDitherPlacement::resolve({0x43708000u},{0},
                downwell::MotionBinary64::fromInteger(160),{},untouched));
            assert(untouched.y==17&&untouched.offset==23); // x=240.5 is not 240.
        }
        {
            auto layers=std::make_unique<downwell::GameplayRoomLayers<4>>(&rooms);
            assert(layers->prepare());
            downwell::RunnerImageValue zero,back;
            assert(back.assign(downwell::MotionBinary64::fromInteger(1000)));
            assert(layers->birth({0,1},0,zero,false)&&
                layers->birth({1,1},0,zero,false,false)&&layers->birth({2,1},0,back,false));
            bool reused=false;
            const auto visit=[&](){
                std::vector<unsigned> seen;
                assert(layers->visit([&](auto h,auto& out){
                    assert(h.slot!=1||reused); // Scenery needs no actor lookup.
                    out.flags=0x10;out.object=out.sprite=true;return true;
                },[&](auto h,int){seen.push_back(h.slot);return true;}));
                return seen;
            };
            assert((visit()==std::vector<unsigned>{2,0}));
            // Flag publication precedes relinking. The unprojected head must
            // terminate its layer, without hiding a different layer's actor.
            assert(layers->activation({1,1},false));
            assert((visit()==std::vector<unsigned>{2}));
            assert(layers->reconcile({1,1}));
            assert((visit()==std::vector<unsigned>{2,0}));
            assert(layers->activation({1,1},true)&&layers->reconcile({1,1}));
            assert((visit()==std::vector<unsigned>{2,0}));
            assert(layers->retire({1,1})&&layers->birth({1,2},0,zero,false));reused=true;
            assert((visit()==std::vector<unsigned>{2,1,0}));
            assert(layers->retire({0,1})&&layers->retire({1,2})&&layers->retire({2,1})&&layers->prepare());
            assert(layers->birth({1,3},0,zero,false,false));reused=false;
            assert(visit().empty());
        }
        {
            auto hiddenBytes=rb;downwell::PackedRoom room;
            assert(rooms.findRoom(11,room));bool changed=false;
            for(unsigned i=0;i<room.layerCount;++i){
                downwell::PackedRoomLayer layer;assert(rooms.roomLayer(room,i,layer));
                if(layer.depth!=-50000)continue;
                unsigned id;assert(layer.membershipCount==1&&rooms.membership(layer.firstMembership,id)&&id==100855);
                const auto offset=downwell::RoomPack::headerSize+rooms.roomCount()*downwell::RoomPack::roomRecordSize+
                    rooms.instanceCount()*downwell::RoomPack::instanceRecordSize+(room.firstLayer+i)*downwell::RoomPack::layerRecordSize+8;
                hiddenBytes[offset]&=0xfe;changed=true;
            }
            downwell::RoomPack hidden(hiddenBytes.data(),hiddenBytes.size());assert(changed&&hidden.valid());
            auto layers=std::make_unique<downwell::GameplayRoomLayers<4>>(&hidden);
            assert(layers->prepare()&&layers->playerSourceId()==100855);
            downwell::RunnerImageValue depth;assert(depth.assign(downwell::MotionBinary64::fromInteger(-50000)));
            assert(layers->birth({0,1},100855,depth,false)&&layers->birth({1,1},0,depth,false));
            unsigned visited=0;
            assert(layers->visit([](auto,auto& out){out.flags=0x10;out.object=out.sprite=true;return true;},
                [&](auto h,int d){assert(h.slot==1&&d==-50000);++visited;return true;})&&visited==1);
            assert(layers->retire({0,1})&&layers->retire({1,1})&&layers->prepare());
            assert(layers->birth({0,2},100855,depth,false)&&!layers->retire({0,1}));

            auto hiddenOwner=std::make_unique<downwell::GameplayCollisionOwner>(objects,sprites,
                downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},&hidden);
            auto hiddenWorld=std::make_unique<GameplayWorld>(300,true,0,false,hiddenOwner->enemyBinding(),
                hiddenOwner->breakableBinding(),hiddenOwner->playerBinding(),&hiddenOwner->terrain());
            assert(hiddenOwner->attach(*hiddenWorld));
            const_cast<downwell::UpgradeInventory&>(hiddenWorld->upgrades()).levels[12]=1;
            UBYTE dummy[8]{};AmigaGameplayDrawList hiddenList;
            assert(hiddenList.collect(*hiddenWorld,dummy,dummy+8,160,34)&&hiddenList.count==0&&!hiddenList.rayVisible);
        }
        auto owner=std::make_unique<downwell::GameplayCollisionOwner>(objects,sprites,
            downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},&rooms);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->valid()&&owner->attach(*scene)&&scene->hasRoomActorDraw());
        auto n=[](int v){return downwell::MotionBinary64::fromInteger(v);};
        auto frames=[&](int id,unsigned& count){downwell::PackedSpriteCollision s;
            if(!sprites.sprite(id,s))return false;count=s.frameCount;return true;};
        downwell::InstanceHandle a,b;
        assert(scene->spawnScriptEffect(n(240),n(120),603,{0x3fe00000u,0},n(270),{},frames,a));
        assert(scene->configureEnemy(0,112,163,0,0,0,0,5,14,0,0,-1,-1,0,0,true,true,true));
        assert(scene->spawnScriptEffect(n(244),n(124),602,{0x3fe00000u,0},{},{},frames,b));
        assert(scene->emitCorpseExpiry(n(240),n(128)));
        std::vector<GameplayWorld::DrawActor> kinds;
        assert(scene->visitRoomActors([&](auto kind,auto,auto){kinds.push_back(kind);return true;}));
        downwell::PackedObject def;assert(objects.findObject(112,def)&&def.creationDepth==-10000);
        assert(
            kinds==std::vector<GameplayWorld::DrawActor>({GameplayWorld::DrawActor::scriptEffect,
                GameplayWorld::DrawActor::scriptEffect,GameplayWorld::DrawActor::enemy,GameplayWorld::DrawActor::player}));
        UBYTE pack[8]{};AmigaGameplayDrawList list;const auto checkpoint=scene->checkpoint();
        assert(list.collect(*scene,pack,pack+8,160,34)&&scene->checkpoint()==checkpoint);
        assert(list.actors[0].sprite==602&&list.actors[0].frame==0&&list.actors[0].x==194&&list.actors[0].y==90);
#ifndef DOWNWELL_REPLAY_VERIFY
        assert(list.actors[1].sprite==603&&list.actors[1].frame==768&&
            list.actors[2].sprite==102&&list.actors[3].sprite==163);
#else
        assert(list.actors[1].sprite==603&&list.actors[1].frame==768);
#endif
        unsigned visited=0;
        assert(!scene->visitRoomActors([&](auto,auto,auto){++visited;return false;})&&visited==1);
        {
            auto& inventory=const_cast<downwell::UpgradeInventory&>(scene->upgrades());
            inventory.levels[12]=1; // Presentation-only fixture.
            const auto player=scene->playerHandle();const auto* record=scene->findPlayer(player);
            assert(record&&list.collect(*scene,pack,pack+8,160,34)&&list.rayVisible);
            assert(owner->playerBinding().activation(player,*record,false)&&owner->reconcileActivation());
            assert(list.collect(*scene,pack,pack+8,160,34)&&!list.rayVisible);
            for(unsigned i=0;i<list.count;++i)
                assert(list.actors[i].sprite!=scene->player().spriteId(false)&&list.actors[i].sprite!=gameplayRayMarker);
            assert(owner->playerBinding().activation(player,*record,true)&&owner->reconcileActivation());
            // The drawing consumer must respect its owner's sequence. This
            // synthetic visitor exercises a future deeper impact without
            // claiming that native fixed-layer migration is integrated yet.
            struct Tape {downwell::InstanceHandle player,effect;bool stale=false;} tape{player,b};
            scene->configureRoomActorDraw([](const void* context,GameplayWorld::DrawActorVisitor visit,void* sink){
                const auto& t=*static_cast<const Tape*>(context);auto h=t.player;
                if(t.stale)++h.generation;
                downwell::RunnerImageValue playerDepth,effectDepth;
                playerDepth.assign(downwell::MotionBinary64::fromInteger(-50000));
                effectDepth.assign(downwell::MotionBinary64::fromInteger(-60000));
                return visit(sink,GameplayWorld::DrawActor::player,h,playerDepth)&&
                    visit(sink,GameplayWorld::DrawActor::scriptEffect,t.effect,effectDepth);
            },&tape);
            assert(list.collect(*scene,pack,pack+8,160,34)&&list.rayVisible);
            unsigned playerAt=list.count,rayAt=list.count,effectAt=list.count;
            for(unsigned i=0;i<list.count;++i){
                if(list.actors[i].sprite==scene->player().spriteId(false))playerAt=i;
                if(list.actors[i].sprite==gameplayRayMarker)rayAt=i;
                if(list.actors[i].sprite==602)effectAt=i;
            }
            assert(playerAt<rayAt&&rayAt<effectAt&&effectAt<list.count);
            tape.stale=true;
            assert(!list.collect(*scene,pack,pack+8,160,34)&&!list.rayVisible);
            for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=602);
            assert(owner->attach(*scene)); // Restore production traversal.
            inventory.levels[12]=0;
        }
        std::uint32_t impactId=0;
        assert(scene->spawnScriptEffectWithSourceId(705100,n(248),n(128),605,{0x3fd99999u,0x9999999au},n(270),{},frames,impactId)&&impactId==705100);
        const unsigned impactFrames[]={0,0,0,1,1,2,2,2};
        for(unsigned frame:impactFrames){
            assert(list.collect(*scene,pack,pack+8,160,34));unsigned seen=0;
            for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==605){
                assert(list.actors[i].frame==768+frame&&list.actors[i].x==198&&list.actors[i].y==94);++seen;
            }
            assert(seen==1);scene->step(0);assert(scene->effectsSucceeded());
        }
        assert(owner->prepareDraw()&&list.collect(*scene,pack,pack+8,160,34));
        for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=605);
        downwell::InstanceHandle impact;
        assert(scene->spawnScriptEffect(n(248),n(128),101,{0x3fe00000u,0},{},{},frames,impact));
        for(unsigned tick=0;tick<16;++tick){
            assert(list.collect(*scene,pack,pack+8,160,34));unsigned seen=0;
            for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==101){
                assert(list.actors[i].frame==tick/2&&list.actors[i].x==198&&list.actors[i].y==94);++seen;
            }
            assert(seen==1);scene->step(0);assert(scene->effectsSucceeded());
        }
        assert(owner->prepareDraw()&&list.collect(*scene,pack,pack+8,160,34));
        for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=101);
        downwell::InstanceHandle bone;
        downwell::InstanceHandle skeleton;
        assert(scene->createSkeletonWithSourceId(405189,248,128,skeleton));
        for(int sprite:{208,207,205})for(int facing:{-1,1}){
            const unsigned count=downwell::SkeletonAnimation::frames(sprite);
            for(unsigned frame=0;frame<count;++frame){
                downwell::RunnerImageValue index;assert(index.assignInteger(frame));
                assert(owner->skeletons().dispatchPublished(skeleton,[&](auto&,auto&,auto& publish,bool&){
                    return publish.sprite(sprite)&&publish.imageIndex(index)&&publish.facing(facing);
                }));
                const auto draws=scene->randomDrawCount();
                assert(list.collect(*scene,pack,pack+8,160,34));unsigned seen=0;
                for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==sprite){
                    assert(list.actors[i].frame==frame&&list.actors[i].x==206&&list.actors[i].y==94&&
                        bool(list.actors[i].flip)==(facing<0));++seen;
                }
                assert(seen==1&&scene->randomDrawCount()==draws);
            }
        }
        assert(owner->skeletons().markDestroyed(skeleton)&&owner->prepareDraw());
        assert(list.collect(*scene,pack,pack+8,160,34));
        for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=205);
        assert(scene->createBoneWithSourceId(405190,248,128,bone));
        const auto rngBeforeDraw=scene->randomDrawCount();
        for(unsigned frame=0;frame<4;++frame){
            assert(list.collect(*scene,pack,pack+8,160,34));unsigned seen=0;
            for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==460){
                assert(list.actors[i].frame==frame&&list.actors[i].x==198&&list.actors[i].y==94&&
                    !list.actors[i].flip);++seen;
            }
            assert(seen==1&&scene->randomDrawCount()==rngBeforeDraw&&owner->bones().advanceAnimation(bone));
        }
        assert(owner->bones().markDestroyed(bone)&&
            scene->spawnBoneImpact(bone,frames)&&owner->prepareDraw());
        const auto impactLocal=scene->scriptEffects().birthSequence().next-1;
        unsigned frameMask=0,ticks=0;
        while(const auto* effect=scene->scriptEffects().get(impactLocal)){
            assert(ticks++<24&&list.collect(*scene,pack,pack+8,160,34));unsigned seen=0;
            for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=460);
            for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==85){
                assert(list.actors[i].frame==effect->drawFrame()&&list.actors[i].x==198&&list.actors[i].y==94);
                frameMask|=1u<<list.actors[i].frame;++seen;
            }
            assert(seen==1);scene->step(0);assert(scene->effectsSucceeded()&&owner->prepareDraw());
        }
        assert(frameMask==63&&list.collect(*scene,pack,pack+8,160,34));
        for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=85);
        {
            downwell::TimeShardEffects<64> shards;
            assert(owner->createTimeShardWithSourceId(shards,405288,n(248),n(128),false));
            auto* shard=shards.get(1);assert(shard);
            const auto key=owner->timeShards().handle(1);
            const auto before=scene->randomDrawCount();
            const auto pixels=read("generated/amiga-game.dwap");downwell::EffectMaskAssets art;
            assert(art.bind(pixels.data(),pixels.size()));
            const downwell::EffectSurfacePlacement::View view{n(160),n(34),n(160),n(284)};
            for(int flash:{-1,0,1})for(unsigned frame=0;frame<8;++frame){
                shard->dflash=flash;assert(shard->imageIndex.assignInteger(frame));
                // Create admits [0,7); use 6.75 for its upper fractional case.
                if(frame==7)shard->imageIndex.bits=0x40d80000u;
                assert(owner->timeShards().publishAnimation(1,405288,*shard));
                assert(owner->timeShards().publishStep(1,405288,*shard));
                assert(list.collect(*scene,pack,pack+8,160,34,false));unsigned seen=0;
                for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==91){
                    assert(list.actors[i].frame==(frame==7?0:frame%6)&&list.actors[i].x==198&&list.actors[i].y==94);++seen;
                }
                assert(seen==unsigned(flash<=0));
                assert(list.collect(*scene,pack,pack+8,160,34,true));
                for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=91);
                unsigned emitted=0;
                assert(scene->visitTimeShardDraw([&](const auto& effect){
                    return downwell::TimeShardDraw::surface(effect,art,view,[&](const auto&,auto,auto){++emitted;return true;});
                })&&emitted==unsigned(flash<=0));
            }
            const auto* published=scene->timeShardForDraw(key);assert(published&&published->dflash==1);
            const auto oldX=published->x.bits,oldFrame=published->imageIndex.bits;
            shard->dflash=0;assert(shard->x.assignInteger(260)&&shard->imageIndex.assignInteger(3));
            assert(scene->timeShardForDraw(key)->x.bits==oldX&&scene->timeShardForDraw(key)->imageIndex.bits==oldFrame);
            assert(list.collect(*scene,pack,pack+8,160,34,false));
            for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=91);
            unsigned emitted=0;
            assert(scene->visitTimeShardDraw([&](const auto& effect){
                return downwell::TimeShardDraw::surface(effect,art,view,[&](const auto&,auto,auto){++emitted;return true;});
            })&&emitted==0);
            assert(owner->timeShards().publishStep(1,405288,*shard));
            assert(list.collect(*scene,pack,pack+8,160,34,false));unsigned seen=0;
            for(unsigned i=0;i<list.count;++i)if(list.actors[i].sprite==91){
                assert(list.actors[i].x==210&&list.actors[i].frame==0);++seen;
            }
            assert(seen==1&&scene->timeShardForDraw(key)->imageIndex.bits==oldFrame);
            assert(owner->timeShards().publishAnimation(1,405288,*shard));
            assert(scene->timeShardForDraw(key)->imageIndex.bits==shard->imageIndex.bits);
            assert(scene->randomDrawCount()==before&&shards.clear());
            assert(owner->prepareDraw()&&!scene->timeShardForDraw(key));
        }
        downwell::DebrisEffects<2> debris;downwell::InstanceHandle first,last;
        assert(scene->spawnDebris(debris,n(240),n(120),1,first)&&
            scene->spawnDebris(debris,n(244),n(124),4,last));
        debris.get(first.generation)->imageIndex.bits=0x40d80000u; // 6.75 -> frame 2
        debris.get(last.generation)->imageIndex.bits=0x3f800000u;
        unsigned ordered=0;
        assert(scene->visitDebrisDraw([&](const auto& effect){
            assert(&effect==debris.retained(ordered?first.generation:last.generation));++ordered;return true;
        })&&ordered==2);
        const auto pixels=read("generated/amiga-game.dwap");downwell::EffectMaskAssets assets;
        assert(assets.bind(pixels.data(),pixels.size()));
        for(bool skeletonShade:{false,true}){
            downwell::InstanceHandle shadeBone;
            assert(skeletonShade?scene->createSkeletonWithSourceId(405199,240,112,shadeBone):
                scene->createBoneWithSourceId(405198,240,112,shadeBone));
            downwell::CurrencySmokeComposition shaded,unshaded;
            const downwell::EffectSurfacePlacement::View view{n(160),n(34),n(160),n(284)};
            const auto currency=[&](auto emit){emit(false,0,n(240),n(112));};
            const auto raster=[](auto value,std::int32_t& out){return value.roundPixel(out);};
            bool enumerated=false;
            assert(unshaded.prepare(currency,scene->smokeEffects(),assets,view,{},[](auto){},raster)&&
                shaded.prepare(currency,scene->smokeEffects(),assets,view,{},[&](auto emit){
                    enumerated=scene->visitRegisteredEnemyShades([&](auto x,auto y){emit(x,y);return true;});
                },raster)&&enumerated);
            unsigned before=0,after=0;
            unshaded.surface().visit([&](auto,auto,auto alpha,auto,auto){for(unsigned b=0;b<16;++b)before+=(alpha>>b)&1u;});
            shaded.surface().visit([&](auto,auto,auto alpha,auto,auto){for(unsigned b=0;b<16;++b)after+=(alpha>>b)&1u;});
            assert(before&&after<before);
            list.entryCoverVisible=true;bool eligible=true;
            assert(list.entryCoverHardwareEligible(*scene,160,34,eligible)&&!eligible);
            list.entryCoverVisible=false;
            assert((skeletonShade?owner->skeletons().markDestroyed(shadeBone):owner->bones().markDestroyed(shadeBone))&&owner->prepareDraw());
        }
        downwell::CurrencySmokeComposition composed;
        const downwell::EffectSurfacePlacement::View view{n(160),n(34),n(160),n(284)};
        auto compose=[&](){return composed.prepareWithLateEffects([](auto){},scene->smokeEffects(),assets,view,{},
            [&](auto emit){return scene->visitDebrisDraw([&](const auto& effect){
                return downwell::DebrisDraw::surface(effect,assets,view,emit);
            });},[](auto){},[](auto coordinate,std::int32_t& pixel){return coordinate.roundPixel(pixel);});};
        const auto beforeDraw=scene->checkpoint();
        assert(compose()&&composed.visible()&&scene->checkpoint()==beforeDraw);
        assert(list.collect(*scene,pack,pack+8,160,34,false));
        unsigned particles=0;
        for(unsigned i=0;i<list.count;++i){const auto& actor=list.actors[i];
            if(actor.sprite==91||actor.sprite==115){
                assert(actor.sprite==(particles?115:91)&&actor.frame==(particles?2:1)&&
                    actor.x==(particles?190:194)&&actor.y==(particles?86:90));++particles;
            }
        }
        assert(particles==2);
        assert(list.collect(*scene,pack,pack+8,160,34,true));
        for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=91&&list.actors[i].sprite!=115);
        debris.get(last.generation)->flash=1;
        assert(list.collect(*scene,pack,pack+8,160,34,false));
        for(unsigned i=0;i<list.count;++i)assert(list.actors[i].sprite!=91);
        assert(debris.clear(owner->debris()));
        assert(compose()&&!composed.visible());
        assert(scene->spawnScriptEffect(n(248),n(128),101,{0x3fe00000u,0},n(1),{},frames,impact));
        assert(!list.collect(*scene,pack,pack+8,160,34)&&list.failedSprite==101);
        // Unknown fractional anchors must not be rounded into a proven variant.
        assert(scene->spawnScriptEffect({0x406e1000u,0},n(124),603,{0x3fe00000u,0},n(270),{},frames,b));
        assert(!list.collect(*scene,pack,pack+8,160,34)&&list.failedSprite==603);
    }
    {
        GameplayWorld scene(0,true,0,false);UBYTE pack[8]{};AmigaGameplayDrawList list;
        auto n=[](int v){return downwell::MotionBinary64::fromInteger(v);};
        assert(scene.emitCorpseExpiry(n(240),n(120)));
        assert(scene.emitDamageNumber(n(240),n(124),n(12),{}));
        assert(list.collect(scene,pack,pack+8,160,34));
        unsigned text=0,expiry=0;
        for(unsigned i=0;i<list.count;++i){
            auto a=list.actors[i];
            if(a.sprite==gameplayDamageTextMarker){assert(!expiry&&a.x==190&&a.y==86&&a.flip==12);++text;}
            if(a.sprite==102){assert(text==1);++expiry;}
        }
        assert(text==1&&expiry==1);
        assert(scene.emitDamageNumber(n(240),n(124),{0x3fe00000u,0},{}));
        assert(!list.collect(scene,pack,pack+8,160,34)&&list.failedSprite==gameplayDamageTextMarker);
    }
    GameplayWorld world(300,true,0,true);
    assert(world.grantUpgrade(12));
    world.registerShop(240,210);
    assert(world.configureEnemy(0,112,169,40,0,0,0,5,14,0,0,-1,-1,0,0,true,true,true));
    UBYTE pack[8]{};
    AmigaGameplayDrawList list;
    unsigned observed=0;
    [[maybe_unused]] unsigned flashDraws=0;
    for(unsigned tick=0;tick<600;++tick) {
        world.step((tick%13==0 ? 4 : 0) | (tick%80<40 ? 1 : 2));
        const int vx=160+world.cameraOffsetX(), vy=34+world.cameraOffsetY();
        if(tick%61==0) assert(world.emitFlashBurst(downwell::MotionBinary64::fromInteger(vx+40),
                                                  downwell::MotionBinary64::fromInteger(vy+60)));
        if(tick%61==0)assert(world.emitCorpseExpiry(downwell::MotionBinary64::fromInteger(vx+50),
                                                  downwell::MotionBinary64::fromInteger(vy+80)));
        const auto checkpoint=world.checkpoint();
        assert(list.collect(world,pack,pack+8,vx,vy));
        assert(world.checkpoint()==checkpoint);
        std::vector<GameplayBob> expected;
        auto emit=[&](int sprite,unsigned frame,int x,int y,int flip=0) {
            if(x+8>110 && x-8<270 && y+8>0 && y-8<284)
                expected.push_back({UWORD(sprite),UWORD(frame),x,y,flip});
        };
#ifndef DOWNWELL_REPLAY_VERIFY
        for(unsigned i=0;i<world.enemyCount();++i) {
            const auto& e=world.enemyAt(i);
            if(e.active && e.awake) emit(e.spriteId,e.drawFrame(),190+e.x-world.cameraOffsetX(),
                142+e.y-world.cameraOffsetY(),e.objectId!=119 && e.facing<0);
        }
#endif
        downwell::drawShopKeepers(world.shops(),[&](int s,unsigned f,int x,int y) { emit(s,f,110+x-vx,y-vy); });
        downwell::drawShopPrompts(world.shops(),false,[&](int s,unsigned f,int x,int y) { emit(s,f,110+x-vx,y-vy); });
#ifndef DOWNWELL_REPLAY_VERIFY
        for(unsigned i=0;i<world.gemSlotEnd();++i) {
            const auto& g=world.gemAt(i);
            if(g.active && g.motion.lifetime.visible()) emit(g.large?134:133,g.motion.animation.frame(),
                190+g.x-world.cameraOffsetX(),142+g.y-world.cameraOffsetY());
        }
#endif
        for(unsigned i=0;i<world.bulletSlotEnd();++i) {
            const auto& b=world.bulletAt(i);
            if(b.active) emit(downwell::machineGunProfile(b.gemHigh).sprite,768+b.animation.frame(),
                190+b.x-world.cameraOffsetX(),142+b.y-world.cameraOffsetY());
        }
        assert(world.corpseExpiryEffects().visitNewest([&](unsigned,const downwell::CorpseExpiryEffect& effect){
            std::int32_t x,y;assert(effect.x.promoted().roundPixel(x)&&effect.y.promoted().roundPixel(y));
            emit(102,effect.drawFrame(),110+x-vx,y-vy);
        }));
        if(world.playerVisible()) emit(world.player().spriteId(world.deathSequence().movementDead()),
            world.player().spriteFrame(world.deathSequence().movementDead(),world.deathSequence().animation),
            190+downwell::roundGameMakerQ8(world.player().offsetXFixed())-world.cameraOffsetX(),
#ifdef DOWNWELL_REPLAY_VERIFY
            200+world.player().offsetY(),
#else
            142+downwell::roundGameMakerQ8(world.player().offsetYFixed())-world.cameraOffsetY(),
#endif
            world.player().facing()<0);
        assert(list.rayVisible==!world.goalStopped());
        if(list.rayVisible) expected.push_back({gameplayRayMarker,0,0,0,0});
#ifndef DOWNWELL_REPLAY_VERIFY
        world.flashEffects().visitNewest([&](const downwell::FlashBallEffect& effect){
            emit(96,effect.drawFrame(),110+effect.pixelX()-vx,effect.pixelY()-vy);++flashDraws;
        });
#endif
        assert(list.count==expected.size());
        int previousDepth=20000;
        for(unsigned i=0;i<list.count;++i){
            const unsigned sprite=list.actors[i].sprite;
            int depth;
            if(sprite==133||sprite==134)depth=10000;
            else if(sprite==458||sprite==459)depth=0;
            else if(sprite==102)depth=-1000;
            else if(sprite==unsigned(world.player().spriteId(world.deathSequence().movementDead())))depth=-50000;
            else if(sprite==96)depth=-200000;
            else continue;
            assert(depth<=previousDepth);previousDepth=depth;
        }
        for(unsigned i=0;i<list.count;++i) {
            const auto& a=list.actors[i]; const auto& b=expected[i];
            assert(a.sprite==b.sprite && a.frame==b.frame && a.x==b.x && a.y==b.y && a.flip==b.flip);
        }
        observed+=list.count;
        assert(list.collect(world,pack,pack+8,vx,vy,true));
        unsigned cursor=0;
        for(const auto& b:expected) {
            if(b.sprite==133 || b.sprite==134)continue;
            assert(cursor<list.count);
            const auto& a=list.actors[cursor++];
            assert(a.sprite==b.sprite && a.frame==b.frame && a.x==b.x && a.y==b.y && a.flip==b.flip);
        }
        assert(cursor==list.count && world.checkpoint()==checkpoint);
    }
#ifndef DOWNWELL_REPLAY_VERIFY
    assert(flashDraws>100);
#endif
    GameplayWorld initial(0);
    boundsOK=false;
    assert(!list.collect(initial,pack,pack+8,160,34) && list.failedSprite!=0);
    boundsOK=true;
    assert(list.collect(initial,pack,pack+8,160,34));
    assert(list.failedSprite==0 && list.failedFrame==0);
    assert(observed>600);
    std::printf("Gameplay collection: 600 ordered world projections, %u commands, no simulation mutation\n",observed);
}
