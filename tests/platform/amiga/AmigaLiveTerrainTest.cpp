#include "platform/amiga/AmigaTerrain.h"
#include "platform/amiga/AmigaPortRuntime.h"
#include "platform/amiga/AmigaWallCollision.h"
#include "core/Replay.h"
#include "core/CurrencyCollisionBounds.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <random>
#include <cstring>

namespace {
void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
using Commands = std::vector<downwell::DeferredCreateInstance>;
void verifyPreparedTerrainRuns(){
    static AmigaTerrainIndex scalar,prepared;
    const AmigaTerrainCollisionContext empty{nullptr,nullptr,nullptr,nullptr,0,nullptr};
    auto compare=[&](int row){
        require(std::equal(std::begin(scalar.cells),std::end(scalar.cells),std::begin(prepared.cells)),
            "prepared run preserves every occupancy cell at each publication");
        require(amigaTerrainContactCount(scalar)==amigaTerrainContactCount(prepared),"prepared contact multiplicity");
        for(unsigned x=0;x<128;++x)for(int y:{0,row,row+1,511})if(y<512)
            require(scalar.nextLanding.next(x,y)==prepared.nextLanding.next(x,y),"prepared landing candidates");
    };
    for(int object:{60,93})for(int step:{-16,16})for(int y:{0,176,8176})for(unsigned count:{1u,7u,128u}){
        require(amigaBuildTerrainIndex(empty,scalar)&&amigaBuildTerrainIndex(empty,prepared),"run indexes initialize");
        const int x=step>0?-512:1520;
        // Overlapping room walls must retain their own occupancy flag.
        require(amigaAddTerrainObject(scalar,60,x,y,true)&&amigaAddTerrainObject(prepared,60,x,y,true),"run overlap fixture");
        AmigaTerrainRunIndex run(prepared,object,x,y,step,count);
        require(run.ready(),"valid run admission");
        for(unsigned cell=0;cell<count;++cell){
            require(amigaAddTerrainObject(scalar,object,x+int(cell)*step,y)&&run.append(),"ordered run cell");
            compare(y/16);
        }
        require(!run.ready()&&!run.append(),"exhausted run cannot publish twice");
        // Whole-span publication matches the per-cell prefix's final state.
        static AmigaTerrainIndex span;
        static AmigaTerrainTiles scalarTiles,spanTiles;
        scalarTiles.clear();spanTiles.clear();
        require(amigaBuildTerrainIndex(empty,span)&&amigaAddTerrainObject(span,60,x,y,true),"span fixture");
        for(unsigned cell=0;cell<count;++cell)
            require(scalarTiles.append(downwell::DeferredCreateInstance{object,x+int(cell)*step,y}),"scalar tiles");
        AmigaTerrainRunIndex whole(span,object,x,y,step,count);
        require(whole.admitSpan(spanTiles)&&whole.publishSpan(spanTiles)&&!whole.ready(),"span publishes");
        require(std::equal(std::begin(span.cells),std::end(span.cells),std::begin(scalar.cells))&&
                !std::memcmp(&span.nextLanding,&scalar.nextLanding,sizeof(AmigaLandingIndex))&&
                span.extraContacts.size()==scalar.extraContacts.size(),"span equals per-cell publication");
        require(spanTiles.size()==scalarTiles.size(),"span tiles");
        for(unsigned id=1;id<=spanTiles.size();++id)
            require(spanTiles.at(id).x==scalarTiles.at(id).x&&spanTiles.at(id).frame==scalarTiles.at(id).frame&&
                    spanTiles.at(id).next==scalarTiles.at(id).next,"span tile order");
        require(spanTiles.first(y)==scalarTiles.first(y),"span tile row");
    }
    require(amigaBuildTerrainIndex(empty,scalar)&&amigaBuildTerrainIndex(empty,prepared),"capacity fixtures initialize");
    // Saturate the run cells' own counts, then fill all but two overflow records.
    for(int x=160;x<224;x+=16)for(unsigned i=0;i<3;++i)
        require(amigaAddTerrainObject(scalar,60,x,192)&&amigaAddTerrainObject(prepared,60,x,192),"saturate run cells");
    for(unsigned i=0;i<3+254;++i)
        require(amigaAddTerrainObject(scalar,60,240,176)&&amigaAddTerrainObject(prepared,60,240,176),"fill contact prefix");
    AmigaTerrainRunIndex limited(prepared,60,160,192,16,4);
    {
        static AmigaTerrainTiles tiles;tiles.clear();
        require(!limited.admitSpan(tiles),"span admission rejects a run that would exhaust overflow");
        AmigaTerrainRunIndex fits(prepared,60,160,192,16,2);
        require(fits.admitSpan(tiles),"span admission accepts a run within overflow capacity");
    }
    for(unsigned i=0;i<3;++i){
        require(amigaAddTerrainObject(scalar,60,160+int(i)*16,192)==limited.append(),"run fails on same contact-capacity cell");
        compare(12);
    }
    for(auto args:std::vector<std::array<int,5>>{{60,-528,0,16,2},{60,1520,0,16,2},
        {60,-512,0,-16,2},{93,160,-16,16,2},{93,160,8192,16,2},{60,161,0,16,2},
        {92,160,0,16,2},{60,160,0,0,2},{60,160,0,16,0}}){
        AmigaTerrainRunIndex invalid(prepared,args[0],args[1],args[2],args[3],unsigned(args[4]));
        require(!invalid.ready()&&!invalid.append(),"unsupported run has no publication");
        compare(12);
    }
}
void verifyWallCollisionLifecycle() {
    static AmigaTerrainIndex index;
    AmigaTerrainCollisionContext context{nullptr,nullptr,nullptr,nullptr,0,nullptr};
    require(amigaBuildTerrainIndex(context,index),"lifecycle cache initializes");
    AmigaWallCollision cache{index};
    struct Presentation {unsigned calls=0;bool changed(int,int,int,int){++calls;return true;}} presentation;
    using Store=downwell::GameplayTerrainInstances;
    struct Lifecycle {
        using H=downwell::InstanceHandle;using R=Store::Record;using V=downwell::RunnerImageValue;
        bool birth(H,const R&){return true;}bool retire(H,const R&){return true;}
        bool activation(H,const R&,bool){return true;}bool membership(H,const R&,bool){return true;}
        bool move(H,const R&,V,V){return true;}bool sprite(H,const R&,int){return true;}
        bool imageIndex(H,const R&,V){return true;}bool markDestroyed(H,const R&){return true;}
    } lifecycle;
    const auto binding=Store::Binding::bindDestruction(lifecycle);
    Store store(binding,Store::Presentation::bind(presentation),Store::WallCollision::bind(cache));
    require(amigaAddTerrainObject(index,60,240,176) && store.append(60,240,176,true),"live wall birth");
    const auto h=store.handle(0);
    require(store.activation(h,false) && !amigaIndexedWall(index,240,176),"deactivation excludes cache immediately");
    require(store.activation(h,false) && store.activation(h,true) && store.activation(h,true) &&
            amigaTerrainContactCount(index)==1,"idempotent activation does not duplicate contributions");
    downwell::GameMakerRandom random;
    require(store.createProperties(h,51,false,random),"wall Create properties");
    struct Ops {
        Store& store;downwell::InstanceHandle h;AmigaTerrainIndex& index;Presentation& presentation;
        bool outOfView(){return true;}
        bool tile(int,int,int,int,int,int,int,int){return true;}
        bool place(int,int,int,bool& hit){hit=true;return true;}
        bool point(int,int,int,bool& hit){hit=true;return true;}
        bool image(int frame){return store.imageIndex(h,frame);}
        bool create(int,int,int){return true;}
        bool deactivate(){return store.activation(h,false);}
        bool destroy(){
            const unsigned before=presentation.calls;
            require(store.requestDestroy(h) && !amigaIndexedWall(index,240,176),
                    "destroy excludes cache before End Step returns");
            require(store.find(h) && store.find(h)->destroyRequested && presentation.calls==before,
                    "marked lifetime survives while visual notification is still coalesced");
            return true;
        }
    } ops{store,h,index,presentation};
    require(store.wallEvent(h,1,random,ops) && store.wallEvent(h,1,random,ops),"surrounded wall two-event lifetime");
    require(store.requestDestroy(h) && store.activation(h,false) && store.activation(h,true) &&
            amigaTerrainContactCount(index)==0,"destroyed wall never reenters cache");
    require(store.retireMarked(h) && !store.find(h),"cleanup retires identity without removing cache twice");
    require(store.append(60,256,192,false),"inactive birth has no cache contribution");
    const auto inactive=store.handle(0);
    require(store.activation(inactive,true) && amigaIndexedWall(index,256,192),"inactive birth enters cache upon activation");
    Store failed(binding, {}, Store::WallCollision::bind(cache));
    require(failed.append(60,272,208,true),"missing contribution fixture");
    require(!failed.requestDestroy(failed.handle(0)) && !failed.valid(),"missing contribution is terminal, not a silent miss");
}
// Cell counts plus overflow records must report the same multiplicity as one
// record per Create, under coincident births, removals and box retirement.
void verifyContactMultiplicity() {
    static AmigaTerrainIndex index;
    static downwell::TerrainWallContacts<4096> reference;
    AmigaTerrainCollisionContext context{nullptr,nullptr,nullptr,nullptr,0,nullptr};
    require(amigaBuildTerrainIndex(context,index),"multiplicity index initializes");
    context.index=&index;reference.clear();
    AmigaPortRuntime runtime(0);
    const int boxX[]={208-240,240-240,272-240},boxY[]={160-176,176-176,208-176};
    runtime.configureBreakables(boxX,boxY,3);
    std::mt19937 random(5);
    bool overflowed=false;
    std::vector<std::pair<int,int>> platforms;
    auto position=[&](int& x,int& y){x=-512+16*int(random()%5)+(random()%4?720:0);y=16*int(random()%4)+(random()%8?160:0);};
    for(unsigned step=0;step<20000;++step){
        int x,y;position(x,y);
        // Alternate growth and decay so cells cross the overflow threshold both ways.
        const bool room=index.extraContacts.size()<240,growing=step%4000<2000;
        const unsigned op=random()%1000,walls=room?(growing?450:250):0;
        if(op<walls){
            require(amigaAddTerrainObject(index,60,x,y)&&reference.append(60,x,y),"coincident wall birth");
        }else if(op<walls+5&&room){
            require(amigaAddTerrainObject(index,92,x,y)&&reference.append(92,x,y),"coincident box birth");
        }else if(op<walls+10&&room){
            require(amigaAddTerrainObject(index,293,x,y)&&reference.append(293,x,y),"object 293 birth");
        }else if(op<walls+30){
            require(amigaAddTerrainObject(index,93,x,y),"platforms have no contact");
            platforms.emplace_back(x,y);
        }else if(op<800){
            require(amigaRemoveTerrainWall(index,x,y)==reference.removeOne(60,x,y),"wall removal agrees");
            require(amigaIndexedWall(index,x,y)==reference.contains(60,x,y),"wall occupancy follows multiplicity");
        }else{
            const int left=x-12+int(random()%24),top=y-12+int(random()%24);
            const downwell::ContactBounds query{left,left+int(random()%40),top,top+int(random()%40)};
            context.runtime=random()%2?&runtime:nullptr;
            unsigned expected=0,actual=0;
            require(reference.visit(query,[&](unsigned object,int bx,int by){
                return object!=92||!context.runtime||context.runtime->breakableActiveAt(bx-240,by-176);
            },[&]{++expected;return true;}),"reference query");
            require(amigaResolveTerrainWallContacts(&context,query,actual)&&actual==expected,
                "cell counts report every overlapping instance");
            for(int sprite:{133,134}){
                auto gx=downwell::MotionBinary64::fromInteger(left),gy=downwell::MotionBinary64::fromInteger(top);
                downwell::ContactBounds gem;
                require(gx.add({0x3fe00000u,0})&&gy.add({0x3fd00000u,0})&&
                    downwell::CurrencyCollisionBounds::calculate(sprite,gx,gy,gem),"fractional currency mask bounds");
                expected=actual=0;
                require(reference.visit(gem,[&](unsigned object,int bx,int by){
                    return object!=92||!context.runtime||context.runtime->breakableActiveAt(bx-240,by-176);
                },[&]{++expected;return true;})&&amigaResolveTerrainWallContacts(&context,gem,actual)&&actual==expected,
                    "currency queries retain coincident walls and box retirement");
                bool expectedPlatform=false,actualPlatform=false;
                for(auto p:platforms)expectedPlatform|=downwell::contactBoundsOverlap(gem,{p.first-8,p.first+8,p.second-8,p.second-6});
                require(amigaResolveTerrainPlatformContact(&context,gem,actualPlatform)&&actualPlatform==expectedPlatform,
                    "fractional currency contacts use the thin platform mask, retaining shared wall cells");
            }
        }
        for(int object:{60,92,293})
            require(amigaTerrainContacts(index,object,x,y)==reference.count(object,x,y),"per-position multiplicity");
        overflowed|=reference.count(60,x,y)>3;
    }
    require(amigaTerrainContactCount(index)==reference.size(),"total multiplicity");
    require(overflowed,"differential reaches the overflow records");
}
void verifyWallRemoval() {
    static AmigaTerrainIndex index;
    AmigaTerrainCollisionContext context{nullptr,nullptr,nullptr,nullptr,0,nullptr};
    context.index=&index;
    for(unsigned overlap=0;overlap<4;++overlap){
        require(amigaBuildTerrainIndex(context,index),"removal index initializes");
        require(amigaAddTerrainObject(index,60,240,176,true) &&
                amigaAddTerrainObject(index,60,240,176),"coincident wall births");
        if(overlap&1)require(amigaAddTerrainObject(index,93,240,176),"overlapping platform");
        if(overlap&2)require(amigaAddTerrainObject(index,92,240,176),"overlapping box");
        require(amigaAddTerrainObject(index,60,240,192),"next landing candidate");
        const unsigned column=(240-AmigaTerrainIndex::minX)/16;
        index.mutableWallOccupancy=false;
        require(!amigaRemoveTerrainWall(index,240,176) && amigaTerrainContacts(index,60,240,176)==2,
                "reconstructed occupancy cannot be subtracted");
        index.mutableWallOccupancy=true;
        require(!amigaRemoveTerrainWall(index,241,176) && !amigaRemoveTerrainWall(index,240,-16),
                "invalid removal is rejected");
        require(amigaRemoveTerrainWall(index,240,176) && amigaIndexedWall(index,240,176) &&
                index.nextLanding.next(column,11)==12,"first removal preserves duplicate occupancy");
        require(amigaRemoveTerrainWall(index,240,176) && !amigaIndexedWall(index,240,176) &&
                !amigaIndexedRoomWall(index,240,176),"last wall removes wall occupancy");
        require(index.nextLanding.next(column,11)==(overlap?12u:13u),
                "landing removal preserves overlapping platforms and boxes");
        require(amigaResolveTerrainSolid(&context,0,0)==bool(overlap&2),
                "only overlapping box remains solid after walls disappear");
        require(!amigaRemoveTerrainWall(index,240,176),"absent wall does not consume another object");
        require(amigaAddTerrainObject(index,60,240,176) && amigaIndexedWall(index,240,176) &&
                index.nextLanding.next(column,11)==12,"reactivation restores contact and landing");
    }
}
void verifyTerrainSegments() {
    static AmigaTerrainIndex index;
    AmigaTerrainCollisionContext context{nullptr, nullptr, nullptr, nullptr, 0, nullptr};
    require(amigaBuildTerrainIndex(context, index), "segment test index initializes");
    context.index = &index;
    require(amigaAddTerrainObject(index, 92, 240, 176) &&
            amigaAddTerrainObject(index, 60, 256, 192), "segment test tiles register");
    require(amigaAddTerrainObject(index,60,256,192),"duplicate wall Create is retained separately");
    unsigned contacts=0;
    require(amigaResolveTerrainWallContacts(&context,{247,250,183,186},contacts) && contacts==3,
        "native query reports box and both overlapping wall instances");
    index.wallContactsComplete=false;
    require(!amigaResolveTerrainWallContacts(&context,{247,250,183,186},contacts),
        "reconstructed occupancy cannot masquerade as actual collision instances");
    index.wallContactsComplete=true;
    AmigaPortRuntime runtime(0);
    int bx = 0, by = 0;
    runtime.configureBreakables(&bx, &by, 1);
    constexpr int unit = downwell::MachineGunTrajectory::unit;
    for (unsigned state = 0; state < 3; ++state) {
        if (state) context.runtime = &runtime;
        if (state == 2) runtime.configureBreakables(&bx, &by, 0);
        require(amigaResolveTerrainWallContacts(&context,{247,250,183,186},contacts) && contacts==(state==2?2u:3u),
            "native contact query filters destroyed boxes without merging walls");
        for (int x = -40; x <= 40; ++x) for (int y = -40; y <= 40; ++y) {
            const bool corners = amigaResolveTerrainSolid(&context, x-4, y-4) ||
                amigaResolveTerrainSolid(&context, x+3, y-4) ||
                amigaResolveTerrainSolid(&context, x-4, y+7) ||
                amigaResolveTerrainSolid(&context, x+3, y+7);
            require(amigaResolveTerrainPlayerMask(&context, x, y) == corners,
                    "batched player mask equals four point queries for every cell alignment");
        }
        for (int x = -24; x <= 40; x += 4) for (int y = -24; y <= 40; y += 4)
            for (unsigned fraction : {0u, 1u, 524288u, 1048575u})
                for (int dx : {-8*unit, -unit/3, 0, unit/3, 8*unit})
                    for (int dy : {-8*unit, -unit/3, 0, unit/3, 8*unit}) {
                        const downwell::ProjectileSegment path({x,fraction}, {y,fraction}, dx, dy);
                        const int left = std::min(x, path.endX.pixel), right = std::max(x, path.endX.pixel);
                        const int top = std::min(y, path.endY.pixel), bottom = std::max(y, path.endY.pixel);
                        bool pixels = false;
                        for (int py = top; py <= bottom; ++py)
                            for (int px = left; px <= right; ++px)
                                pixels |= amigaResolveTerrainSolid(&context, px, py) && path.intersects(px,px+1,py,py+1);
                        require(amigaResolveTerrainSegment(&context, path) == pixels,
                                "indexed segment equals pixel queries including inactive breakables");
                    }
    }
}
struct ShopCreates {
    downwell::ShopWorld shops;
    downwell::GameMakerRandom& random;
    Commands commands;
    static bool create(void* opaque, const downwell::DeferredCreateInstance& command) {
        auto& self = *static_cast<ShopCreates*>(opaque);
        self.commands.push_back(command);
        if (command.objectId != 277) return true;
        return self.shops.create(command.x, command.y,
            [&](unsigned lo, unsigned hi) { return lo + self.random.irandomSmall(hi-lo); },
            [&] { return self.random.chooseIndexSmall(3); },
            4, 1, false, 4, false) != nullptr;
    }
};
bool collect(void* context, const downwell::DeferredCreateInstance& command) {
    static_cast<Commands*>(context)->push_back(command);
    return true;
}
struct LiveCollision {
    AmigaTerrainIndex index;
    AmigaTerrainTiles tiles;
    unsigned creates = 0;
    static bool create(void* opaque, const downwell::DeferredCreateInstance& command) {
        auto& self = *static_cast<LiveCollision*>(opaque);
        ++self.creates;
        return amigaAddTerrainObject(self.index, command.objectId, command.x, command.y) &&
               self.tiles.append(command);
    }
};

// Exercise the complete native simulation/generator/collision wiring without
// relying on the executable's still-forecast-based renderer.
struct LiveSimulation {
    AmigaTerrainFragment fragments[19];
    AmigaTerrainEnemy enemies[64];
    AmigaTerrainDecoration decorations[128];
    AmigaTerrainIndex index;
    AmigaAreaOneTerrainBuilder builder{fragments, 19, enemies, 64, decorations, 128};
    AmigaTerrainCollisionContext collision;
    AmigaPortRuntime* runtime = nullptr;
    const downwell::BuilderPatternPack& patterns;
    unsigned generationTicks = 0;

