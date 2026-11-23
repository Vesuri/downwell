#include "core/AreaOneBuilder.h"
#include "core/BuilderRewardChoice.h"
#include "core/CavernEnemyChoice.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
using namespace downwell;
using Commands = std::vector<DeferredCreateInstance>;

// Hash fields, not structure padding; pin ordered Create output plus RNG state.
std::uint64_t emissionDigest = 14695981039346656037ULL;
std::uint64_t legacyDigest = 14695981039346656037ULL;
std::uint64_t preBoxDigest = 14695981039346656037ULL;
unsigned initialBorderPairs = 0;
unsigned emptyPreparedSegments = 0;
void mix(std::uint32_t value,bool newFieldPayload=false,bool boxPayload=false) {
    auto legacy=(newFieldPayload||boxPayload)?0u:value;
    auto preBox=boxPayload?0u:value;
    for (unsigned byte = 0; byte < 4; ++byte) {
        emissionDigest = (emissionDigest ^ (value & 255u)) * 1099511628211ULL;
        legacyDigest = (legacyDigest ^ (legacy & 255u)) * 1099511628211ULL;
        preBoxDigest = (preBoxDigest ^ (preBox & 255u)) * 1099511628211ULL;
        value >>= 8;
        legacy >>= 8;
        preBox >>= 8;
    }
}
void capture(const Commands& commands, const GameMakerRandom& random) {
    mix(static_cast<std::uint32_t>(commands.size()));
    for (const auto& c : commands) {
        for (std::int32_t value : {c.objectId, c.x, c.y, c.integerScaleX,
             c.integerImageIndex, c.integerVelocityX, c.integerVelocityY,
             c.integerDirection, c.integerMoveSpeed, c.motionState}) mix(value);
        mix(c.auxiliaryState,c.objectId==82,c.objectId==92);
        for (float value : {c.scaleX, c.scaleY, c.imageSpeed, c.imageIndex,
             c.velocityX, c.velocityY, c.direction, c.moveSpeed}) {
            std::uint32_t bits;
            static_assert(sizeof bits == sizeof value, "32-bit float fixture");
            std::memcpy(&bits, &value, sizeof bits);
            mix(bits);
        }
        mix(c.motionInitialized); mix(c.grounded); mix(c.active);
        for (unsigned alarm = 0; alarm < AlarmBank::count; ++alarm) mix(c.alarms.get(alarm));
    }
    mix(random.stateIndex());
    mix(static_cast<std::uint32_t>(random.drawCount()));
    mix(static_cast<std::uint32_t>(random.drawCount() >> 32));
    for (unsigned word = 0; word < 16; ++word) mix(random.stateWord(word));
}

void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}

bool collect(void* context, const DeferredCreateInstance& command) {
    static_cast<Commands*>(context)->push_back(command);
    return true;
}

bool sameRandom(const GameMakerRandom& a, const GameMakerRandom& b) {
    if (a.drawCount() != b.drawCount() || a.stateIndex() != b.stateIndex())
        return false;
    for (unsigned i = 0; i < 16; ++i)
        if (a.stateWord(i) != b.stateWord(i)) return false;
    return true;
}

bool sameCommand(const DeferredCreateInstance& a, const DeferredCreateInstance& b) {
    if (a.objectId != b.objectId || a.x != b.x || a.y != b.y ||
        a.scaleX != b.scaleX || a.scaleY != b.scaleY ||
        a.integerScaleX != b.integerScaleX || a.imageSpeed != b.imageSpeed ||
        a.imageIndex != b.imageIndex || a.integerImageIndex != b.integerImageIndex ||
        a.velocityX != b.velocityX || a.velocityY != b.velocityY ||
        a.integerVelocityX != b.integerVelocityX || a.integerVelocityY != b.integerVelocityY ||
        a.direction != b.direction || a.integerDirection != b.integerDirection ||
        a.moveSpeed != b.moveSpeed || a.integerMoveSpeed != b.integerMoveSpeed ||
        a.motionState != b.motionState || a.auxiliaryState != b.auxiliaryState ||
        a.motionInitialized != b.motionInitialized || a.grounded != b.grounded ||
        a.active != b.active) return false;
    for (unsigned i = 0; i < AlarmBank::count; ++i)
        if (a.alarms.get(i) != b.alarms.get(i)) return false;
    return true;
}
}

