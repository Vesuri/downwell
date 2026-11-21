// WR.5 host replay of the recorded world-one slice routes (the .inputs and
// .states pairs AmigaSceneRuntimeTest's testWorldOneSlice writes).
//   build/amiga-world-one-replay-test <rooms> <objects> <patterns> <route base>
// replays the tape through the shipping world and checks every recorded
// witness, in seconds rather than an emulator run.
//   DOWNWELL_REPLAY_UNOWNED=1  the shipping configuration: no collision
//                              owner, the world owns objDitherAbove
//   DOWNWELL_REPLAY_SOUNDS=<f> write the replay's sound requests
//   DOWNWELL_REPLAY_DRAWS=<f>  write each frame's gameplay draw list
//   DOWNWELL_SLICE_HARD=1      the hard-mode routes' run style
// tools/compare-world-one-ownership.sh requires both ownership modes to give
// the same sounds, draw lists and effect-surface producers.
#define main scene_runtime_test_main
#include "AmigaSceneRuntimeTest.cpp"
#undef main
#include "platform/amiga/AmigaGameplayDrawList.h"
#include <sstream>

int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}

static void replayWorldOne(const char* roomPath, const char* objectPath, const char* patternPath, const std::string& base) {
    auto rb=readFile(roomPath),ob=readFile(objectPath),pb=readFile(patternPath);
    downwell::RoomPack rooms(rb.data(),rb.size());
    downwell::ObjectPack objects(ob.data(),ob.size());
    downwell::BuilderPatternPack patterns(pb.data(),pb.size());
    downwell::PackedRoom room;require(rooms.findRoom(11,room),"replay room");
    // The unowned draw order's depth table is instance_create's depth.
    for(std::uint32_t i=0;i<objects.objectCount();++i){
        downwell::PackedObject o;require(objects.object(i,o),"replay object");
        if(o.objectId>=111&&o.objectId<=119&&o.objectId!=114)
            require(downwell::enemyDrawDepth(o.objectId)==o.creationDepth,"enemy draw depth matches the object pack");
    }
    const bool owned=!std::getenv("DOWNWELL_REPLAY_UNOWNED");
    auto sb=readFile("generated/downwell-sprites.dwsc");
    static std::unique_ptr<downwell::SpriteCollisionPack> sprites;
    sprites.reset(new downwell::SpriteCollisionPack(sb.data(),sb.size()));
    static decltype(downwell::makeCollisionOwner(objects,*sprites)) owner;
    static std::unique_ptr<AmigaPortRuntime> gamePointer;
    if(owned){
        owner=downwell::makeCollisionOwner(objects,*sprites,{},{},&rooms,
            downwell::GameplayCollisionOwner::Ownership::nativeScenery,true,true,false);
        require(owner->valid()&&owner->declareExternalCollisionBackend(),"replay owner");
        gamePointer.reset(new AmigaPortRuntime(300,false,1201-176,true,
            owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms,
            GameplayWorld::PlayerBirthTiming::authoredRoom));
        require(gamePointer->effectsSucceeded()&&owner->attach(*gamePointer),"replay owner attaches");
    }else{
        gamePointer.reset(new AmigaPortRuntime(300,false,1201-176,true,{},{},{},nullptr,&rooms,
            GameplayWorld::PlayerBirthTiming::authoredRoom));
        require(gamePointer->bindOwnedDitherAbove(),"replay world owns objDitherAbove");
    }
    auto& game=*gamePointer;
    game.configureCompactAreaTitles(downwell::area_titles::entries,downwell::area_titles::count,"english");
    require(game.configureCompactShopSpeech(downwell::shop_speech::entries,downwell::shop_speech::count,"english"),
        "replay merchant speech strings");
    game.initializeEntryInput(false);
    std::unique_ptr<downwell::GameplayEncounterSession> session;
    if(owned){
        session.reset(new downwell::GameplayEncounterSession(game,*owner));
        require(session->start(),"replay encounter session");
    }
    LiveSceneWorld live(patterns,objects);
    live.bind(game,rooms,room);
    if(std::getenv("DOWNWELL_SLICE_HARD")){
        require(game.beginRun(0,true),"hard-mode replay begins the run");
        live.builder.applyRunStyle(0,true);
    }
    // DOWNWELL_REPLAY_SOUNDS=<path>: the replay's sound requests, in the
    // slice's log format, for comparing ownership modes.
    std::unique_ptr<downwell_test::SoundRequestLog> sounds;
    if(const char* path=std::getenv("DOWNWELL_REPLAY_SOUNDS"))sounds.reset(new downwell_test::SoundRequestLog(path));
    game.soundRequests().clear();
    bool resultsBank=false;
    auto drainSounds=[&]{
        if(!sounds){game.soundRequests().clear();return;}
        resultsBank=resultsBank||(game.deathResults().active&&!game.exitDither().active);
        const bool clearBank=(game.levelResultRequested()||game.levelClearActive())&&!game.nextLevelRequested();
        sounds->banks(game.soundRequests().tick(),"run",clearBank?"clear":resultsBank?"results":"none");
        sounds->drain(game.soundRequests(),game.soundVoices());
    };
    // The shipping gameplay CPU bank, as DownwellAmiga.cpp collects from it.
    const auto art=readFile("generated/amiga-cavern.dwap");
    static AmigaActorFrames frames;require(frames.prepare(art.data(),art.data()+art.size()),"replay actor frames");
    std::unique_ptr<std::ofstream> draws;
    if(const char* path=std::getenv("DOWNWELL_REPLAY_DRAWS"))draws.reset(new std::ofstream(path));
    static AmigaGameplayDrawList list;
    static downwell::EffectMaskAssets effects;require(effects.bind(art.data(),art.size()),"replay effect assets");
    auto logDraw=[&]{
        // DownwellAmiga.cpp shows the clear scene instead of gameplay.
        if(!draws||game.levelResultRequested()||game.levelClearActive())return;
        const int vx=160+game.cameraOffsetX(),vy=34+game.cameraOffsetY();
        const bool ok=list.collect(game,art.data(),art.data()+art.size(),vx,vy,!game.dead(),&frames);
        *draws<<game.tick()<<' '<<ok<<' '<<list.count;
        for(unsigned i=0;i<list.count;++i){const auto& a=list.actors[i];
            *draws<<' '<<a.sprite<<':'<<a.frame<<':'<<a.x<<':'<<a.y<<':'<<a.flip;}
        {
            // The controller effect-surface producers DownwellAmiga.cpp uses.
            using Real=downwell::MotionBinary64;
            const downwell::EffectSurfacePlacement::View view{Real::fromInteger(vx),Real::fromInteger(vy),
                Real::fromInteger(160),Real::fromInteger(284)};
            *draws<<" S";
            const bool good=game.visitRegisteredEffectSurfaces(effects,view,[&](const downwell::EffectMaskAssets::Asset& a,Real x,Real y){
                *draws<<' '<<(a.low?a.low-art.data():-1)<<':'<<a.originX<<':'<<a.originY<<':'<<x.high<<'.'<<x.low<<':'<<y.high<<'.'<<y.low;return true;});
            *draws<<(good?" ok":" FAIL")<<" H";
            for(unsigned i=0;i<game.enemyCount();++i){const auto& e=game.enemyAt(i);
                if(e.active&&e.awake)*draws<<' '<<240+e.x<<','<<176+e.y;}
            const bool shades=game.visitRegisteredEnemyShades([&](Real x,Real y){*draws<<" r"<<x.high<<'.'<<x.low<<','<<y.high<<'.'<<y.low;return true;});
            *draws<<(shades?" ok":" FAIL")<<" T"<<(game.levelNumbers()?int(game.levelNumbers()->size()):-1)
                <<" C"<<game.compactCorpses().size()<<" D"<<game.compactDebris().size()<<" M"<<game.smokeEffects().size()
                <<" R"<<(game.registeredSmoke()?int(game.registeredSmoke()->size()):-1)<<'/'<<(game.registeredCorpses()?int(game.registeredCorpses()->size()):-1)
                <<'/'<<(game.timeShardInstances()?int(game.timeShardInstances()->size()):-1);
        }
        if(std::getenv("DOWNWELL_REPLAY_ENEMIES")){
            *draws<<" E";
            for(unsigned i=0;i<game.enemyCount();++i){const auto& e=game.enemyAt(i);
                if(e.lifetimeId)*draws<<' '<<e.lifetimeId<<':'<<e.objectId<<':'<<e.imagePhase<<':'<<e.imageFrame<<':'<<e.active<<e.awake<<e.frozen<<e.activationPending<<':'<<e.spriteId;}
        }
        if(game.hasRoomActorDraw()){
            *draws<<" |";
            game.visitRoomActorsReadOnly([&](GameplayWorld::DrawActor kind,downwell::InstanceHandle key,downwell::RunnerImageValue depth){
                std::int32_t d=0;depth.promoted().roundPixel(d);
                int index=-1;
                if(kind==GameplayWorld::DrawActor::enemy){if(const auto* e=game.resolveEnemyLifetime(key))index=int(e->lifetimeId)*1000+e->objectId;}
                if(kind==GameplayWorld::DrawActor::bullet){if(const auto* b=game.resolveBulletLifetime(key))index=int(b-&game.bulletAt(0));}
                *draws<<' '<<int(kind)<<'@'<<d<<'#'<<index;return true;});
        }
        *draws<<'\n';
    };
    const auto tape=readFile((base+".inputs").c_str());
    std::vector<std::string> witnesses;
    {std::ifstream in(base+".states");std::string line;while(std::getline(in,line))witnesses.push_back(line);}
    std::size_t t=0,w=0;
    auto check=[&](unsigned level){
        std::ostringstream state;
        state<<game.tick()<<' '<<level<<' '<<game.player().offsetXFixed()<<' '<<game.player().offsetYFixed()
            <<' '<<game.health()<<' '<<game.randomDrawCount()<<' '<<game.currency()
            <<' '<<game.shops().noControl<<' '<<game.compactShopSpeechLive();
        if(w>=witnesses.size()||state.str()!=witnesses[w]){
            std::cout<<"REPLAY first divergence at witness "<<w<<" (tape byte "<<t<<"): replay="<<state.str()
                <<" recorded="<<(w<witnesses.size()?witnesses[w]:std::string("<none>"))<<std::endl;
            std::exit(1);
        }
        ++w;
    };
    unsigned enemyPeak=0,bulletPeak=0;
    auto frameStep=[&](auto&& step){
        enemyPeak=std::max(enemyPeak,game.enemyCount());bulletPeak=std::max(bulletPeak,game.bulletSlotEnd());
        if(!session){step();drainSounds();logDraw();return;}
        downwell::GameplayEncounterSession::Frame frame(*session);
        require(frame.valid()&&frame.sampleInput(downwell::unpackReplayInput(std::uint8_t(tape[t-1]))),"replay frame samples input");
        step();
        drainSounds();
        require(frame.beginDraw(true),"replay frame draws");
        logDraw();
        require(frame.finish(),"replay frame completes");
    };
    for(unsigned level=1;level<=3&&t<tape.size();++level){
        while(t<tape.size()&&!game.levelResultRequested()&&!game.dead()){
            check(level);
            const auto packed=std::uint8_t(tape[t++]);
            frameStep([&]{game.step(packed);});
        }
        if(game.dead()){
            while(t<tape.size()){const auto packed=std::uint8_t(tape[t++]);frameStep([&]{game.step(packed);});}
            break;
        }
        bool heart=false;require(game.enterLevelClear(0,heart),"replay Result Create");
        while(t<tape.size()&&!game.nextLevelRequested()){
            check(level);
            bool fire=downwell::unpackReplayInput(std::uint8_t(tape[t++])).fire;
            frameStep([&]{require(game.stepLevelClear(fire,false,false,heart,[](auto,unsigned){}),"replay Result Step");});
        }
        if(level<3&&game.nextLevelRequested()){
            downwell::Input input;input.fire=true;
            require(game.prepareNextAreaOneLevel(input,300),"replay next cavern");
            live.bind(game,rooms,room,true);
        }
    }
    if(sounds)sounds->flush();
    std::cout<<"REPLAY "<<(owned?"owned":"unowned")<<" matched "<<w<<" witnesses over "<<t<<" of "<<tape.size()
        <<" inputs; tick="<<game.tick()<<" area="<<game.runStage().area<<" level="<<game.runStage().level
        <<" hp="<<game.health()<<" draws="<<game.randomDrawCount()
        <<" terrain-peak="<<nativeTerrainPeak<<" enemy-peak="<<enemyPeak<<" bullet-peak="<<bulletPeak<<std::endl;
    require(w==witnesses.size()&&t==tape.size(),"replay consumes the whole route");
}


int main(int argc, char** argv) {
    if(argc<5){std::fprintf(stderr,"usage: %s rooms objects patterns route-base\n",argv[0]);return 2;}
    replayWorldOne(argv[1],argv[2],argv[3],argv[4]);
    return 0;
}