    explicit LiveSimulation(const downwell::BuilderPatternPack& pack)
        : collision{nullptr, nullptr, &pack, nullptr, 0, nullptr}, patterns(pack) {}

    static bool create(void* opaque, const downwell::DeferredCreateInstance& command) {
        auto& world = *static_cast<LiveSimulation*>(opaque);
        if (!amigaAddTerrainObject(world.index, command.objectId, command.x, command.y))
            return false;
        if (command.objectId == 92) {
            const int x = command.x - 240, y = command.y - 176;
            if (!world.runtime->appendBreakables(&x, &y, 1)) return false;
        }
        if (amigaEnemyHealth(command.objectId))
            return world.runtime->configureEnemy(world.runtime->enemyCount(), command.objectId,
                0, command.x - 240, command.y - 176,
                command.integerVelocityX, command.integerVelocityY,
                amigaEnemyHealth(command.objectId), amigaEnemyReward(command.objectId),
                command.motionState, command.auxiliaryState,
                command.alarms.get(0), command.alarms.get(1),
                command.integerDirection, command.integerMoveSpeed,
                command.active, command.grounded, true);
        return true;
    }
    static bool generate(void* opaque, AmigaPortRuntime& runtime,
                         downwell::GameMakerRandom& random, int bottom) {
        auto& world = *static_cast<LiveSimulation*>(opaque);
        require(world.runtime == &runtime, "generation receives bound runtime");
        ++world.generationTicks;
        downwell::AreaOneBuilderStep result;
        return world.builder.stepVisible(world.patterns, random, bottom, result);
    }
    void bind(AmigaPortRuntime& port) {
        runtime = &port;
        require(amigaBuildTerrainIndex(collision, index), "live simulation starts with empty generated terrain");
        collision.index = &index;
        collision.runtime = &port;
        downwell::GameMakerRandom random;
        builder.resetInitialCheckpoint(random);
        builder.setCreateObserver(create, this);
        port.configureEnemyRandom(random);
        port.configureGenerationStep(generate, this);
        port.configureLandingResolver(amigaResolveTerrainLanding, &collision);
        port.configureSolidResolver(amigaResolveTerrainSolid, &collision,
                                    amigaResolveTerrainSegment, amigaResolveTerrainPlayerMask);
    }
};
void compareRandom(const downwell::GameMakerRandom& a,
                   const downwell::GameMakerRandom& b) {
    require(a.drawCount() == b.drawCount() && a.stateIndex() == b.stateIndex(),
            "native generator consumes the same number of RNG draws");
    for (unsigned i = 0; i < 16; ++i)
        require(a.stateWord(i) == b.stateWord(i), "complete RNG state matches");
}

void checkSchedule(const downwell::BuilderPatternPack& patterns, bool interleave) {
    AmigaTerrainFragment fragments[19];
    AmigaTerrainEnemy enemies[64];
    AmigaTerrainDecoration decorations[128];
    AmigaAreaOneTerrainBuilder native(fragments, 19, enemies, 64, decorations, 128);
    LiveCollision collision;
    AmigaTerrainCollisionContext collisionContext{nullptr, nullptr, &patterns, nullptr, 0, nullptr};
    require(amigaBuildTerrainIndex(collisionContext, collision.index), "empty live collision map initializes");
    collisionContext.index = &collision.index;
    native.setCreateObserver(LiveCollision::create, &collision);
    std::array<bool, AmigaTerrainIndex::columns * AmigaTerrainIndex::rows> solid{}, landing{};
    downwell::GameMakerRandom nativeRandom, referenceRandom(2);
    native.resetInitialCheckpoint(nativeRandom);
    for (unsigned i = 0; i < 331; ++i) referenceRandom.nextU32();
    downwell::AreaOneBuilder reference;
    reference.restoreInitialCheckpoint(-16, 704, -30, 8, 3);
    Commands commands;
    unsigned ticks = 0, enemyCount = 0, decorationCount = 0;
    while (!native.endReached() && ticks++ < 128) {
        downwell::AreaOneBuilderStep actual, expected;
        const unsigned oldCount = native.fragmentCount();
        const auto oldCreates = collision.creates;
        require(native.stepVisible(patterns, nativeRandom, native.frontierY() - 128, actual),
                "invisible tick succeeds");
        require(native.fragmentCount() == oldCount && !actual.patternGenerated &&
                    native.frontierY() == reference.y() && collision.creates == oldCreates,
                "invisible tick leaves terrain unchanged");
        compareRandom(nativeRandom, referenceRandom);
        const auto commandStart = commands.size();
        require(reference.step(patterns, referenceRandom, collect, &commands, expected) &&
                    native.stepVisible(patterns, nativeRandom, native.frontierY() - 127, actual),
                "visible tick advances both builders");
        require(native.frontierY() == reference.y() && actual.advancedRows == expected.advancedRows &&
                    actual.patternGenerated == expected.patternGenerated,
                "native frontier advances by exactly the recovered event");
        if (expected.patternGenerated) {
            require(native.fragmentCount() == oldCount + 1, "one new fragment emitted");
            const auto& fragment = fragments[oldCount];
            require(fragment.pattern.family == expected.family &&
                        fragment.pattern.sourceIndex == expected.patternIndex &&
                        fragment.y == expected.patternY && fragment.reversed == expected.reversed &&
                        fragment.sideRoom == expected.sideRoom &&
                        fragment.appendPillars == expected.appendPillars,
                    "native fragment preserves recovered metadata");
        }
        for (auto i = commandStart; i < commands.size(); ++i) {
            const auto& command = commands[i];
            if (command.objectId == 60 || command.objectId == 92 || command.objectId == 93) {
                const unsigned cell = (command.y / 16) * AmigaTerrainIndex::columns +
                    (command.x - AmigaTerrainIndex::minX) / 16;
                require(cell < solid.size(), "reference Create lies in collision storage");
                landing[cell] = true;
                if (command.objectId != 93) solid[cell] = true;
            }
            if (command.objectId == 111 || command.objectId == 112 || command.objectId == 113 ||
                command.objectId == 116 || command.objectId == 117 || command.objectId == 119) {
                require(enemyCount < native.enemyCount(), "enemy exists on its generation tick");
                const auto& enemy = enemies[enemyCount++];
                require(enemy.objectId == command.objectId && enemy.x == command.x - 240 &&
                            enemy.y == command.y - 176 && enemy.speedX == command.integerVelocityX &&
                            enemy.speedY == command.integerVelocityY &&
                            enemy.direction == command.integerDirection &&
                            enemy.moveSpeed == command.integerMoveSpeed &&
                            enemy.motionState == command.motionState &&
                            enemy.auxiliaryState == command.auxiliaryState &&
                            enemy.alarm0 == command.alarms.get(0) && enemy.alarm1 == command.alarms.get(1) &&
                            enemy.grounded == command.grounded && enemy.active == command.active,
                        "native enemy retains exact integer Create state and alarms");
            }
            if (command.objectId == 289 || command.objectId == 293 || command.objectId == 296 ||
                command.objectId == 109) {
                require(decorationCount < native.decorationCount(), "decoration exists on creation tick");
                const auto& decoration = decorations[decorationCount++];
                require(decoration.objectId == command.objectId && decoration.x == command.x &&
                            decoration.y == command.y && decoration.frame == command.integerImageIndex &&
                            decoration.flipX == (command.integerScaleX < 0),
                        "native decoration retains its recovered location and appearance");
            }
        }
        require(enemyCount == native.enemyCount() && decorationCount == native.decorationCount(),
                "no future actors or decorations created early");
        require(collision.creates == commands.size(), "every recovered Create is observed exactly once");
        // Independent full-grid oracle from actual Create events. In
        // particular, unchosen '2'/'3' cells must remain absent.
        for (unsigned column = 0; column < AmigaTerrainIndex::columns; ++column) {
            unsigned next = 0;
            for (int row = AmigaTerrainIndex::rows - 1; row >= 0; --row) {
                const unsigned cell = row * AmigaTerrainIndex::columns + column;
                if (landing[cell]) next = row + 1;
                require(collision.index.nextLanding[cell] == next,
                        "incremental links equal full recomputation after each generation event");
                require(amigaResolveTerrainSolid(&collisionContext,
                            column * 16 + AmigaTerrainIndex::minX - 240, row * 16 - 176) == solid[cell],
                        "only actually emitted terrain is solid at each generation tick");
            }
        }
        compareRandom(nativeRandom, referenceRandom);
        if (interleave && expected.patternGenerated) {
            nativeRandom.irandomSmall(180);
            referenceRandom.irandomSmall(180);
        }
    }
    require(native.endReached() && reference.endReached() && native.fragmentCount() == 19,
            "complete native generation ends with nineteen fragments");
    if (!interleave) {
        AmigaTerrainFragment forecast[19];
        Commands forecastCommands;
        downwell::GameMakerRandom forecastRandom;
        unsigned forecastEnemies = 0;
        require(amigaBuildInitialAreaOneTerrain(patterns, forecast, 19,
                    nullptr, 0, forecastEnemies, &forecastRandom,
                    nullptr, 0, nullptr, collect, &forecastCommands),
                "complete forecast succeeds with current source-backed RNG semantics");
        compareRandom(forecastRandom, referenceRandom);
        require(forecastCommands.size() == commands.size(),
                "forecast emits every incremental Create exactly once");
        for (unsigned i = 0; i < commands.size(); ++i) {
            const auto& a = forecastCommands[i];
            const auto& b = commands[i];
            require(a.objectId == b.objectId && a.x == b.x && a.y == b.y &&
                    a.scaleX == b.scaleX && a.scaleY == b.scaleY &&
                    a.imageSpeed == b.imageSpeed && a.imageIndex == b.imageIndex &&
                    a.velocityX == b.velocityX && a.velocityY == b.velocityY &&
                    a.direction == b.direction && a.moveSpeed == b.moveSpeed &&
                    a.motionInitialized == b.motionInitialized &&
                    a.integerImageIndex == b.integerImageIndex &&
                    a.integerScaleX == b.integerScaleX &&
                    a.integerVelocityX == b.integerVelocityX &&
                    a.integerVelocityY == b.integerVelocityY &&
                    a.integerDirection == b.integerDirection &&
                    a.integerMoveSpeed == b.integerMoveSpeed &&
                    a.motionState == b.motionState && a.auxiliaryState == b.auxiliaryState &&
                    a.grounded == b.grounded && a.active == b.active,
                    "forecast preserves ordered Create data");
            for (unsigned alarm = 0; alarm < 12; ++alarm)
                require(a.alarms.get(alarm) == b.alarms.get(alarm),
                        "forecast preserves Create alarm state");
        }
        for (unsigned i = 0; i < 19; ++i)
            require(forecast[i].pattern.family == fragments[i].pattern.family &&
                    forecast[i].pattern.sourceIndex == fragments[i].pattern.sourceIndex &&
                    forecast[i].y == fragments[i].y &&
                    forecast[i].reversed == fragments[i].reversed &&
                    forecast[i].sideRoom == fragments[i].sideRoom &&
                    forecast[i].appendPillars == fragments[i].appendPillars,
                    "forecast preserves every incremental fragment");
    }
    unsigned tileCount = 0;
    unsigned blocks = 0;
    for (const auto& command : commands) if (command.objectId == 92) ++blocks;
    std::cout << "Generated breakables (interleaved=" << interleave << "): " << blocks << '\n';
    for (int y = 0; y < AmigaTerrainTiles::rows * 16; y += 16) {
        unsigned id = collision.tiles.first(y);
        for (const auto& command : commands) {
            if (command.y != y || (command.objectId != 92 && command.objectId != 93)) continue;
            require(id != 0, "each emitted visible tile appears in its row list");
            const auto& tile = collision.tiles.at(id);
            require(tile.objectId == command.objectId && tile.x == command.x &&
                        tile.frame == (command.objectId == 93 ? 23 : command.integerImageIndex),
                    "tile rendering preserves object order and recovered Create frame");
            id = tile.next;
            ++tileCount;
        }
        require(id == 0, "row list has no uncreated probabilistic tiles");
    }
    require(tileCount == collision.tiles.size() && tileCount > 0,
            "row lists account for every stored tile exactly once");
    for (const auto& command : commands)
        if (command.objectId == 60)
            require(amigaIndexedWall(collision.index, command.x, command.y),
                    "generated walls are available for neighbor-based tile rendering");
    native.resetInitialCheckpoint(nativeRandom);
    require(native.frontierY() == 704 && native.fragmentCount() == 0 &&
                native.enemyCount() == 0 && native.decorationCount() == 0 &&
                nativeRandom.drawCount() == 331 && !native.failed(),
            "restart resets generator and counts without stale records becoming live");
}
}