int main(int argc, char** argv) {
    // Independent floating-point transcription of the normal-mode VM branch.
    for (unsigned level = 1; level <= 3; ++level)
        for (unsigned seed = 0; seed < 128; ++seed) {
            GameMakerRandom actual(seed), expected(seed);
            for (unsigned repeat = 0; repeat < 32; ++repeat)
                for (char symbol : {'a', 'b', 'c', 'd', 'e', 't', '%', '.', '1'}) {
                    int enemy = -1;
                    if ((expected.nextU32() / 4294967296.0) * (level + 2) <= level) {
                        switch (symbol) {
                        case 'a': enemy = 111; break;
                        case 'b': enemy = 116; break;
                        case 'c': if (level >= 2 && expected.nextU32() % 2) enemy = 119; break;
                        case 'd': enemy = 113; break;
                        case 'e': enemy = 117; break;
                        case 't': enemy = 112; break;
                        case '%': {
                            const int choices[] = {113, 111, 119, 112};
                            enemy = choices[expected.nextU32() % 4]; break;
                        }
                        default: break;
                        }
                    }
                    require(cavernEnemyChoice(symbol, level, actual) == enemy &&
                            sameRandom(actual, expected), "cavern enemy rules preserve VM values and RNG order");
                }
        }
    struct BoundaryRandom {
        std::uint32_t word;
        unsigned draws = 0;
        std::uint32_t nextU32() { ++draws; return word; }
        unsigned chooseIndexSmall(unsigned n) { return nextU32() % n; }
    };
    for (unsigned level = 1; level <= 3; ++level) {
        const std::uint32_t cutoff = static_cast<std::uint32_t>(4294967296.0 * level / (level + 2));
        for (std::uint32_t word : {0U, cutoff - 1, cutoff, cutoff + 1, 0xffffffffU}) {
            BoundaryRandom random{word};
            const bool allowed = (word / 4294967296.0) * (level + 2) <= level;
            require(cavernEnemyChoice('a', level, random) == (allowed ? 111 : -1) && random.draws == 1,
                    "inclusive cavern probability boundaries match runner doubles");
        }
    }
    for (unsigned seed = 0; seed < 128; ++seed) {
        AreaOneBuilder builder;
        GameMakerRandom actual(seed), expected(seed);
        builder.reset(240, 704, actual, {2, 0, false, false});
        const unsigned count = expected.nextU32() % 3 == 2 ? 3 : 2;
        int rooms[3] = {};
        for (unsigned i = 0; i < count; ++i) {
            unsigned rejected = 0;
            for (;;) {
                rooms[i] = 3 + static_cast<int>(expected.irandom(13));
                bool conflict = false;
                for (unsigned j = 0; j < i; ++j)
                    if (std::abs(rooms[i] - rooms[j]) <= 2) conflict = true;
                if (!conflict) break;
                if (++rejected > 100) { rooms[i] = 9999; break; }
            }
        }
        require(builder.rightRoomCount() == count && sameRandom(actual, expected),
                "setup mixes one-word room-count choice with two-word position irandom");
        for (unsigned i = 0; i < count; ++i)
            require(builder.rightRoom(i) == rooms[i], "source-order room positions match");
    }
    for (unsigned seed = 0; seed < 128; ++seed)
        for (int health : {-1, 0, 1, 2, 3, 4})
            for (unsigned style = 0; style < 5; ++style) {
                GameMakerRandom actual(seed), expected(seed);
                int a = 0, b = 0;
                for (unsigned item = 0; item < 16; ++item) {
                    // Original Windows choose is a single unsigned word modulo argc.
                    if (!b) b = expected.nextU32() % 2 ? 109 : 293;
                    else b = b == 109 ? 293 : 109;
                    if (health <= 2 && b != 109) {
                        const unsigned choices[] = {0, 1, 1, 1, 1, 1};
                        if (choices[expected.nextU32() % 6]) b = 109;
                    }
                    if (style == 1) b = 109;
                    a = builderRewardChoice(a, health, style, actual);
                    require(a == b && actual.drawCount() == expected.drawCount() &&
                        actual.stateIndex() == expected.stateIndex(), "reward choice preserves VM branch and RNG ordering");
                    for (unsigned i = 0; i < 16; ++i)
                        require(actual.stateWord(i) == expected.stateWord(i), "reward choice preserves all RNG words");
                }
            }
    require(argc == 2, "builder pack path required");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    BuilderPatternPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "builder pack validates");
    // Each source newline writes self.y after its two pillars, before any
    // child of the next row. Prepared terrain must keep those same boundaries.
    struct RowPublication {
        AreaOneBuilder* builder;
        int published=704;
        unsigned writes=0,failAt=0;
        std::vector<int> tape;
        static bool position(void* opaque,int x,int y){
            auto& self=*static_cast<RowPublication*>(opaque);
            require(x==-16&&self.builder->y()==self.published&&y==self.published+16,
                "source row setter observes the previous builder position");
            self.tape.insert(self.tape.end(),{-1,x,y});
            if(++self.writes==self.failAt)return false;
            self.published=y;return true;
        }
        static bool create(void* opaque,const DeferredCreateInstance& child){
            auto& self=*static_cast<RowPublication*>(opaque);
            require(self.builder->y()==self.published,
                "synchronous child sees the last published builder row");
            if(child.objectId==60||child.objectId==93)
                require(child.y==self.published,"terrain birth sees its source builder row");
            self.tape.insert(self.tape.end(),{child.objectId,child.x,child.y});
            return true;
        }
    };
    for(unsigned seed:{2u,4u,8u}){
        std::vector<int> expected;
        GameMakerRandom expectedRandom;
        for(unsigned mode=0;mode<3;++mode){
            AreaOneBuilder builder;
            GameMakerRandom random(seed);
            builder.reset(-16,704,random);
            RowPublication publication{&builder};
            builder.setPositionPublisher(RowPublication::position,&publication);
            builder.enableTerrainRuns(mode==1);
            if(mode==2)builder.setPreparedTerrainEmitter([](void* p,const AreaOneBuilder::PreparedTerrain& row,
                AreaOneBuilder::EmitFunction emit){return row.executeReference(emit,p);});
            for(unsigned tick=0;tick<128&&!builder.endReached();++tick){
                AreaOneBuilderStep result;
                require(builder.step(pack,random,RowPublication::create,&publication,result),
                    "row-published generation reaches every synchronous child");
            }
            require(builder.endReached()&&publication.writes==builder.rowsGenerated(),
                "one source setter per advanced row including end padding");
            if(!mode){expected=publication.tape;expectedRandom=random;}
            else require(expected==publication.tape&&sameRandom(expectedRandom,random),
                "scalar/run/prepared paths preserve interleaved setters, births and RNG");
        }
    }
    {
        AreaOneBuilder builder;
        GameMakerRandom random(2);
        builder.reset(-16,704,random);
        RowPublication publication{&builder};publication.failAt=1;
        builder.setPositionPublisher(RowPublication::position,&publication);
        AreaOneBuilderStep result;
        require(!builder.step(pack,random,RowPublication::create,&publication,result)&&
            builder.y()==704&&builder.rowsGenerated()==0&&publication.tape.size()==9&&
            publication.tape[6]==-1,
            "failed row publication stops after pillars without advancing or later births");
    }
    unsigned runs=0,runCells=0;
    for(unsigned seed=1;seed<=8;++seed)for(unsigned level=1;level<=3;++level)
    for(unsigned executor:{0u,1u,2u,3u,4u})for(unsigned failAt:{0u,1u,2u,17u,101u,701u}){
        AreaOneBuilder scalar,grouped;GameMakerRandom a(seed),b(seed);
        scalar.reset(240,704,a,{level,seed%5,true,false});
        grouped.reset(240,704,b,{level,seed%5,true,false});
        scalar.enableTerrainRuns(false);
        grouped.enableTerrainRuns(true);
        struct RunProbe {
            GameMakerRandom& random;
            Commands commands;
            std::vector<GameMakerRandom> observations;
            unsigned count=0,failAt,runs=0,cells=0;
            unsigned executor=0;
            const AreaOneBuilder::TerrainRun* active=nullptr;
            AreaOneBuilder::EmitFunction callback=nullptr;
        } left{a,{}, {},0,failAt},right{b,{}, {},0,failAt};
        right.executor=executor;
        const auto emit=[](void* context,const DeferredCreateInstance& command){
            auto& p=*static_cast<RunProbe*>(context);
            p.commands.push_back(command);p.observations.push_back(p.random);
            ++p.count;
            if(p.active){
                const auto before=p.random;
                require(!p.active->execute(nullptr,nullptr)&&
                    !p.active->executeReference(nullptr,nullptr)&&
                    !p.active->executeEach([](const auto&){return true;})&&sameRandom(before,p.random),
                    "all executors reject reentry before consuming RNG");
            }
            for(unsigned i=0;i<p.count%4;++i)p.random.nextU32();
            return !p.failAt||p.count!=p.failAt;
        };
        grouped.setTerrainRunEmitter([](void* context,const AreaOneBuilder::TerrainRun& run,AreaOneBuilder::EmitFunction emit){
            auto& p=*static_cast<RunProbe*>(context);++p.runs;p.cells+=run.count;
            require(run.count>1&&(run.objectId==60||run.objectId==93)&&
                (run.stepX==16||run.stepX==-16),"admitted terrain run geometry");
            p.active=&run;
            const bool ok=p.executor==0?run.executeReference(emit,context):p.executor==1?run.execute(emit,context):
                run.executeEach([&](const auto& command){return emit(context,command);});
            p.active=nullptr;
            require(run.completed()==ok,"run completion tracks callback failure");
            const auto after=p.random;const unsigned calls=p.count;
            require(!run.execute(emit,context)&&!run.executeReference(emit,context)&&
                !run.executeEach([&](const auto& command){return emit(context,command);})&&
                sameRandom(after,p.random)&&calls==p.count,
                "run cannot be replayed after completion or failure");
            return ok;
        });
        if(executor==4){
            right.callback=emit;
            grouped.setBreakableEmitter([](void* context,int x,int y,const GeneratedBreakableCreate& create){
                auto& p=*static_cast<RunProbe*>(context);
                DeferredCreateInstance command{92,x,y};
                command.imageSpeed=0;
                command.integerImageIndex=create.frame;command.imageIndex=float(create.frame);
                command.setBoxCreate(create.sound,create.gems);
                return p.callback(context,command);
            },&right);
        }
        if(executor>=3)grouped.setPreparedTerrainEmitter([](void* context,const AreaOneBuilder::PreparedTerrain& batch,
                                                          AreaOneBuilder::EmitFunction emit){
            auto& p=*static_cast<RunProbe*>(context);++p.runs;p.cells+=batch.count();
            if(batch.draws()==0&&batch.count()==2&&batch.creates()[0]==42&&batch.creates()[2]==52){
                ++initialBorderPairs;
                const std::uint8_t words[]={42,1,83,0,52,1,83,0};
                require(batch.wordCount()==2&&batch.platforms()==0&&
                    std::memcmp(batch.words(),words,sizeof words)==0,
                    "initial border pair has exact occupancy, contacts and landing bits");
            }
            const auto beforeCalls=p.count;
            auto expectedRandom=p.random;
            if(!batch.count())expectedRandom.advance(batch.draws());
            const bool ok=batch.executeReference(emit,context);
            if(!batch.count()){
                ++emptyPreparedSegments;
                require(ok&&batch.completed()&&p.count==beforeCalls&&sameRandom(expectedRandom,p.random),
                    "empty prepared segment consumes exactly its RNG words without a Create callback");
            }
            require(batch.completed()==ok,"prepared segment completion tracks callback failure");
            const auto after=p.random;const unsigned calls=p.count;
            require(!batch.executeReference(emit,context)&&!batch.executeSpan([]{return true;})&&!batch.beginPublication()&&
                    sameRandom(after,p.random)&&calls==p.count,"prepared segment cannot be replayed");
            return ok;
        });
        bool terminated=false;
        for(unsigned tick=0;tick<128&&!terminated;++tick){
            for(unsigned j=0;j<tick%5;++j){a.nextU32();b.nextU32();}
            scalar.setPlayerHealth(1+tick%6);grouped.setPlayerHealth(1+tick%6);
            AreaOneBuilderStep x{},y{};
            const bool ok=scalar.stepVisible(pack,a,tick%3?100000:scalar.y()-128,emit,&left,x);
            require(grouped.stepVisible(pack,b,tick%3?100000:grouped.y()-128,emit,&right,y)==ok&&
                sameRandom(a,b)&&left.commands.size()==right.commands.size(),"terrain run preserves terminal status/RNG/emission count");
            for(unsigned j=0;j<left.commands.size();++j)
                require(sameCommand(left.commands[j],right.commands[j])&&sameRandom(left.observations[j],right.observations[j]),
                    "terrain run preserves command and full RNG state at EACH Create");
            require(scalar.y()==grouped.y()&&scalar.rowsGenerated()==grouped.rowsGenerated()&&
                scalar.generationCount()==grouped.generationCount()&&scalar.chunkOrCrate()==grouped.chunkOrCrate()&&
                scalar.specialObjectsGenerated()==grouped.specialObjectsGenerated()&&
                x.patternGenerated==y.patternGenerated&&x.patternIndex==y.patternIndex&&
                x.family==y.family&&x.patternY==y.patternY&&x.reversed==y.reversed&&
                x.sideRoom==y.sideRoom&&x.appendPillars==y.appendPillars&&x.advancedRows==y.advancedRows,
                "terrain run preserves partial builder state");
            left.commands.clear();right.commands.clear();left.observations.clear();right.observations.clear();
            terminated=!ok||scalar.endReached();
        }
        require(terminated,"terrain run tape bounded");runs+=right.runs;runCells+=right.cells;
    }
    require(runs>100&&runCells>runs*2,"terrain run corpus covers repeated wall/platform spans");
    require(initialBorderPairs>0,"prepared initial border pair path exercised");
    require(emptyPreparedSegments>0,"RNG-only prepared segments exercised");
    std::cout<<"ordered terrain runs "<<runs<<", candidate cells "<<runCells<<'\n';
    {
        AreaOneBuilder invalid;GameMakerRandom random(1);
        invalid.reset(240,704,random,{1,0,true,false});invalid.enableTerrainRuns(true);
        unsigned ignoredRuns=0;
        invalid.setTerrainRunEmitter([](void* context,const AreaOneBuilder::TerrainRun&,AreaOneBuilder::EmitFunction){
            ++*static_cast<unsigned*>(context);return true; // Invalid: never executes the run.
        });
        bool ok=true;
        for(unsigned tick=0;tick<128&&ok;++tick){
            AreaOneBuilderStep step;
            ok=invalid.stepVisible(pack,random,100000,
                [](void*,const DeferredCreateInstance&){return true;},&ignoredRuns,step);
        }
        require(!ok&&ignoredRuns==1,"a run emitter cannot silently drop the span");
    }
    // Preparation must not move shared RNG or health-dependent decisions to
    // level entry. Pin live interleaving, including the state at failed Create.
    // This is an immediate-executor regression tape, not a Windows capture.
    unsigned interleavedCallbacks=0,failedCreates=0,finishedLevels=0;
    for(unsigned seed=1;seed<=8;++seed)for(unsigned level=1;level<=3;++level)
    for(unsigned style: {0u,1u,4u})for(unsigned failAt: {0u,1u,17u,101u,701u}){
        AreaOneBuilder live;GameMakerRandom rng(seed);
        live.reset(240,704,rng,{level,style,true,false});
        struct Probe {
            GameMakerRandom& random;
            GameMakerRandom afterCallback;
            Commands commands;
            unsigned count=0,failAt;
        } probe{rng,rng,{},0,failAt};
        auto callback=[](void* opaque,const DeferredCreateInstance& command){
            auto& p=*static_cast<Probe*>(opaque);
            p.commands.push_back(command);++p.count;
            // Child Create effects happen before the next cell, even when
            // the callback ultimately fails after partially applying them.
            for(unsigned i=0;i<p.count%4;++i)p.random.nextU32();
            p.afterCallback=p.random;
            return !p.failAt || p.count!=p.failAt;
        };
        bool terminated=false;
        for(unsigned tick=0;tick<128 && !terminated;++tick){
            for(unsigned i=0;i<tick%5;++i)rng.nextU32();
            live.setPlayerHealth(1+tick%6);
            const bool visible=tick%3!=0;
            const auto before=rng;
            const int beforeY=live.y();
            AreaOneBuilderStep event{};
            const bool ok=live.stepVisible(pack,rng,visible?100000:live.y()-128,callback,&probe,event);
            if(!visible)require(ok && probe.commands.empty() && sameRandom(before,rng) &&
                live.y()==beforeY && !event.patternGenerated,"interleaved invisible event is a no-op");
            capture(probe.commands,rng);
            for(auto value: {int(ok),live.y(),live.generationCount(),int(live.rowsGenerated()),
                int(live.endReached()),live.chunkOrCrate(),int(live.specialObjectsGenerated()),
                int(live.rightRoomsMade()),int(live.shopMade()),int(live.spawnerMade()),
                int(event.patternGenerated),int(event.family),int(event.patternIndex),event.patternY,
                int(event.reversed),int(event.sideRoom),int(event.appendPillars),int(event.advancedRows)})mix(value);
            interleavedCallbacks+=probe.commands.size();probe.commands.clear();
            if(!ok){
                require(failAt && probe.count==failAt && sameRandom(rng,probe.afterCallback),
                    "failed callback stops all subsequent emission and RNG draws");
                ++failedCreates;terminated=true;
            }else if(live.endReached()){
                require(!failAt,"failure tape must reach its requested callback");
                ++finishedLevels;terminated=true;
            }
        }
        require(terminated,"interleaved tape terminates within its bound");
    }
    std::cout<<"interleaved callbacks "<<interleavedCallbacks<<", failures "<<failedCreates
        <<", levels "<<finishedLevels<<", digest "<<std::hex<<emissionDigest<<std::dec<<'\n';
    // 9e683b15 (W1.4): GunModule draws irandom(bulletMaxNum) until it differs
    // from the current gun, which moves every later draw of a tape.
    require(interleavedCallbacks==148293 && failedCreates==288 && finishedLevels==72,
        "interleaved corpus covers all successful levels and failure boundaries");
    // Row setters now remain visible on a later failed Create, instead of
    // leaving y at the beginning of the pattern. Successful emission/RNG
    // fingerprints below remain unchanged. The GunModule draws (9e683b15)
    // moved this digest with the callback count.
    require(emissionDigest==0x18187d2f2693b492ULL,
        "live RNG/health interleaving preserves ordered commands and partial builder state");
    emissionDigest=legacyDigest=preBoxDigest=14695981039346656037ULL;
    unsigned fieldCount=0;
    for(unsigned seed=1;seed<=32;++seed){
        AreaOneBuilder builder;GameMakerRandom actual(seed),shadow(seed);
        builder.reset(240,704,actual,{2,4,true,false});
        struct Probe {GameMakerRandom& actual;GameMakerRandom& shadow;unsigned& fields;} probe{actual,shadow,fieldCount};
        AreaOneBuilderStep result;
        for(unsigned tick=0;tick<100 && !builder.endReached();++tick)
            require(builder.stepVisible(pack,actual,100000,[](void* opaque,const DeferredCreateInstance& command){
                auto& p=*static_cast<Probe*>(opaque);
                if(command.objectId!=82)return true;
                require(p.actual.drawCount()>p.shadow.drawCount(),"field Create has a new RNG draw");
                while(p.shadow.drawCount()+1<p.actual.drawCount())p.shadow.nextU32();
                require(command.timeFieldRadiusWord()==p.shadow.nextU32(),"field command preserves exactly its source Create word");
                require(sameRandom(p.actual,p.shadow),"field capture preserves complete source random stream");
                ++p.fields;return true;
            },&probe,result),"field capture builder traversal");
        require(builder.endReached(),"field capture traverses whole level");
    }
    require(fieldCount>0,"natural builder patterns exercise ordinary time fields");
    std::cout << "retained time-field Create words " << fieldCount << '\n';
    for (unsigned seed = 1; seed <= 32; ++seed) {
        AreaOneBuilder configured;
        GameMakerRandom configuredRandom(seed);
        configured.reset(240, 704, configuredRandom, {2,4,true,false});
        require(configured.generationCount() == -10 && !configured.shopMade() &&
            configured.rightRoomCount() >= 3 && configured.rightRoomCount() <= 4 &&
            configured.rightRoom(0) == 0, "later-stage member setup retains extra first side room");
        for (unsigned i = 1; i < configured.rightRoomCount(); ++i) {
            require(configured.rightRoom(i) >= 3 && configured.rightRoom(i) <= 16,
                "stage-two side room range uses levelLength minus one");
            for (unsigned j = 0; j < i; ++j)
                require(std::abs(configured.rightRoom(i) - configured.rightRoom(j)) > 2,
                    "all side-room indices obey source separation");
        }
        Commands emitted;
        AreaOneBuilderStep configuredStep;
        for (unsigned step = 0; step < 100 && !configured.endReached(); ++step)
            require(configured.stepVisible(pack, configuredRandom, 100000, collect, &emitted, configuredStep),
                "configured shop traversal succeeds");
        unsigned shops = 0;
        for (const auto& command : emitted) if (command.objectId == 277) ++shops;
        require(configured.endReached() && configured.shopsGenerated() == 1 && shops == 1,
            "member source branch emits exactly one actual Shop object through pattern traversal");
        capture(emitted, configuredRandom);
        configuredRandom.setSeed(seed);
        configured.reset(240, 704, configuredRandom, {1,4,false,true});
        emitted.clear();
        for (unsigned step = 0; step < 100 && !configured.endReached(); ++step) {
            require(configured.stepVisible(pack, configuredRandom, 100000, collect, &emitted, configuredStep),
                "style-four nonmember traversal succeeds");
            if (configured.shopsGenerated())
                require(configured.rightRoomsMade() == configured.rightRoomCount(),
                    "style four postpones the shop until the last side room");
        }
        require(configured.endReached() && configured.shopsGenerated() == 1,
            "style four permits exactly one shop even on first level");
        capture(emitted, configuredRandom);
    }
    std::cout << "ordered emission digest " << std::hex << emissionDigest << std::dec << '\n';
    // Re-recorded after VM-audited level-dependent enemy spawning; the previous
    // 1fd36f0408435e8b kept level-one odds and omitted crawler cells. This is a regression
    // fingerprint, not an independent Windows-runner capture. The GunModule
    // draws (9e683b15) moved it from 970b28efcbf63992.
    require(legacyDigest == 0x68ce296493063d05ULL,
        "retaining field radius changes no other Create fields or RNG state");
    // Both moved with the GunModule draws (9e683b15): from ddd99e417bca728f
    // and 5c6663997c7bbf14.
    require(preBoxDigest == 0x4d752d652f32584dULL,
        "retaining box Create choices changes no prior emission fields or RNG state");
    require(emissionDigest == 0xaad735cb40293659ULL,
        "64 seeded levels retain ordered box sound/gem choices and complete RNG state");
    GameMakerRandom random(2);
    AreaOneBuilder builder;
    builder.reset(240, 704, random);
    const GameMakerRandom before = random;
    Commands commands;
    AreaOneBuilderStep result;
    result.patternGenerated = true;
    result.advancedRows = 99;
    for (int bottom : {0, 575, 576}) {
        require(builder.stepVisible(pack, random, bottom, collect, &commands, result),
                "invisible builder step succeeds");
        require(builder.y() == 704 && builder.generationCount() == -30 &&
                    commands.empty() && sameRandom(random, before) &&
                    !result.patternGenerated && result.advancedRows == 0,
                "strict visibility boundary is a complete no-op and clears result");
    }
    // ceil(576.01) == 577: fractional camera motion across the boundary counts.
    require(builder.stepVisible(pack, random, 577, collect, &commands, result),
            "fractional boundary crossing generates a row");
    require(builder.y() == 720 && builder.generationCount() == -29 &&
                commands.size() == 2 && random.drawCount() == before.drawCount() + 20,
            "prelude advances one row with its two walls and exact RNG cost");
    require(builder.stepVisible(pack, random, 100000, collect, &commands, result) &&
                builder.y() == 736 && builder.generationCount() == -28 &&
                commands.size() == 4,
            "camera jump never catches up multiple builder events in one tick");

    AreaOneBuilder direct, incremental;
    GameMakerRandom directRandom(2), incrementalRandom(2);
    direct.reset(240, 704, directRandom);
    incremental.reset(240, 704, incrementalRandom);
    Commands directCommands, incrementalCommands;
    unsigned ticks = 0;
    while (!direct.endReached() && ticks++ < 128) {
        const auto pausedRandom = incrementalRandom;
        const auto pausedCount = incrementalCommands.size();
        require(incremental.stepVisible(pack, incrementalRandom, incremental.y() - 128,
                    collect, &incrementalCommands, result) &&
                    sameRandom(pausedRandom, incrementalRandom) &&
                    pausedCount == incrementalCommands.size(),
                "waiting between fragments neither emits nor consumes random draws");
        AreaOneBuilderStep expected;
        require(direct.step(pack, directRandom, collect, &directCommands, expected) &&
                    incremental.stepVisible(pack, incrementalRandom, incremental.y() - 127,
                        collect, &incrementalCommands, result),
                "direct and visible single events succeed");
        require(result.patternGenerated == expected.patternGenerated &&
                    result.family == expected.family && result.patternIndex == expected.patternIndex &&
                    result.patternY == expected.patternY && result.reversed == expected.reversed &&
                    result.sideRoom == expected.sideRoom && result.appendPillars == expected.appendPillars &&
                    result.advancedRows == expected.advancedRows &&
                    incremental.y() == direct.y() && sameRandom(directRandom, incrementalRandom),
                "visible event preserves pattern metadata and complete RNG state");
    }
    require(direct.endReached() && incremental.endReached() &&
                directCommands.size() == incrementalCommands.size(),
            "both schedules finish the complete level");
    bool emittedTorch = false;
    for (const auto& command : directCommands)
        emittedTorch = emittedTorch || command.objectId == 296;
    require(emittedTorch, "standalone builder tape retains I/J torch emission coverage");
    unsigned plainWalls = 0, plainPlatforms = 0, crates = 0;
    for (const auto& command : directCommands) {
        if (command.objectId == 92) {
            DeferredCreateInstance expected(92, command.x, command.y);
            require(command.integerImageIndex == 16 || command.integerImageIndex == 17 ||
                        command.integerImageIndex == 22,
                    "crate frame remains one of its source Create choices");
            expected.imageSpeed = 0.0F;
            const auto box=command.boxCreate();
            require(box.breakSound>=171&&box.breakSound<=174&&
                box.haveGem==(command.integerImageIndex==22?2:0)&&box.wallHp.high==0x3ff00000u&&
                !box.wallHp.low&&!box.hit&&!box.drilled&&!box.autoTile&&!box.checked,
                "crate command retains its source Create fields without repeating RNG");
            expected.setBoxCreate(box.breakSound,box.haveGem);
            expected.integerImageIndex = command.integerImageIndex;
#ifdef DOWNWELL_FREESTANDING
            expected.imageIndex = command.integerImageIndex == 22 ? 22.0F : 0.0F;
#else
            expected.imageIndex = static_cast<float>(command.integerImageIndex);
#endif
            require(sameCommand(command, expected),
                    "crate commands retain fresh non-presentation fields and alarms");
            ++crates;
            continue;
        }
        if (command.objectId != 60 && command.objectId != 93) continue;
        const DeferredCreateInstance expected(command.objectId, command.x, command.y);
        require(sameCommand(command, expected),
            "reused plain terrain commands retain fresh defaults including all alarms");
        if (command.objectId == 60) ++plainWalls; else ++plainPlatforms;
    }
    require(plainWalls > 100 && plainPlatforms > 0 && crates > 0,
        "default-field checks cover walls, platforms and crates interleaved with actors");
    for (std::size_t i = 0; i < directCommands.size(); ++i)
        require(sameCommand(directCommands[i], incrementalCommands[i]),
                "ordered emissions preserve every Create-event field including alarms");
    const auto finishedRandom = incrementalRandom;
    const auto finishedCount = incrementalCommands.size();
    require(incremental.stepVisible(pack, incrementalRandom, 100000, collect,
                &incrementalCommands, result) && sameRandom(finishedRandom, incrementalRandom) &&
                finishedCount == incrementalCommands.size() && !result.patternGenerated &&
                result.advancedRows == 0,
            "finished generator stays inert even with a visible frontier");

    // Live events share this stream. Precomputing an entire level cannot stand
    // in for generation interleaved with enemy alarms that also draw randomness.
    AreaOneBuilder interleaved;
    GameMakerRandom liveRandom(2);
    interleaved.reset(240, 704, liveRandom);
    Commands liveCommands;
    ticks = 0;
    while (!interleaved.endReached() && ticks++ < 128) {
        require(interleaved.stepVisible(pack, liveRandom, interleaved.y() - 127,
                    collect, &liveCommands, result), "interleaved builder succeeds");
        if (result.patternGenerated) liveRandom.irandomSmall(60);
    }
    bool different = liveCommands.size() != directCommands.size();
    for (std::size_t i = 0; !different && i < liveCommands.size(); ++i)
        different = !sameCommand(liveCommands[i], directCommands[i]);
    require(interleaved.endReached() && different,
            "intervening gameplay RNG draws change later emitted level content");
    std::cout << "area-one visible builder tests passed\n";
}
