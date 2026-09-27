// Bounded deterministic gameplay timing, with the production effect assets.
// No pacing: update, draw submission and presentation are measured separately.
// Build: make build/profile-sdl-game
// Run: ./build/profile-sdl-game [FRAMES (1..6000)] [SCREENSHOT_DIRECTORY]
// Default: 1800 updates; every run also has a 45-second wall-clock deadline.
// present_ms includes the renderer's vsync wait, not just GPU work. The normal
// accelerated backend is representative; SDL's dummy/software backend is not.
// DOWNWELL_PROFILE_DRAW_EVERY=N samples rendering every Nth simulation tick,
// allowing a deeper replay within the deadline without changing its inputs.
// DOWNWELL_PROFILE_FIRE_PHASE=0..9 selects a repeatable firing sequence;
// different sequences reach different depths without altering game state.
// DOWNWELL_PROFILE_PACED=1 uses the production tick limiter as well as vsync.
#include "core/Game.h"
#include "core/RoomPack.h"
#include "core/GameplayContactProfileInput.h"
#include "core/SpritePack.h"
#include "core/FontPack.h"
#include "core/StoredWorldCollision.h"
#include "platform/sdl/PlatformSDL.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include <memory>
#include <string>

static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}
int main(int argc,char** argv) {
    unsigned frames=downwell::gameplayContactProfileFrames;
    unsigned drawEvery=1;
    unsigned firePhase=downwell::gameplayContactProfilePhase;
    const bool paced=std::getenv("DOWNWELL_PROFILE_PACED") &&
        std::string(std::getenv("DOWNWELL_PROFILE_PACED"))=="1";
    if(const char* value=std::getenv("DOWNWELL_PROFILE_FIRE_PHASE")){
        char* end=nullptr;const auto parsed=std::strtoul(value,&end,10);
        if(!*value || *end || parsed>9)return 2;
        firePhase=static_cast<unsigned>(parsed);
    }
    if(const char* value=std::getenv("DOWNWELL_PROFILE_DRAW_EVERY")){
        char* end=nullptr;const auto parsed=std::strtoul(value,&end,10);
        if(!*value || *end || parsed<1 || parsed>6000)return 2;
        drawEvery=static_cast<unsigned>(parsed);
    }
    if(argc>3)return 2;
    if(argc>1){
        char* end=nullptr;
        const auto value=std::strtoul(argv[1],&end,10);
        if(!*argv[1] || *end || value==0 || value>6000)return 2;
        frames=static_cast<unsigned>(value);
    }
    const auto rb=read("generated/downwell-rooms.dwrm");
    const auto ob=read("generated/downwell-objects.dwob");
    const auto bb=read("generated/downwell-builder-patterns.dwbp");
    const auto sb=read("generated/downwell-sprites.dwsp");
    const auto fb=read("generated/downwell-fonts.dwfn");
    const auto eb=read("generated/amiga-game.dwap");
    downwell::RoomPack rooms(rb.data(),rb.size());
    downwell::ObjectPack objects(ob.data(),ob.size());
    downwell::BuilderPatternPack builders(bb.data(),bb.size());
    downwell::SpritePack sprites(sb.data(),sb.size());
    downwell::FontPack fonts(fb.data(),fb.size());
    downwell::EffectMaskAssets effects;
    if(!rooms.valid() || !objects.valid() || !builders.valid() ||
       !sprites.valid() || !fonts.valid() || !effects.bind(eb.data(),eb.size())) return 20;
    downwell::PlatformSDL platform;
    if(!platform.ready() || !platform.loadSpriteAssets(sprites,"extracted/utmt-0.9.2.0/TexturePages") ||
       !platform.loadFontAssets(fonts)) return 20;
    const auto cb=read("generated/downwell-sprites.dwsc"),mb=read("generated/downwell-sprite-masks.dwsm");
    downwell::SpriteCollisionPack collisions(cb.data(),cb.size());
    downwell::SpriteMaskPack masks(mb.data(),mb.size());
    auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,collisions,masks);
    if(!owner->valid())return 20;
    downwell::Game game(&rooms,&objects,&builders,&effects,owner->binding(),true);
    if(!owner->attach(game.startup()))return 20;
    game.applyStartupShortcut();
    if(!game.startup().inMainRoom()) return 20;
    using Clock=std::chrono::steady_clock;
    const auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
    const auto deadline=Clock::now()+std::chrono::seconds(45);
    double update=0,draw=0,present=0,pacing=0,peak=0;
    unsigned rendered=0;
    std::puts("tick,y,records,update_ms,draw_ms,present_ms,pacing_ms,ticks_per_second,peak_work_ms,state_digest");
    auto sampleBegin=Clock::now();
    for(unsigned tick=0;tick<frames;++tick){
        if(Clock::now()>deadline) return 21;
        if(platform.pollInput().quit) return 0;
        const auto& state=game.startup();
        const auto input=downwell::gameplayContactProfileInput(tick,state.mainPlayerX(),state.mainPlayerY(),firePhase);
        const auto a=Clock::now();game.update(input);const auto b=Clock::now();
        const bool render=tick%drawEvery==0 || tick+1==frames;
        if(render)game.render(platform);const auto c=Clock::now();
        if(render){platform.present();++rendered;}const auto d=Clock::now();
        if(paced)platform.waitForNextTick();const auto e=Clock::now();
        if(!game.simulationSucceeded() || !game.renderingSucceeded()) return 20;
        update+=ms(a,b);draw+=ms(b,c);present+=ms(c,d);
        pacing+=ms(d,e);
        if(ms(a,c)>peak)peak=ms(a,c);
        if((tick+1)%180==0 || tick+1==frames){
            const unsigned updates=(tick%180)+1;
            std::printf("%u,%.1f,%zu,%.3f,%.3f,%.3f,%.3f,%.1f,%.3f,%016llx\n",tick+1,state.mainPlayerY(),
                        state.activeInstanceCount(),update/updates,draw/(rendered?rendered:1),
                        present/(rendered?rendered:1),pacing/updates,
                        1000.0*updates/ms(sampleBegin,e),peak,
                        static_cast<unsigned long long>(state.stateDigest()));
            update=draw=present=pacing=peak=0;
            rendered=0;
            sampleBegin=e;
            std::fflush(stdout);
        }
    }
    if(argc==3 && !platform.saveScreenshot(argv[2]))return 20;
}