int main(int argc, char** argv) {
    verifyPreparedTerrainRuns();
    verifyWallCollisionLifecycle();
    verifyContactMultiplicity();
    verifyWallRemoval();
    verifyTerrainSegments();
    require(argc == 2, "builder pack path required");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    downwell::BuilderPatternPack patterns(bytes.data(), bytes.size());
    require(patterns.valid(), "builder pack validates");
    for (unsigned seed = 1; seed <= 16; ++seed) {
        downwell::GameMakerRandom hostRandom(seed), nativeRandom(seed);
        downwell::AreaOneBuilder host;
        AmigaTerrainFragment fragments[32];
        AmigaAreaOneTerrainBuilder native(fragments, 32);
        const downwell::AreaOneBuilder::Setup setup{1,4,false,true};
        host.reset(240, 704, hostRandom, setup);
        native.resetRoom(240, 704, nativeRandom, setup);
        ShopCreates hostCreates{{},hostRandom,{}}, nativeCreates{{},nativeRandom,{}};
        native.setCreateObserver(ShopCreates::create, &nativeCreates);
        if(seed&1)native.setTerrainRunObserver([](void* p,const downwell::AreaOneBuilder::TerrainRun& run){
            return run.execute(ShopCreates::create,p);
        });
        for (unsigned tick = 0; tick < 100 && !host.endReached(); ++tick) {
            downwell::AreaOneBuilderStep a, b;
            require(host.stepVisible(patterns, hostRandom, 100000, ShopCreates::create, &hostCreates, a) &&
                native.stepVisible(patterns, nativeRandom, 100000, b), "generated shop paired traversal");
            compareRandom(hostRandom, nativeRandom);
            require(host.y() == native.frontierY() && a.patternGenerated == b.patternGenerated &&
                a.family == b.family && a.patternIndex == b.patternIndex && a.reversed == b.reversed,
                "shop Create RNG preserves subsequent pattern selection and frontier");
        }
        require(host.endReached() && native.endReached() && hostCreates.shops.allocatedCount == 1 &&
            nativeCreates.shops.allocatedCount == 1, "actual generated shop registered once on both paths");
        require(hostCreates.commands.size() == nativeCreates.commands.size(), "paired create stream length");
        for (unsigned i = 0; i < hostCreates.commands.size(); ++i) {
            const auto& a = hostCreates.commands[i]; const auto& b = nativeCreates.commands[i];
            require(a.objectId == b.objectId && a.x == b.x && a.y == b.y,
                "terrain and enemy creates remain aligned after stock random draws");
        }
        const auto& a = hostCreates.shops.instances[0]; const auto& b = nativeCreates.shops.instances[0];
        require(a.x == b.x && a.y == b.y && a.farewellVariant == b.farewellVariant,
            "generated shop location and farewell match");
        for (unsigned i = 0; i < 3; ++i)
            require(a.stock.items[i].upgrade == b.stock.items[i].upgrade &&
                a.stock.items[i].price == b.stock.items[i].price &&
                a.stock.items[i].remaining == b.stock.items[i].remaining,
                "generated stock and style-discounted prices match");
    }
    checkSchedule(patterns, false);
    checkSchedule(patterns, true);
    AmigaTerrainTiles boundedTiles;
    downwell::DeferredCreateInstance tileCommand(92, 160, 704);
    tileCommand.integerImageIndex = 22;
    for (unsigned i = 0; i < AmigaTerrainTiles::capacity; ++i)
        require(boundedTiles.append(tileCommand), "tile storage accepts its full capacity");
    require(!boundedTiles.append(tileCommand) && boundedTiles.size() == AmigaTerrainTiles::capacity,
            "tile overflow is reported without changing the list");
    boundedTiles.clear();
    require(boundedTiles.size() == 0 && boundedTiles.first(704) == 0 &&
                boundedTiles.first(-16) == 0 && boundedTiles.first(8192) == 0,
            "tile reset discards old row links and bounds queries");
    LiveSimulation live(patterns);
    AmigaPortRuntime runtime(300, false, 4096 - 176, true);
    live.bind(runtime);
    for (unsigned tick = 0; tick < 300; ++tick) {
        runtime.step(0);
        require(runtime.generationSucceeded() && runtime.enemyCount() == live.builder.enemyCount(),
                "native runtime creates only the enemies emitted by its live generator");
    }
    require(live.generationTicks == 300 && live.builder.fragmentCount() > 0 &&
                live.builder.fragmentCount() < 19 && runtime.enemyCount() > 0 &&
                runtime.player().grounded(),
            "live simulation generates only the visible prefix and lands on generated terrain");
    {
        LiveSimulation indexed(patterns), pixels(patterns);
        AmigaPortRuntime fast(300, false, 4096-176, true), reference(300, false, 4096-176, true);
        indexed.bind(fast); pixels.bind(reference);
        reference.configureSolidResolver(amigaResolveTerrainSolid, &pixels.collision);
        for (unsigned tick = 0; tick < 600; ++tick) {
            downwell::Input input;
            input.fire = tick % 23 < 17;
            input.left = tick % 180 < 35;
            input.right = tick % 180 >= 90 && tick % 180 < 125;
            const auto packed = downwell::packReplayInput(input);
            fast.step(packed); reference.step(packed);
            require(fast.checkpoint() == reference.checkpoint() &&
                    fast.trajectoryDigest() == reference.trajectoryDigest() &&
                    fast.blocksDestroyed() == reference.blocksDestroyed(),
                    "indexed bullet terrain preserves live simulation checkpoints");
            compareRandom(fast.randomState(), reference.randomState());
        }
        require(fast.player().shots() > 0, "indexed/pixel live comparison exercises firing");
    }
    LiveCollision boundary;
    {
        AmigaTerrainIndex shuffled{};
        bool occupied[4][AmigaTerrainIndex::rows]{};
        for (unsigned insertion = 0; insertion < 128; ++insertion) {
            const unsigned column = insertion % 4;
            const unsigned row = (insertion * 197) % AmigaTerrainIndex::rows;
            const int x = AmigaTerrainIndex::minX + column*42*16;
            require(amigaAddTerrainObject(shuffled, 93, x, row*16),
                "shuffled landing insertion succeeds");
            occupied[column][row] = true;
            require(amigaAddTerrainObject(shuffled, 93, x, row*16),
                "duplicate shuffled landing insertion succeeds");
            for (unsigned c = 0; c < 4; ++c) {
                unsigned next = 0;
                for (int y = AmigaTerrainIndex::rows-1; y >= 0; --y) {
                    if (occupied[c][y]) next = y+1;
                    require(shuffled.nextLanding[y*AmigaTerrainIndex::columns+c*42] == next,
                        "each shuffled insertion preserves nearest landing in every row");
                }
            }
        }
    }
    AmigaTerrainCollisionContext empty{nullptr, nullptr, &patterns, nullptr, 0, nullptr};
    require(amigaBuildTerrainIndex(empty, boundary.index), "boundary map initializes");
    require(amigaResolveTerrainLanding(&empty, 0, 4096 - 176) == downwell::gameplayNoLanding,
            "empty fragment store does not manufacture a floor at world Y4096");
    auto indexedEmpty = empty;
    indexedEmpty.index = &boundary.index;
    require(amigaResolveTerrainLanding(&indexedEmpty, 0, 4096 - 176) == downwell::gameplayNoLanding &&
                amigaResolveTerrainLanding(&indexedEmpty, 0, 8192) == downwell::gameplayNoLanding &&
                amigaResolveTerrainLanding(&indexedEmpty, 0, 300000) == downwell::gameplayNoLanding,
            "empty indexed queries including beyond the last row return no landing safely");
    require(amigaAddTerrainObject(boundary.index, 60, 240, 5008) &&
                amigaResolveTerrainLanding(&indexedEmpty, 0, 4096 - 176) == 5008 - 192,
            "real terrain below world Y4096 remains eligible for landing");
    require(!amigaAddTerrainObject(boundary.index, 60, -528, 0) &&
                !amigaAddTerrainObject(boundary.index, 60, 1536, 0) &&
                !amigaAddTerrainObject(boundary.index, 60, 160, -16) &&
                !amigaAddTerrainObject(boundary.index, 60, 160, 8192) &&
                !amigaAddTerrainObject(boundary.index, 60, 161, 0),
            "invalid terrain coordinates are rejected before writing");
    require(amigaAddTerrainObject(boundary.index, 60, -512, 0, true) &&
                amigaIndexedRoomWall(boundary.index, -512, 0) &&
                amigaAddTerrainObject(boundary.index, 93, 1520, 8176),
            "edge cells and initial-room wall identity are supported");
    const auto snapshot = boundary.index;
    for (int extreme : {std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max() - 15}) {
        require(!amigaAddTerrainObject(boundary.index, 60, extreme, 0) &&
                !amigaAddTerrainObject(boundary.index, 93, 0, extreme),
                "aligned extreme coordinates reject without signed overflow");
    }
    require(amigaAddTerrainObject(boundary.index, 60, -512, 0, true) &&
                amigaAddTerrainObject(boundary.index, 113, 5, -9),
            "duplicate terrain and non-terrain Create events are safe");
    // Bits 6-7 count the duplicate's contact; occupancy flags stay unchanged.
    for (unsigned cell = 0; cell < sizeof(snapshot.cells); ++cell)
        require((snapshot.cells[cell] & 63) == (boundary.index.cells[cell] & 63) &&
                    snapshot.nextLanding[cell] == boundary.index.nextLanding[cell],
                "duplicates do not disturb the collision map");
    require(amigaTerrainContacts(boundary.index, 60, -512, 0) == 2,
            "duplicate wall keeps its own contact");
    // Each capacity failure is terminal until reset, including a failure after
    // some objects in the current fragment have already been emitted.
    for (unsigned limit = 0; limit < 3; ++limit) {
        AmigaTerrainFragment fragments[19];
        AmigaTerrainEnemy enemies[64];
        AmigaTerrainDecoration decorations[128];
        AmigaAreaOneTerrainBuilder builder(fragments, limit == 0 ? 0 : 19,
            enemies, limit == 1 ? 0 : 64, decorations, limit == 2 ? 0 : 128);
        downwell::GameMakerRandom random;
        builder.resetInitialCheckpoint(random);
        downwell::AreaOneBuilderStep step;
        unsigned ticks = 0;
        while (!builder.failed() && ticks++ < 128)
            builder.stepVisible(patterns, random, builder.frontierY() - 127, step);
        require(builder.failed(), "capacity overflow is detected");
        const auto afterFailure = random;
        const unsigned count = builder.enemyCount();
        require(!builder.stepVisible(patterns, random, 100000, step) &&
                    builder.enemyCount() == count && !step.patternGenerated,
                "partial event cannot be silently retried");
        compareRandom(random, afterFailure);
        builder.resetInitialCheckpoint(random);
        require(!builder.failed() && builder.enemyCount() == 0, "explicit reset clears failure");
    }
    std::cout << "Amiga incremental terrain generation tests passed\n";
}
