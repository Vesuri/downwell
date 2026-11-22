#include "core/StoredEncounterSession.h"
#include "platform/sdl/AudioSDL.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
using namespace downwell;
namespace downwell {
struct AudioSDLTestAccess {
    static std::uint32_t master(AudioSDL& audio){AudioSDL::Lock lock(audio.device_);return audio.master_.value.bits;}
};
}
static std::vector<std::uint8_t> read(const char* path){
    std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};
}
struct Sink final:Renderer {
    unsigned calls=0;
    void clear(Color)override{++calls;}
    void setClipRect(int,int,int,int)override{++calls;}
    void clearClipRect()override{++calls;}
    void fillRect(int,int,int,int,Color)override{++calls;}
    void drawSpriteTiled(std::int32_t,std::uint32_t,int,int)override{++calls;}
    void drawSprite(std::int32_t,std::uint32_t,int,int)override{++calls;}
    void drawSpriteTransformed(std::int32_t,std::uint32_t,float,float,float,float,float)override{++calls;}
    void drawText(std::int32_t,const char*,float,float,Color,TextAlign)override{++calls;}
};
struct SessionAudio:AudioSDL {
    unsigned stops=0;
    bool stopAll(){++stops;return AudioSDL::stopAll();}
};
int main(){
    assert(SDL_Init(SDL_INIT_AUDIO)==0);
    const auto r=read("generated/downwell-rooms.dwrm"),o=read("generated/downwell-objects.dwob"),
        b=read("generated/downwell-builder-patterns.dwbp"),c=read("generated/downwell-sprites.dwsc"),
        m=read("generated/downwell-sprite-masks.dwsm");
    RoomPack rooms(r.data(),r.size());ObjectPack objects(o.data(),o.size());
    BuilderPatternPack builders(b.data(),b.size());SpriteCollisionPack sprites(c.data(),c.size());
    SpriteMaskPack masks(m.data(),m.size());
    assert(rooms.valid()&&objects.valid()&&builders.valid()&&sprites.valid()&&masks.valid());
    using Session=StoredEncounterSession<SessionAudio>;
    {
        SessionAudio missing;assert(missing.ready()&&!missing.hasResource(349));
        for(int id=TimeFieldWind::firstSound;id<TimeFieldWind::firstSound+TimeFieldWind::soundCount-1;++id){
            char path[128];std::snprintf(path,sizeof(path),"extracted/audio/sound-effects/%03d_sfxMagicWind%d.ogg",id,id-348);
            assert(missing.load(id,path,MotionBinary64::fromInteger(1)));
        }
        assert(missing.hasResource(349)&&!missing.hasResource(355));
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto game=std::make_unique<Game>(&rooms,&objects,&builders,nullptr,owner->binding(),true);
        assert(owner->attach(game->startup()));
        auto session=std::make_unique<Session>(*game,*owner,missing);
        const auto before=game->startup().stateDigest();
        assert(!session->start()&&!session->succeeded()&&!game->startupShortcutApplied());
        assert(game->startup().stateDigest()==before);
        assert(!session->update({})&&game->tick()==0);
        Sink sink;assert(!session->draw(sink)&&!sink.calls);
    }
    {
        SessionAudio audio;assert(audio.ready());
        for(int id=TimeFieldWind::firstSound;id<TimeFieldWind::firstSound+TimeFieldWind::soundCount;++id){
            char path[128];std::snprintf(path,sizeof(path),"extracted/audio/sound-effects/%03d_sfxMagicWind%d.ogg",id,id-348);
            assert(audio.load(id,path,MotionBinary64::fromInteger(1))&&audio.hasResource(id));
        }
        assert(!audio.hasResource(-1)&&!audio.hasResource(361));
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto game=std::make_unique<Game>(&rooms,&objects,&builders,nullptr,owner->binding(),true);
        assert(owner->attach(game->startup()));
        auto session=std::make_unique<Session>(*game,*owner,audio);
        assert(session->start()&&game->startupShortcutApplied());
        assert(audio.stops==1);
        assert(AudioSDLTestAccess::master(audio)==0x3f800000u);
        session->settings().masterGain={0x3fe00000u,0};
        assert(session->update({})&&AudioSDLTestAccess::master(audio)==0x3f000000u);
        Sink sink;
        const_cast<StartupRuntime&>(game->startup()).useCaptureGarden(true); // W1.14: pinned walk
        for(unsigned frame=0;frame<4096&&!game->startup().inMainRoom();++frame){
            Input input;
            if(game->startup().inMenu())input.right=true;
            if(game->startup().inPlayMenu()&&!game->startup().playMenuYOffset()&&!game->startup().playMenuSelected())input.fire=true;
            assert(session->update(input)&&session->draw(sink));
        }
        assert(game->startup().inMainRoom()&&session->succeeded());
        std::printf("Session main entry: RNG=%u x=%.0f y=%.0f\n",unsigned(game->startup().randomDrawCount()),
            double(game->startup().mainPlayerX()),double(game->startup().mainPlayerY()));
        for(unsigned i=0;i<164;++i){
            Input input;input.cancel=i<2;
            assert(session->update(input)&&session->draw(sink));
            assert(game->startup().shops().inputState().cancel==(i==0));
        }
        assert(sink.calls);
        // Lifecycle fixture, not proof of a naturally generated level exit or
        // a combat kill: put the real exit at the player and use the normal
        // alarms, selection, room handoff, death menu and Retry thereafter.
        auto& world=const_cast<StartupRuntime&>(game->startup());
        world.registerLevelExit(int(world.mainPlayerY()));
        Input heldCancel;heldCancel.cancel=true;
        for(unsigned i=0;i<240&&!world.levelClearActive();++i)
            assert(session->update(heldCancel)&&session->draw(sink));
        assert(world.levelClearActive());
        Input earlyConfirm=heldCancel;earlyConfirm.fire=true;
        for(unsigned i=0;i<160;++i)assert(session->update(earlyConfirm)&&session->draw(sink));
        assert(world.levelClearActive()&&world.levelClear().selection.visible);
        assert(!world.shops().inputState().confirm);
        // Release and press again: a held entrance press cannot auto-select.
        assert(session->update(heldCancel)&&session->draw(sink));
        Input confirm;confirm.fire=true;confirm.cancel=true;
        for(unsigned i=0;i<240&&world.runStage().level!=2;++i)
            assert(session->update(confirm)&&session->draw(sink));
        assert(world.inMainRoom()&&world.runStage().level==2&&!world.levelClearActive());
        assert(session->update(heldCancel)&&session->draw(sink));
        assert(!world.shops().inputState().cancel); // Held across clear -> new room.
        // A looping device voice cannot expire while the CPU fast-forwards
        // the death menu. Only the production ResetPlayer binding can stop it.
        int oldVoice=-1;bool oldPlaying=false;
        // Keep the witness outside the admitted wind family so its ordinary
        // overlap-channel stop cannot accidentally satisfy this assertion.
        assert(audio.load(348,"extracted/audio/sound-effects/348_sfxPickup_Cat.wav",MotionBinary64::fromInteger(1)));
        assert(audio.play(348,90,true,oldVoice)&&audio.isPlaying(oldVoice,oldPlaying)&&oldPlaying);
        const auto stopsBeforeRetry=audio.stops;
        assert(world.subtractPlayerHealth(MotionBinary64::fromInteger(world.mainPlayerHealth())));
        bool retried=false;
        for(unsigned i=0;i<900&&!retried;++i){
            Input input;input.cancel=true;
            if(world.deathResults().active){
                const auto& menu=world.deathResults().menu;
                if(menu.sequenceOver&&!menu.inputDisabled&&!menu.goalPassed)input.fire=true;
                else if(menu.goalPassed&&menu.goalNotice>=3)input.fire=(i%2)==0;
            }
            const bool results=world.deathResults().active;
            assert(session->update(input,42)&&session->draw(sink));
            retried=results&&!world.deathResults().active;
        }
        assert(retried&&world.inMainRoom()&&world.mainPlayerHealth()==4&&world.randomSeed()==42);
        assert(audio.stops==stopsBeforeRetry+1&&audio.isPlaying(oldVoice,oldPlaying)&&!oldPlaying);
        assert(session->update(heldCancel)&&session->draw(sink)&&!world.shops().inputState().cancel);
        assert(session->update({})&&session->draw(sink));
        assert(session->update(heldCancel)&&session->draw(sink)&&world.shops().inputState().cancel);
        assert(owner->valid()&&session->succeeded());
        std::puts("Session retained across level completion, death and Retry");
        // There must be no callback borrowing this owner outside the update.
        // A nested probe can bind the existing core scope after the session.
        assert(game->withCollisionPass([](StartupRuntime&){return false;},[]{return true;}));
        // A registered family without its required Draw environment must fail,
        // not disappear. The session then prevents every subsequent operation,
        // even if the offending object is removed by diagnostic fixture code.
        auto shop=world.shopCreationOwner<1>(objects,audio);
        assert(shop.children().create(212,world.mainCameraRenderViewX()+80,world.mainCameraRenderViewY()+60));
        assert(!session->draw(sink)&&!session->succeeded());
        assert(shop.clear());
        const auto tick=game->tick(),rng=world.randomDrawCount();const auto commands=sink.calls;
        assert(!session->update({})&&!session->draw(sink));
        assert(game->tick()==tick&&world.randomDrawCount()==rng&&sink.calls==commands);
        assert(!session->retire());
        assert(!world.shops().borrowedInput);
        // Normal application exit retires a healthy session without requiring
        // the device or world to be destroyed inside the simulation callback.
        auto quitOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto quitGame=std::make_unique<Game>(&rooms,&objects,&builders,nullptr,quitOwner->binding(),true);
        assert(quitOwner->attach(quitGame->startup()));
        auto quitSession=std::make_unique<Session>(*quitGame,*quitOwner,audio);
        assert(quitSession->start());
        Input quit;quit.quit=true;
        assert(quitSession->update(quit)&&quitSession->finished()&&quitSession->retire());
        const auto quitTick=quitGame->tick();
        assert(!quitSession->update({})&&quitGame->tick()==quitTick);
    }
    SDL_Quit();
    std::puts("Stored production session resources, startup, update, Draw and terminal failure passed");
}
