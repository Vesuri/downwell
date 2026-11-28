#ifndef DOWNWELL_CORE_RUN_SESSION_H
#define DOWNWELL_CORE_RUN_SESSION_H
#include "BulletWallSound.h"
#include "RunGemStatistics.h"
#include "WallBackgroundTiles.h"
#include "RunClock.h"
#include "RunCombo.h"
#include "RunStage.h"
#include "DeathResults.h"
#include "ResultsDestination.h"
#include "AmmoHudMotion.h"
#include "AmmoGaugeRows.h"
#include "Consumables.h"
#include "ShopWorld.h"
#include "SideCameraLocker.h"
#include "LevelExit.h"
#include "GoalTimeField.h"
#include "ExitDither.h"
#include "UpgradeInventory.h"
#include "CurrencyStep.h"
#include "GemStreakRise.h"
#include "MachineGunProfile.h"
#include "LevelClearController.h"
#include "FlashBallEffects.h"
#include "SmokeEffects.h"
#include "BloodEffects.h"
#include "CorpseExpiryEffects.h"
#include "CorpseLifecycle.h"
#include "TimeStopBound.h"
#include "BloodGoalContact.h"
#include "TimeStopGoalContact.h"
#include "OrdinaryTimeFields.h"
#include "TypicalPlayerDamage.h"
#include "PlayerDamage.h"
#include "MusicDuck.h"
#include "PlayerDamageNumbers.h"
#include "RunnerPointDirection.h"
#include "ContactCoordinates.h"
#include "PortableAtan.h"
#include "TableAtan.h"
#include "ForcedPlayerJump.h"
#include "Profile.h"
#include "SoundPlayScript.h"
#include "TimeFieldWind.h"
#include "SoundRequests.h"
#include "ShopViewEvents.h"
namespace downwell {
// Persistent subset currently modeled by the port. ResetPlayer calls
// scrInitialize, which resets defaults then reloads records through scrLoadSave.
// Callers supply that snapshot; a new run must not erase it or count death twice.
struct RunRecords {
    std::uint64_t lifetimeGems = 0;
    std::uint32_t bestRunGems = 0;
    std::uint32_t bestCombo = 0;
};

// Shared ownership and reset boundary for the implemented run-level systems.
// World/room instances, player initialization, RNG reseeding and platform
// effects remain separate; this is not a whole-game restart by itself.
struct RunSession {
    // The currently admitted persistent controller. Draw mutates this even
    // when simulation is paused; replay/verification repaints must not tick it.
    mutable GemStreakRise gemStreakRise;
    // soundInitialize writes entries 0..10 to zero. Room changes do not
    // reinitialize them; scrInitialize/new-run reset does.
    int overlapSounds[11]{};
    TimeFieldWind fieldWind; // global.voidWind survives ordinary room changes.
    // W2.3: audio_is_playing as gameplay sees it (source lengths, not a
    // device). Room changes keep voices; ResetPlayer/scrInitialize stop all.
    SoundVoices voices;
    // W2.3: every sound the run asks for, in call order (see SoundRequests).
    SoundRequests soundRequests;
    // Script fields for owners that keep none of their own (the compact
    // world's objects); nothing reads them back across calls.
    SoundPlayState soundCaller;
    SoundRequestAudio audio(int category=-1,bool cut=false){
        return SoundRequestAudio(voices,soundRequests).context(category,cut);
    }
    // soundPlay(asset, priority, loop, cut): the new voice (sndsnd).
    int soundPlay(int asset,int priority,bool loop,bool cut){
        SoundPlayEnvironment env;auto a=audio(-1,cut);int result=-1;
        SoundPlayScript::plain(soundCaller,env,a,asset,priority,loop,cut,result,effectsSucceeded);
        return result;
    }
    // soundPlayOL(asset, priority, loop, cut, channel): the voice if it is
    // playing, else -1 (and -1 for an offscreen enemymove/waterEnemy call,
    // which also needs the caller's y and the view: the overload below).
    // The gunshot channel's bgmDuck is music (WM).
    int soundPlayOL(int asset,int priority,bool loop,bool cut,const char* channel){
        SoundPlayEnvironment env;return soundPlayOL(asset,priority,loop,cut,channel,env);
    }
    int soundPlayOL(int asset,int priority,bool loop,bool cut,const char* channel,
        Fixed16 y,Fixed16 viewY,Fixed16 viewHeight){
        SoundPlayEnvironment env;env.y=y;env.viewY=viewY;env.viewHeight=viewHeight;
        return soundPlayOL(asset,priority,loop,cut,channel,env);
    }
    int soundPlayOL(int asset,int priority,bool loop,bool cut,const char* channel,SoundPlayEnvironment& env){
        const int category=SoundPlayScript::channel(channel);
        auto a=audio(category,cut);int result=-1;bool returned=false;
        SoundPlayScript::overlap(soundCaller,env,a,[]{return true;},overlapSounds[category],
            asset,priority,loop,cut,channel,result,returned,effectsSucceeded);
        return returned?result:-1;
    }
    bool stopSound(int id){return audio().stop(id);}
    bool setSoundGain(int id,AudioLevel gain,int time=0){return audio().setGain(id,gain,time);}
    bool setSoundPitch(int id,AudioLevel pitch){return audio().setPitch(id,pitch);}
    // W2.3: Shop Step's sounds for the compact shops, after the interaction:
    // the step's merchant and UI sounds (ShopFeedback), then the !criminal
    // branch's sfxSkInRange on entering the view (left/top in room pixels,
    // 160 x 284) and sfxSkAnger once the player passes the keeper.
    void playShopSounds(int viewLeft,int viewTop,int playerX){
        for(auto& shop:shops.instances){
            if(!shop.allocated||shop.sourceId)continue;
            const auto& f=shop.feedback;
            for(unsigned i=0;i<f.soundCount;++i)
                soundPlayOL(f.sounds[i].resource,f.sounds[i].priority,false,true,
                    f.sounds[i].channel==ShopFeedback::Channel::merchant?"merchant":"UI");
            if(!shop.present)continue;
            struct Ops {
                RunSession& run;const ShopWorld::Instance& shop;int left,top,player;
                MotionBinary64 x(){return MotionBinary64::fromInteger(shop.x);}
                MotionBinary64 y(){return MotionBinary64::fromInteger(shop.y);}
                MotionBinary64 keeperX(){return MotionBinary64::fromInteger(shop.x+18);}
                bool view(unsigned field,MotionBinary64& out){
                    const int values[]={left,top,160,284};out=MotionBinary64::fromInteger(values[field]);return true;
                }
                bool playerX(MotionBinary64& out){out=MotionBinary64::fromInteger(player);return true;}
                bool sound(int id){run.soundPlayOL(id,90,false,true,"merchant");return run.effectsSucceeded;}
            } ops{*this,shop,viewLeft,viewTop,playerX};
            if(!ShopViewEvents::step(shop.shopInView,shop.keeperAngry,ops))effectsSucceeded=false;
        }
    }
    void playDeathSounds(){
        soundPlay(89,100,false,false);
        setSoundGain(backgroundMusicId,AudioLevel{});
        stopSound(backgroundMusicId);
    }
    // objPauseMenu Create's audio_pause_all and its Resume's audio_resume_all.
    bool pauseAllSounds(){return audio().pauseAll();}
    bool resumeAllSounds(){return audio().resumeAll();}
    template<class Audio> bool playSkeletonStompSound(SoundPlayState& caller,
        SoundPlayEnvironment& environment,Audio& audio){
        int result=-1;bool returned=false;
        return SoundPlayScript::overlap(caller,environment,audio,[]{return false;},overlapSounds[0],
            122,50,false,true,"footsteps",result,returned,effectsSucceeded);
    }
    template<class Audio> bool playSkeletonSound(SoundPlayState& caller,int sound,
        SoundPlayEnvironment& environment,Audio& audio){
        if(!effectsSucceeded)return false;
        if(sound<132||sound>134)return effectsSucceeded=false;
        int result=-1;bool returned=false;
        return SoundPlayScript::overlap(caller,environment,audio,[]{return false;},overlapSounds[4],
            sound,50,false,true,"enemymove",result,returned,effectsSucceeded);
    }
    template<class Random,class Material,class Audio> bool playBulletWallSound(SoundPlayState& caller,
        SoundPlayEnvironment& environment,Random& random,Material material,Audio& audio){
        return BulletWallSound::play(caller,environment,overlapSounds[2],random,material,audio,effectsSucceeded);
    }
    template<class Audio> bool playCurrencyPickupSound(SoundPlayState& caller,
        AudioLevel volume,Audio& audio){
        if(!effectsSucceeded)return false;
        SoundPlayEnvironment env;env.gameVolume=volume;
        int result=-1;bool returned=false;
        // UI (channel 8) neither tests the view nor applies underwater pitch.
        return SoundPlayScript::overlap(caller,env,audio,[]{return false;},overlapSounds[8],
            167,60,false,true,"UI",result,returned,effectsSucceeded);
    }
    template<class Audio> bool playBoxChainSound(SoundPlayState& caller,
        AudioLevel volume,bool inWater,Audio& audio){
        SoundPlayEnvironment env;env.gameVolume=volume;env.inWater=inWater;
        int result=-1;
        return SoundPlayScript::plain(caller,env,audio,342,50,false,true,result,effectsSucceeded);
    }
    template<class Audio> bool playBoxBreakSound(SoundPlayState& caller,int sound,
        AudioLevel volume,Audio& audio){
        if(!effectsSucceeded)return false;
        SoundPlayEnvironment env;env.gameVolume=volume;
        int result=-1;bool returned=false;
        return SoundPlayScript::overlap(caller,env,audio,[]{return false;},overlapSounds[7],
            sound,50,false,true,"waterThings",result,returned,effectsSucceeded);
    }
    RunGemStatistics gems;
    RunClock clock;
    RunCombo combo;
    // global.killCount: scrGlobalStuff resets it for a new run, not rmMain
    // entry. DeathMenu/EndMenu display the cumulative total across levels.
    std::uint32_t kills = 0;
    bool noKill = false; // global.achNoKill; initialized by scrPlayerInit, not room loading.
    bool noDamage=true; // scrGlobalStuff; scrPlayerInit also sets both flags.
    bool areaNoDamage=false;
    bool playerDamaged=false; // scrPlayerGlobalStat / scrPlayerInit; Alarm 1 clears it.
    void expirePlayerDamage(){playerDamaged=false;}
    // global.pHit: initialized by scrPlayerGlobalStat, not scrPlayerInit.
    // Both damage writers assign 1; player End Step advances 1 -> 2 -> 0.
    unsigned playerHit=0;
    void setPlayerHit(){playerHit=1;}
    void playerDamageEndStep(){if(playerHit)playerHit=playerHit>1?0:playerHit+1;}
    bool firstLand=false;
    int backgroundMusicId=193; // scrInitialize; survives ordinary run/room reset.
    // scrPlayerGlobalStat. Only reset to 100 and floored at 10 (whole values).
    Fixed16 oxygen=Fixed16::fromInteger(100);
    MotionBinary64 fallDepth{}; // scrPlayerGlobalStat; player ordinary Step writes yy - objDepthMeter.y.
    bool publishFallDepth(MotionBinary64 privateY,RunnerImageValue meterY){
        auto negative=meterY.promoted();negative.high^=0x80000000u;
        if(!effectsSucceeded||!privateY.add(negative))return effectsSucceeded=false;
        fallDepth=privateY;return true;
    }
    bool setOxygen(Fixed16 value){
        if(!effectsSucceeded)return false;
        oxygen=value;return true;
    }
    bool floorTimeStopOxygen(){
        if(!effectsSucceeded)return false;
        if(oxygen.v<Fixed16::fromInteger(10).v)oxygen=Fixed16::fromInteger(10);
        return true;
    }
    template<class Audio> bool duckMusic(MusicDuck& caller,int duration,AudioLevel amount,Audio& audio){
        if(!effectsSucceeded)return false;
        return effectsSucceeded=caller.execute(duration,amount,[&]{return stage.area;},
            [&]{return backgroundMusicId;},audio);
    }
    static bool areaMusic(int area,int& sound){
        if(area<1||area>5)return false;
        sound=193+area;return true;
    }
    void playerCreated(){noKill=true;noDamage=true;areaNoDamage=true;firstLand=false;oxygen=Fixed16::fromInteger(100);fallDepth={};playerDamaged=false;}
    template<class Audio,class Delay,class Cue>
    bool recordEnemyDeath(bool cutGunshot,Audio& audio,Delay delay,Cue cue){
        return recordEnemyDeath(cutGunshot,audio,[&]{noKill=false;return true;},delay,cue);
    }
    // Explicit publication provider retained as the source/failure reference.
    // The owned overload above writes the shared flag at this same boundary.
    // Both controller scripts remain required. Keep completed prefixes on failure.
    template<class Audio,class NoKill,class Delay,class Cue>
    bool recordEnemyDeath(bool cutGunshot,Audio& audio,NoKill clearNoKill,Delay delay,Cue cue){
        if(!effectsSucceeded)return false;
        if(combo.state.count==0xffffffffu)return effectsSucceeded=false;
        ++combo.state.count;
        if(cutGunshot){
            bool playing=false;
            if(!audio.isPlaying(overlapSounds[1],playing)||!effectsSucceeded)return effectsSucceeded=false;
            // The VM reads OLchannel[1] again after audio_is_playing.
            if(playing&&(!audio.stop(overlapSounds[1])||!effectsSucceeded))return effectsSucceeded=false;
        }
        if(kills==0xffffffffu)return effectsSucceeded=false;
        ++kills;
        if(!clearNoKill()||!effectsSucceeded||!delay()||!effectsSucceeded||!cue()||!effectsSucceeded)
            return effectsSucceeded=false;
        return true;
    }
    RunStage stage;
    // levelBeginCue borrows the caller's actual globals and sound-script
    // fields. Object 372 must finish Create before firstLand changes. Music
    // selection reads the live area after synchronous creation/audio calls.
    template<class Create,class AreaMusic,class Audio>
    bool beginLevelCue(bool& firstLand,bool playerTimeStop,int& bgm,
        MotionBinary64 x,MotionBinary64 y,Create create,AreaMusic areaMusic,
        SoundPlayState& caller,SoundPlayEnvironment& env,Audio& audio){
        if(!effectsSucceeded)return false;
        if(firstLand||playerTimeStop)return true;
        const bool ending=stage.area==5;
        if(!ending&&(!create(372,x,y)||!effectsSucceeded))return effectsSucceeded=false;
        firstLand=true;
        bool playing=false;
        if(!audio.isPlaying(186,playing)||!effectsSucceeded)return effectsSucceeded=false;
        if(playing&&(!audio.stop(186)||!effectsSucceeded))return effectsSucceeded=false;
        if(ending){
            if(!audio.isPlaying(bgm,playing)||!effectsSucceeded)return effectsSucceeded=false;
            if(playing&&(!audio.stop(bgm)||!effectsSucceeded))return effectsSucceeded=false;
        }
        int selected;
        if(!areaMusic(stage.area,selected)||!effectsSucceeded)return effectsSucceeded=false;
        if(bgm!=selected){
            if(!areaMusic(stage.area,selected)||!effectsSucceeded)return effectsSucceeded=false;
            bgm=selected;
        }
        if(ending)return true;
        if(!audio.isPlaying(bgm,playing)||!effectsSucceeded)return effectsSucceeded=false;
        if(playing)return true;
        if(!audio.setGain(bgm,audio_level::one,0)||!effectsSucceeded)return effectsSucceeded=false;
        int result=-1;bool returned=false;
        return SoundPlayScript::overlap(caller,env,audio,[]{return false;},overlapSounds[5],
            bgm,100,true,true,"music",result,returned,effectsSucceeded);
    }
    DeathResults results;
    AmmoHudMotion ammoHud;
    AmmoGaugeRows ammoGauge;
    Consumables consumables;
    UpgradeInventory upgrades;
    unsigned bodiesEaten=0; // global.bodiesEaten (Knife and Fork)
    ShopWorld shops;
    // Wall tiles come only from the reference runner's stored wall
    // collision (StartupRuntime) and host owner fixtures. Freestanding
    // targets link neither, so their store keeps one slot, not 32 KB; an
    // unexpected append beyond it fails effects instead of drawing.
#ifdef DOWNWELL_FREESTANDING
    static constexpr unsigned wallTileCapacity=1;
#else
    static constexpr unsigned wallTileCapacity=2048;
#endif
    WallBackgroundTiles<wallTileCapacity> wallBackground;
    bool addWallBackground(MotionBinary64 x,MotionBinary64 y){
        if(!effectsSucceeded)return false;
        return effectsSucceeded=wallBackground.append(x,y);
    }
    // Room-owned effects are reset on run/room retirement, not on a repaint.
    FlashBallEffects<32> flashEffects;
    // W1.9: Exploding Knees adds four puffs per stomp and Rocket Jump two per
    // jump to the box, death and gun smoke.
    static constexpr unsigned smokeCapacity=48;
    SmokeEffects<smokeCapacity> smokeEffects;
    BloodEffects<32> bloodEffects;
    CorpseExpiryEffects<32> corpseExpiryEffects;
    PlayerDamageNumbers<8> damageNumbers;
    // Compatibility insertion places a new -1000 instance before existing
    // equals, even across object families. Keys survive room/run reset.
    std::uint32_t nextForegroundBirth=1;
    template<class Expiry,class Number> bool visitForegroundEffects(Expiry expiry,Number number)const{
        if(!effectsSucceeded||!corpseExpiryEffects.valid()||!damageNumbers.valid())return false;
        unsigned e=0,n=0;
        while(e<corpseExpiryEffects.size()||n<damageNumbers.size()){
            const auto* left=corpseExpiryEffects.atNewest(e);
            const auto* right=damageNumbers.atNewest(n);
            if((e<corpseExpiryEffects.size()&&(!left||!left->drawBirth))||
               (n<damageNumbers.size()&&(!right||!right->drawBirth)))return false;
            if(left&&right&&left->drawBirth==right->drawBirth)return false;
            if(left&&(!right||left->drawBirth>right->drawBirth)){
                if(!expiry(*left))return false;++e;
            }else{
                if(!number(*right))return false;++n;
            }
        }
        return true;
    }
    // Object 221 has no mask/sprite or collision events. These hooks remain
    // explicit so registry membership can be added without changing ownership.
    struct DamageNumberHooks {
        bool birth(unsigned,RunnerImageValue,RunnerImageValue){return true;}
        bool position(unsigned,bool,const PlayerDamageNumber&){return true;}
        bool retire(unsigned,const PlayerDamageNumber&){return true;}
    };
    std::uint32_t createDamageNumber(MotionBinary64 x,MotionBinary64 y,
        MotionBinary64 amount,MotionBinary64 direction){
        DW_PROFILE_SCOPE(profileDamageNumberCreate);
        if(!effectsSucceeded)return 0;
        if(!nextForegroundBirth){effectsSucceeded=false;return 0;}
        DamageNumberHooks hooks;
        const auto child=damageNumbers.createFromPlayer(x,y,amount,direction,hooks);
        effectsSucceeded=child!=0;
        if(child){damageNumbers.get(child)->drawBirth=nextForegroundBirth++;DW_PROFILE_DAMAGE_BIRTH();}
        return child;
    }
    bool advanceDamageNumberAlarm(PlayerDamageNumbers<8>::Phase phase,unsigned index){
        DamageNumberHooks hooks;
        if(effectsSucceeded)effectsSucceeded=damageNumbers.advanceAlarm(phase,index,hooks);
        return effectsSucceeded;
    }
    // Live Q8 player globals and caller coordinates share the world's origin.
    // Convert without rounding subpixels or doing coordinate sums in int32.
    bool contactDamageNumber(std::int32_t playerX, std::int32_t playerY,
        int callerX, int callerY, int originX, int originY) {
        if (!effectsSucceeded) return false;
        auto coordinate = [](std::int32_t q8, int origin) {
            auto result = MotionBinary64::fromInteger(q8);
            if (!result.zero()) result.high -= 8u << 20; // Exact division by 256.
            result.add(MotionBinary64::fromInteger(origin));
            return result;
        };
        MotionBinary64 px,py,direction;
        bool directed,tableDirection=false;
        {
            DW_PROFILE_SCOPE(profileContactDirection);
            const auto atan=
#ifdef DOWNWELL_TRACE_CONTACT_DIRECTION
                    [&](MotionBinary64 dy,MotionBinary64 dx,MotionBinary64& result){
                        // Test-only observation of the actual narrowed inputs;
                        // no platform dependency or observer in shipping code.
                        extern void traceContactDirection(std::int32_t,std::int32_t,int,int,int,int,
                            MotionBinary64,MotionBinary64);
                        traceContactDirection(playerX,playerY,callerX,callerY,originX,originY,dx,dy);
                        return portableAtan(dy,dx,result);
                    }
#elif defined(DOWNWELL_PORTABLE_CONTACT_ATAN)
                    portableAtan
#else
                    [&](MotionBinary64 dy,MotionBinary64 dx,MotionBinary64& result){
#ifdef DOWNWELL_COMPARE_CONTACT_ATAN
                        if(referenceContactAtan)return portableAtan(dy,dx,result);
#endif
                        tableDirection=tableAtanCandidate(dy,dx,result);
                        return tableDirection||portableAtan(dy,dx,result);
                    }
#endif
                ;
#ifndef DOWNWELL_REFERENCE_CONTACT_COORDINATES
            ContactCoordinates contact;
            if(contact.prepare(playerX,playerY,callerX,callerY,originX,originY)){
                px=contact.playerX;py=contact.playerY;
                directed=RunnerPointDirection::fromDifferences(contact.dx,contact.dy,direction,atan);
            }else
#endif
            {
                px=coordinate(playerX,originX);py=coordinate(playerY,originY);
                auto cx=MotionBinary64::fromInteger(callerX),cy=MotionBinary64::fromInteger(callerY);
                directed=cx.add(MotionBinary64::fromInteger(originX))&&
                    cy.add(MotionBinary64::fromInteger(originY))&&
                    RunnerPointDirection::evaluate(cx,cy,px,py,direction,atan);
            }
        }
        if (!directed) {
            effectsSucceeded = false;
            return false;
        }
        const auto child=createDamageNumber(px,py,MotionBinary64::fromInteger(1),direction);
        if(child&&tableDirection)damageNumbers.get(child)->certifyTableContact();
        return child!=0;
    }
    template<class LengthdirX> bool stepDamageNumbers(PlayerDamageNumbers<8>::Phase phase,LengthdirX lengthdirX){
        DamageNumberHooks hooks;
        if(effectsSucceeded)effectsSucceeded=damageNumbers.step(phase,lengthdirX,hooks);
        return effectsSucceeded;
    }
    bool stepDamageNumbers(PlayerDamageNumbers<8>::Phase phase){
        DW_PROFILE_SCOPE(profileDamageNumberStep);
        DamageNumberHooks hooks;
        if(effectsSucceeded)effectsSucceeded=damageNumbers.step(phase,[](MotionBinary64 speed,MotionBinary64 direction,MotionBinary64& x){
            RunnerImageValue length,angle,radian;
            MotionBinary64 cosine;
            return length.assign(speed)&&angle.assign(direction)&&SmokeDirection::radians(angle,radian)&&
                smokeCosine(radian.promoted(),cosine)&&SmokeDirection::component(length,cosine,false,x);
        },hooks,
#ifdef DOWNWELL_REFERENCE_CONTACT_MOTION
            false
#else
            true
#endif
        );
        return effectsSucceeded;
    }
    void clearDamageNumbers(){DamageNumberHooks hooks;damageNumbers.clear(hooks);}
    // This effect has no source collision events or external membership in the
    // current worlds. Keep publication hooks explicit for registry integration.
    struct ExpiryHooks {
        bool birth(unsigned,const CorpseExpiryEffect&){return true;}
        bool sprite(unsigned,const CorpseExpiryEffect&){return true;}
        bool retire(unsigned,const CorpseExpiryEffect&){return true;}
    };
    std::uint32_t createCorpseExpiry(RunnerImageValue x,RunnerImageValue y){
        if(!effectsSucceeded)return 0;
        if(!nextForegroundBirth){effectsSucceeded=false;return 0;}
        ExpiryHooks hooks;
        const auto child=corpseExpiryEffects.create(x,y,hooks);
        effectsSucceeded=child!=0;
        if(child)corpseExpiryEffects.get(child)->drawBirth=nextForegroundBirth++;
        return child;
    }
    bool emitCorpseExpiry(MotionBinary64 x,MotionBinary64 y){
        RunnerImageValue px,py;
        if(!px.assign(x)||!py.assign(y)){effectsSucceeded=false;return false;}
        return createCorpseExpiry(px,py)!=0;
    }
    // Shared alarm adapter: complete child Create, assign the source-selected
    // sprite, then request parent retirement. The scheduler supplies verified
    // nonempty unit-track frame counts and lifetime removal, not event rules.
    template<class Frames,class Destroy>
    bool advanceCorpseAlarm(CorpseLifecycle& corpse,unsigned index,Frames frames,Destroy destroy){
        if(!effectsSucceeded)return false;
        struct Events {
            RunSession& session;Frames& frames;Destroy& retire;
            bool create(int object,RunnerImageValue x,RunnerImageValue y,std::uint32_t& child){
                if(object!=242)return false;
                child=session.createCorpseExpiry(x,y);return child!=0;
            }
            bool sprite(std::uint32_t child,int id){
                ExpiryHooks hooks;
                return session.corpseExpiryEffects.setSprite(child,id,frames(id),hooks);
            }
            void destroy(){retire();}
        } events{*this,frames,destroy};
        effectsSucceeded=corpse.advanceAlarm(index,events);
        return effectsSucceeded;
    }
    void advanceCorpseExpiryAnimation(){
        ExpiryHooks hooks;
        if(effectsSucceeded)effectsSucceeded=corpseExpiryEffects.advanceAnimation(corpseExpiryEffects.beginPhase(),hooks);
    }
    void clearCorpseExpiry(){ExpiryHooks hooks;corpseExpiryEffects.clear(hooks);}
    OrdinaryTimeFields<64> timeFields;
    bool effectsSucceeded=true;
    bool adoptTimeField(int x,int y,std::uint32_t radiusWord,std::uint32_t sourceId=0){
        if(effectsSucceeded)effectsSucceeded=timeFields.adoptCreate(x,y,radiusWord,sourceId);
        return effectsSucceeded;
    }
    bool createTimeField(int x,int y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=timeFields.create(x,y,random);
        return effectsSucceeded;
    }
    template<class Events> bool damagePlayer(PlayerDamage& caller,MotionBinary64 amount,Events& events){
        if(effectsSucceeded)effectsSucceeded=caller.execute(amount,events);
        return effectsSucceeded;
    }
    template<class Events> bool forcedPlayerJump(ForcedPlayerJump& caller,
        MotionBinary64 horizontal,MotionBinary64 vertical,Events& events){
        if(effectsSucceeded)effectsSucceeded=caller.execute(horizontal,vertical,events);
        return effectsSucceeded;
    }
    // Adapter for players whose published coordinates are integer pixels.
    // Player traversal and live pTimeStop remain the scheduler's responsibility.
    template<class Events> bool typicalPlayerDamage(TypicalPlayerDamage& caller,
        MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
        GameMakerRandom& random,Events& events){
        if(effectsSucceeded)effectsSucceeded=caller.execute(amount,horizontal,vertical,events,
            [&](int x,int y){return createTimeField(x,y,random);});
        return effectsSucceeded;
    }
    void advanceFlashAnimation(){
        if(effectsSucceeded)effectsSucceeded=flashEffects.advanceAnimation(flashEffects.beginAnimation());
    }
    bool emitFlashBurst(Fixed16 x,Fixed16 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=flashEffects.skeletonBurst(x,y,random);
        return effectsSucceeded;
    }
    template<unsigned Count,unsigned Variation,unsigned Radius>
    bool emitFlash(Fixed16 x,Fixed16 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=flashEffects.template burst<Count,Variation,Radius>(x,y,random);
        return effectsSucceeded;
    }
    bool emitBoxFlash(Fixed16 x,Fixed16 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=flashEffects.boxBurst(x,y,random);
        return effectsSucceeded;
    }
    void advanceBloodAnimation(){
        if(effectsSucceeded)effectsSucceeded=bloodEffects.advanceAnimation(bloodEffects.beginPhase());
    }
    // Caller supplies the source TimeStopBound query, including field-mask
    // contacts and strict room-side exemptions. A global pause bit is not enough.
    template<class Allowed> void stepBlood(BloodEffects<32>::Phase phase,Allowed allowed){
        if(effectsSucceeded)effectsSucceeded=bloodEffects.step(phase,allowed);
    }
    bool emitBloodBurst(MotionBinary64 x,MotionBinary64 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=bloodEffects.burst(x,y,random);
        return effectsSucceeded;
    }
    // pTimeStop is supplied by the source player/controller phase, not inferred
    // from goalStopped or control lock. OtherFields covers parentTimeField
    // instances beyond the owned goal field and returns query success + overlap.
    template<class OtherFields> void stepBloodBound(BloodEffects<32>::Phase phase,
        bool pTimeStop,int roomWidth,OtherFields otherFields){
        if(!effectsSucceeded)return;
        effectsSucceeded=bloodEffects.stepChecked(phase,[&](auto,const BloodEffect& effect,bool& allowed){
            return TimeStopBound::evaluate(pTimeStop,effect.x.promoted(),effect.y.promoted(),roomWidth,
                [&](auto,auto,bool& inField){
                    if(levelExitPresent){
                        if(!BloodGoalContact::query(effect,goalField,inField))return false;
                        if(inField)return true;
                    }
                    if(startFieldPresent){
                        if(!BloodGoalContact::query(effect,startField,inField))return false;
                        if(inField)return true;
                    }
                    return otherFields(effect,inField);
                },allowed);
        });
    }
    void advanceSmokeAnimation(){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.advanceAnimation(smokeEffects.beginPhase());
    }
    void stepSmoke(SmokeEffects<smokeCapacity>::Phase phase,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.step(phase,random);
    }
    void collideSmokeShopWalls(){
        collideSmokeWalls([](const ContactBounds&,auto){return true;});
    }
    template<class Terrain> void collideSmokeWalls(Terrain terrain){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.collideWalls(smokeEffects.beginPhase(),
            [&](const ContactBounds& bounds,auto emit){
                return terrain(bounds,emit) && shops.visitWallContacts(bounds,emit);
            });
    }
    bool emitSmokeBurst(Fixed16 x,Fixed16 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.skeletonBurst(x,y,random);
        return effectsSucceeded;
    }
    // The box script supplies y+4; do not add its offset a second time here.
    bool emitBoxSmoke(Fixed16 x,Fixed16 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.boxBurst(x,y,random);
        return effectsSucceeded;
    }
    SideCameraLockers sideCameraLockers;
    bool gemHigh = false;
    // global.pGunType: styleUpdate selects the machine gun; GunModule
    // pickups change it for the rest of the run, across levels.
    int gunType = 0;
    // Begin Step owns this selection; pickups later in the tick must not
    // change it early. Retain the derived profile until its input changes.
    unsigned weaponSelectionStreak = 0;
    bool weaponSelectionValid = false, weaponSelectionHigh = false;
    void selectWeaponLevel(const CurrencyAccount& currency) {
        gemHigh = machineGunProfileForStreak(currency.streak).level != 0;
    }
    // Pickup-only upgrade effects: nonGemGainGem (combo rewards) does not
    // invoke scrRechargeGem. Ownership is sampled at collection, not birth.
    // W2.3 sounds: a floor bounce below the view top (viewTopQ8: the view's
    // top relative to the gem's origin, in Q8; the default never plays),
    // Alarm 2's sfxGemDisappear for an unattracted expiry, and scrGainGem's
    // pickup (sfxUpgradeGem_pickup_shoot with pugGemPop, else sfxGemGet) and
    // scrRechargeGem's recharge.
    template<class Solid, class Random>
    CurrencyStepResult collectCurrency(GemMotion& motion, bool large, unsigned amount,
        PlayerMotion& player, CurrencyAccount& currency, Solid solid, Random& random,
        long viewTopQ8=0x7fffffffL) {
        const bool attracted=motion.attracted;
        const auto result = stepCurrency(motion, large, amount,
            player.frameStartXFixed(), player.frameStartYFixed(),
            player.offsetXFixed(), player.offsetYFixed(), currency, solid, random,
            upgrades.levels[20] != 0);
        if(result==CurrencyStepResult::expired){
            if(!attracted)soundPlay(166,30,false,true);
            return result;
        }
        if(motion.bounced&&GemMotion::q8(motion.y)>viewTopQ8)soundPlay(motion.sound.bounceSound,30,false,true);
        if (result == CurrencyStepResult::collected) {
            gemStreakRise.reset();
            playGemPickup();
            if(upgrades.levels[4]&&player.rechargeFromGem(amount))soundPlay(27,90,false,true);
        }
        return result;
    }
    void playGemPickup(){
        if(upgrades.levels[16])soundPlay(341,70,false,true);
        else soundPlayOL(167,60,false,true,"UI");
    }
    // objCurrency's stopped branch (pTimeStop outside fields) skips motion
    // and alarms; collection by the player still sees the gem.
    CurrencyStepResult collectFrozenCurrency(const GemMotion& motion,bool large,unsigned amount,
        PlayerMotion& player,CurrencyAccount& currency){
        if(!motion.collectible(player.offsetXFixed(),player.offsetYFixed(),large))return CurrencyStepResult::alive;
        currency.collect(amount,300,upgrades.levels[20]!=0);
        gemStreakRise.reset();
        playGemPickup();
        if(upgrades.levels[4]&&player.rechargeFromGem(amount))soundPlay(27,90,false,true);
        return CurrencyStepResult::collected;
    }
    // Shop Create snapshots prices from run ownership; acquiring a card later
    // does not reprice already-created stock. Hard mode remains separate
    // from the current normal run adapter.
    // global.playStyle as the builder saw it (style 4 discounts stock).
    unsigned playStyle = 0;
    bool hardMode = false; // global.hardMode, chosen at style select
    template<class Range, class Choice>
    ShopWorld::Instance* createShop(int x, int y, Range random, Choice chooseFarewell, int health) {
        return shops.create(x, y, random, chooseFarewell, health, stage.area > 0 ? stage.area : 1,
                            hardMode, playStyle, upgrades.levels[14] != 0);
    }
    template<class Range,class Choice,class Owner>
    ShopWorld::Instance* createRegisteredShop(int x,int y,Range random,Choice chooseFarewell,int health,Owner& owner){
        return shops.createRegistered(x,y,random,chooseFarewell,health,stage.area>0?stage.area:1,
            hardMode,playStyle,upgrades.levels[14]!=0,owner,effectsSucceeded);
    }
    template<class Range,class Choice,class Owner,class Lookup>
    ShopWorld::Instance* createRegisteredShop(int x,int y,Range random,Choice chooseFarewell,int health,Owner& owner,Lookup& lookup){
        return shops.createRegisteredWithText(x,y,random,chooseFarewell,health,stage.area>0?stage.area:1,
            hardMode,playStyle,upgrades.levels[14]!=0,owner,effectsSucceeded,lookup);
    }
    LevelExit levelExit;
    GoalTimeField goalField;
    GoalTimeField startField;
    bool startFieldPresent=false;
    // startTimeField Create does not call inherited Create. Its radius draw
    // precedes first-level destruction, so even an absent field advances RNG.
    bool createStartField(int x,int y,GameMakerRandom& random){
        if(!effectsSucceeded)return false;
        startField.create(x,y,random);
        startFieldPresent=!(stage.area==1&&stage.level==1);
        return true;
    }
    unsigned goalFieldBirthBoundary = 0;
    bool levelExitPresent = false;
    // Boolean parentTimeField contact, distinct from instance_place identity.
    // Ordering cannot change the union of these supported, active masks. Do
    // not use the first tested field as the shard burst's selected instance.
    // Callers supply the source event's coordinates and current mask/scales.
    bool parentTimeFieldContact(TimeStopGoalContact::Mask mask,
        MotionBinary64 x, MotionBinary64 y, RunnerImageValue xs,
        RunnerImageValue ys, bool& hit) {
        if (!effectsSucceeded) return false;
        if (!levelExitPresent && !startFieldPresent && !timeFields.size()) { hit = false; return true; }
        RunnerImageValue px, py;
        effectsSucceeded = px.assign(x) && py.assign(y);
        if (!effectsSucceeded) return false;
        bool found = false;
        for (unsigned i = 0; i < timeFields.size(); ++i) {
            effectsSucceeded = TimeStopGoalContact::query(mask, px, py, xs, ys,
                *timeFields.at(i), found);
            if (!effectsSucceeded) return false;
            if (found) { hit = true; return true; }
        }
        if (levelExitPresent) {
            effectsSucceeded = TimeStopGoalContact::query(mask, px, py, xs, ys, goalField, found);
            if (!effectsSucceeded) return false;
        }
        if(!found&&startFieldPresent){
            effectsSucceeded=TimeStopGoalContact::query(mask,px,py,xs,ys,startField,found);
            if(!effectsSucceeded)return false;
        }
        hit = found;
        return true;
    }
    // Shared field adapter. Callers choose the source event's position: clock
    // checks use old published x/y; transitions use private xx/yy. Never merge
    // those queries or infer their answer from goalStopped.
    bool playerGoalContact(MotionBinary64 x,MotionBinary64 y,int facing,bool& hit) {
        if(!effectsSucceeded)return false;
        if(!levelExitPresent&&!startFieldPresent){hit=false;return true;}
        RunnerImageValue px,py,sx,sy;
        effectsSucceeded=px.assign(x) && py.assign(y) &&
            sx.assign(MotionBinary64::fromInteger(facing<0?-1:1)) &&
            sy.assign(MotionBinary64::fromInteger(1));
        if(!effectsSucceeded)return false;
        // Published integral positions with unit scales have exactly the
        // rectangle used by the compact goal query (also when mirrored).
        // Keep clock checks cheap on 68000; fractional transitions use the
        // original transformed sampling path below.
        std::int32_t ix,iy;
        RunnerImageValue rx,ry;
        const bool integral=px.roundPixel(ix) && py.roundPixel(iy) &&
           rx.assign(MotionBinary64::fromInteger(ix)) && ry.assign(MotionBinary64::fromInteger(iy)) &&
           rx.bits==px.bits && ry.bits==py.bits &&
           ix>=-1048576 && ix<=1048576 && iy>=-1048576 && iy<=1048576;
        bool found=false;
        const auto query=[&](const GoalTimeField& field){
            if(integral&&field.x>=-1048576&&field.x<=1048576&&field.y>=-1048576&&field.y<=1048576){
                found=field.meetsPlayer(ix,iy);return true;
            }
            effectsSucceeded=TimeStopGoalContact::query(TimeStopGoalContact::Mask::Player,px,py,sx,sy,field,found);
            return effectsSucceeded;
        };
        if(levelExitPresent&&!query(goalField))return false;
        if(!found&&startFieldPresent&&!query(startField))return false;
        hit=found;
        return true;
    }
    // playerGoalContact at whole published pixels. Within +/-2^20 (player and
    // fields) each coordinate narrows exactly, so the query is each field's
    // rectangle test; farther positions take the transformed sampling path,
    // which world one does not link.
    bool playerGoalContactPixels(int x,int y,int facing,bool& hit){
        if(!effectsSucceeded)return false;
        if(!levelExitPresent&&!startFieldPresent){hit=false;return true;}
        const auto near=[](int px,int py){return px>=-1048576&&px<=1048576&&py>=-1048576&&py<=1048576;};
        if(near(x,y)&&(!levelExitPresent||near(goalField.x,goalField.y))&&
           (!startFieldPresent||near(startField.x,startField.y))){
            hit=(levelExitPresent&&goalField.meetsPlayer(x,y))||(startFieldPresent&&startField.meetsPlayer(x,y));
            return true;
        }
#ifdef DOWNWELL_WORLD_ONE_ONLY
        (void)facing;return effectsSucceeded=false;
#else
        return playerGoalContact(MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),facing,hit);
#endif
    }
    bool goalStopped = false, exitControlsLocked = false;
    // global.pTimeStop as published by a compact world's player Begin Step.
    // The registered SDL owner keeps its own TimeStopEffects and leaves it false.
    bool timeStopped = false;
    bool exitDitherRequested = false, levelResultRequested = false;
    ExitDither exitDither;
    LevelClearController levelClear;
    LevelClearPresentation::Frame levelClearFrame;
    bool levelClearActive = false, nextLevelRequested = false;

    // One logical Draw phase per gameplay update, not per renderer repaint.
    // The source consumes field RNG even outside its camera drawing bounds.
    void gameplayPresentationStep(GameMakerRandom& random, bool timeStopped = false) {
        if (!effectsSucceeded) return;
        effectsSucceeded = timeFields.advanceDraw(timeStopped, random,
            levelExitPresent, goalFieldBirthBoundary,
            [&] { goalField.advanceDraw(timeStopped, random); });
        if (!effectsSucceeded) return;
        // The placed start field predates generated ordinary/exit fields.
        if(startFieldPresent)startField.advanceDraw(timeStopped,random);
        results.presentationStep();
        if(results.active)playResultsDrawSounds();
    }
    // W2.3 DeathMenu sounds. Step and alarms (after results.beginStep):
    // sfxUiMove_Up_in_menu per cursor edge, Alarm 0's sfxUiNumbers_Appear and
    // Alarm 1's sfxUiProgress_Bar_Celebration.
    void playResultsStepSounds(){
        if(!results.active)return;
        for(unsigned n=results.inputEffects.navigationSounds;n;--n)soundPlayOL(324,90,false,true,"UI");
        if(results.alarmSounds&1)soundPlayOL(325,90,false,true,"UI");
        if(results.alarmSounds&2)soundPlayOL(327,90,false,true,"UI");
    }
    // Create's sfxUiGame_Over_Appear when the results open.
    void playResultsOpenSound(bool wasActive){
        if(results.active&&!wasActive)soundPlayOL(319,90,false,true,"UI");
    }
    // Draw: the fill blip at 1 + (progBarLength / 96) * 1.2 from the bar's
    // pre-fill length, the finish sound, and the goal's pop.
    void playResultsDrawSounds(){
        const auto& f=results.frame;
        if(f.fillSound){
            const int voice=soundPlayOL(328,90,false,true,"UI");
            // progBarLength / 96 = (fake - previous goal) / (next - previous)
            // with the pre-fill total (the frame's displayed total).
            const auto span=f.nextGoal-f.previousGoal,done=f.displayedTotal-f.previousGoal;
            if(span&&span<0x7fffffffULL&&done<0x7fffffffULL){
                // Q16 done / span (32-bit long division), times 1.2
                // (78643/65536), plus 1.
                const std::uint32_t d=std::uint32_t(span),whole=std::uint32_t(done)/d;
                std::uint32_t rest=std::uint32_t(done)%d,unit=whole<<16;
                for(std::uint32_t bit=0x8000u;bit;bit>>=1){rest<<=1;if(rest>=d){rest-=d;unit|=bit;}}
                const std::uint64_t scaled=(std::uint64_t(unit)*78643u+0x8000u)>>16;
                if(whole<256u&&scaled<=255u*65536u)setSoundPitch(voice,AudioLevel::raw(std::int32_t(65536u+scaled)));
            }
        }
        if(f.completedSound)soundPlayOL(330,80,false,true,"UI");
        if(f.unlockedGoal>=0)soundPlayOL(329,90,false,true,"UI");
    }

    // Result Create: pause the song, then fade the whoosh in (not in area 3).
    int levelClearWhoosh=-1;
    void playLevelClearOpenSounds(){
        bool playing=false;
        if(voices.isPlaying(backgroundMusicId,playing)&&playing)audio().pause(backgroundMusicId);
        levelClearWhoosh=stage.area!=3?soundPlayOL(186,80,true,true,"UI"):-1;
        if(voices.playing(levelClearWhoosh)){
            setSoundGain(levelClearWhoosh,AudioLevel{});
            setSoundGain(levelClearWhoosh,audio_level::one,1000);
        }
    }
    // scrResult, scrGainUg/gainHp and Result Alarm 0/1 sounds.
    void playLevelClearEffectSound(LevelClearSelection::Effect event,unsigned value){
        using E=LevelClearSelection::Effect;
        switch(event){
        case E::moveSound:soundPlayOL(338,90,false,true,"UI");break;
        case E::grantChoice:{
            const unsigned t=levelClear.offers.ids[value];
            if(t==13||t==6)soundPlay(3,90,false,true);
            if(t==21)soundPlay(167,60,false,true);
            break;
        }
        case E::confirmSound:soundPlayOL(339,90,false,true,"UI");break;
        case E::fadeWhoosh:
            if(voices.playing(levelClearWhoosh))setSoundGain(levelClearWhoosh,AudioLevel{},1500);
            break;
        case E::nextLevel:stopSound(levelClearWhoosh);break;
        default:break;
        }
    }
    // objTransDither Alarm 0: soundPlayOL(187, 80, 0, 1, "UI").
    void advanceExitDitherAlarm(){
        const bool before=exitDither.soundRequested;
        exitDither.advanceAlarm();
        if(exitDither.soundRequested&&!before)soundPlayOL(187,80,false,true,"UI");
    }

    // Called only after the backend retires the old room's display/DMA.
    // No run reset: upgrades, currency, records and stage survive this boundary.
    bool enterLevelClear(unsigned style, bool heartGoal, int health,
                         PlayerMotion& player, GameMakerRandom& random) {
        if (!levelResultRequested || levelClearActive) return false;
        if (!levelClear.create(style, heartGoal, health, upgrades, player, random)) return false;
        levelClearActive = true;
        playLevelClearOpenSounds();
        flashEffects.clear();
        wallBackground.clear();
        smokeEffects.clear();
        bloodEffects.clear();
        clearCorpseExpiry();
        clearDamageNumbers();
        if(!timeFields.clear()){effectsSucceeded=false;return false;}
        // Room creation is followed by Draw before the next Step/Alarm phase.
        levelClearFrame = levelClear.presentation.advance(stage.area, stage.level,
            levelClear.selection.next, levelClear.selection.visible);
        levelResultRequested = exitDitherRequested = false;
        levelExitPresent = false;
        startFieldPresent = false;
        exitDither = {};
        nextLevelRequested = false;
        return true;
    }
    template<class Emit>
    bool stepLevelClear(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
                        CurrencyAccount& currency, PlayerMotion& player, int& health,
                        GameMakerRandom& random, Emit emit) {
        return stepLevelClearPublished(confirm,leftPressed,rightPressed,heartGoal,currency,player,health,
            random,emit,[](int,int){return true;});
    }
    template<class Emit,class DitherBirth>
    bool stepLevelClearPublished(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
                        CurrencyAccount& currency, PlayerMotion& player, int& health,
                        GameMakerRandom& random, Emit emit,DitherBirth birth) {
        return stepLevelClearScheduled(confirm,leftPressed,rightPressed,heartGoal,currency,player,health,
            random,emit,birth,[&](auto event){
                advanceExitDitherAlarm();return levelClear.advanceAlarmsChecked(event);
            });
    }
    template<class Emit,class DitherBirth,class Alarms>
    bool stepLevelClearScheduled(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
                        CurrencyAccount& currency, PlayerMotion& player, int& health,
                        GameMakerRandom& random, Emit emit,DitherBirth birth,Alarms alarms) {
        return stepLevelClearPositioned(confirm,leftPressed,rightPressed,heartGoal,currency,player,health,
            random,emit,birth,alarms,[](std::int32_t){return true;},[]{return true;});
    }
    // W2.3: scrPlMovement's sound scripts, in source order, right after the
    // player's motion. Only their RNG draws and audio_is_playing reads change
    // gameplay. floor is the material index (soundFootstep's switch:
    // breakable, thin, foliage, rock, metal) of instance_place(x, y + 2,
    // sParentSolid); every table's irandom_range(1, count) consumes two words
    // whatever the count. fullStepGain: pTimeStop || groundRoom() || atPit.
    struct PlayerFloor { int operator()()const{return 2;} };
    struct NoRocketJump { bool operator()(PlayerMotion&)const{return true;} };
    template<class Floor=PlayerFloor,class Rocket=NoRocketJump>
    bool playPlayerSounds(PlayerMotion& player,GameMakerRandom& random,bool dead,Floor floor={},bool fullStepGain=false,
                          Rocket rocket={}){
        using Event=PlayerMotion::SoundEvent;
        // initSfxFs / initSfxJl: global.sfxFs[i] and global.sfxJl[i].
        static const std::uint8_t steps[5][5]={{3,48,49,50},{3,63,64,65},{4,51,52,53,54},{4,55,56,57,58},{4,59,60,61,62}};
        static const std::uint8_t lands[5][5]={{3,66,67,68},{2,81,82},{4,69,70,71,72},{4,73,74,75,76},{4,77,78,79,80}};
        const auto material=[&]{const int m=floor();return m<0||m>4?0:m;};
        for(unsigned i=0;i<player.soundEventCount();++i){
            switch(player.soundEvent(i)){
            case Event::jump:case Event::wallKick:
                random.chooseIndexSmall(1); // soundPlay(choose(86), 80, 0, 1)
                soundPlay(86,80,false,true);
                break;
            case Event::land:
                // soundLand: landOn && !global.death && !napping, then only
                // while sfxGunRecharge is silent.
                if(!dead&&!voices.playing(27)){
                    const auto& table=lands[material()];
                    soundPlayOL(table[1+random.irandomSmall(table[0]-1u)],60,false,true,"footsteps");
                }
                break;
            case Event::footstep:{
                const auto& table=steps[material()];
                soundPlayOL(table[1+random.irandomSmall(table[0]-1u)],60,false,true,"footsteps");
                // sndgain = audio_sound_get_gain(sndsnd) * (1 or 0.7).
                AudioLevel gain;
                if(voices.getGain(soundCaller.sndsnd,gain)){
                    soundCaller.sndgain=fullStepGain?gain:audio_level::product(gain,AudioLevel::raw(45875));
                    setSoundGain(soundCaller.sndsnd,soundCaller.sndgain);
                }
                break;
            }
            case Event::hardLandTurn:
                soundPlay(88,30,false,true); // no draw
                break;
            case Event::rocket:
                if(!rocket(player))return effectsSucceeded=false;
                break;
            }
        }
        startRechargeSounds(player); // the landing's scrWallCol recharge follows
        return effectsSucceeded;
    }
    // soundPlay(27, 90, 0, 1) for each successful scrRecharge since the last
    // call: the landing, stomps and contact knockback (scrFjump).
    void startRechargeSounds(PlayerMotion& player){
        for(unsigned n=player.takeRechargeSounds();n;--n)soundPlay(27,90,false,true);
    }
    // goalTimeField 255 (and its child startTimeField) Collision with
    // objPlayer_n at the published player position: a new voidWind once the
    // last one ends. gameVolume is 1 until the options model it.
    bool goalWindPossible()const{
        // The contact's first act is audio_is_playing(global.voidWind); a
        // playing wind makes the overlap unobservable, so skip its query.
        return (levelExitPresent||startFieldPresent)&&!voices.playing(fieldWind.voice);
    }
    bool collideGoalFieldWind(GameMakerRandom& random,int x,int y,int facing){
        if(!effectsSucceeded)return false;
        if(!goalWindPossible())return true;
        bool hit=false;
        if(!playerGoalContactPixels(x,y,facing,hit))return effectsSucceeded=false;
        return !hit||resultStepWind(random);
    }
    // Result Step after scrResult, and the goal fields' contact.
    bool resultStepWind(GameMakerRandom& random){
        std::int32_t sound=0;
        auto a=audio(SoundPlayScript::channel("UI"),true);
        if(!fieldWind.contact(random,sound,overlapSounds[8],audio_level::one,a))
            return effectsSucceeded=false;
        return true;
    }
    struct NoResultStepTail { bool operator()()const{return true;} };
    // ResultTail is Result Step's code after scrResult (W2.3: its voidWind).
    template<class Emit,class DitherBirth,class Alarms,class Position,class Wipe,class ResultTail=NoResultStepTail>
    bool stepLevelClearPositioned(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
                        CurrencyAccount& currency, PlayerMotion& player, int& health,
                        GameMakerRandom& random, Emit emit,DitherBirth birth,Alarms alarms,Position position,Wipe wipe,
                        ResultTail resultTail={}) {
        if (!effectsSucceeded || !levelClearActive || nextLevelRequested) return false;
        const auto effect = [&](LevelClearSelection::Effect event, unsigned value) {
            if (event == LevelClearSelection::Effect::dither) {
                if(!birth(0,levelClear.motion.viewY())||!effectsSucceeded){effectsSucceeded=false;return;}
                exitDitherRequested = true;
                exitDither.create(0, levelClear.motion.viewY());
            }
            if (event == LevelClearSelection::Effect::nextLevel) nextLevelRequested = true;
            playLevelClearEffectSound(event,value);
            emit(event, value);
        };
        if(!alarms([&](auto event,unsigned value){
            effect(event,value);return effectsSucceeded;
        }))return effectsSucceeded=false;
        // Ordinary Step uses ascending resource IDs: objTransDither (68)
        // before Result (325). Its view read precedes scrResult's new yy.
        exitDither.step(levelClear.motion.viewY(),284);
        if(!wipe()||!effectsSucceeded)return effectsSucceeded=false;
        const bool ok = levelClear.step(confirm, leftPressed, rightPressed, heartGoal,
            upgrades, consumables, currency, player, health, combo.healthReserve, random, effect);
        if(!ok||!effectsSucceeded)return effectsSucceeded=false;
        std::int32_t y;
        if(!levelClear.motion.instanceY(y)||!position(y)||!effectsSucceeded)return effectsSucceeded=false;
        if(!resultTail()||!effectsSucceeded)return effectsSucceeded=false;
        levelClearFrame = levelClear.presentation.advance(stage.area, stage.level,
            levelClear.selection.next, levelClear.selection.visible);
        return ok;
    }
    // The loader consumes this once before constructing rmMain, matching
    // scrNextLevel(0). It must not use restart(), which clears run ownership.
    bool advanceAfterLevelClear() {
        if (!levelClearActive || !nextLevelRequested) return false;
        stage.advance();
        levelClearActive = nextLevelRequested = false;
        goalStopped = exitControlsLocked = false;
        exitDitherRequested = false;
        exitDither = {};
        return true;
    }

    void advanceLevelExitAlarms(int viewX, int viewY) {
        if (!levelExitPresent) return;
        advanceExitDitherAlarm();
        levelExit.advanceAlarms([&](LevelExit::Effect effect, int, int) {
            if (effect == LevelExit::Effect::ditherTransition) {
                exitDitherRequested = true;
                exitDither.create(viewX, viewY);
            }
            if (effect == LevelExit::Effect::resultRoom) levelResultRequested = true;
        });
    }
    void collideLevelExit(int playerX, int playerY, bool dead) {
        if (levelExitPresent && levelExit.meetsPlayer(playerX, playerY))
            levelExit.collide(dead, goalStopped, exitControlsLocked);
    }

    template<class Camera,class PublishChild=LevelExit::NoChildPublication>
    bool createLevelExit(int roomWidth, int y, GameMakerRandom& random, Camera& camera,
                         PublishChild publishChild={}) {
        if(!effectsSucceeded)return false;
        levelExitPresent = false;
        goalStopped = exitControlsLocked = false;
        exitDitherRequested = levelResultRequested = false;
        exitDither = {};
        levelExit.create(roomWidth, y, [&](LevelExit::Effect effect, int x, int childY) {
            // Child birth and collision-property writes precede its remaining
            // Create effects. In particular the goal field is scaled before
            // radius consumes RNG. A failed child stops later child creation.
            if(!effectsSucceeded)return;
            if(!publishChild(effect,x,childY)){effectsSucceeded=false;return;}
            if (effect == LevelExit::Effect::roomEndSetter) camera.setLevelExit(childY);
            else if (effect == LevelExit::Effect::goalTimeField) {
                goalFieldBirthBoundary = timeFields.size();
                goalField.create(x, childY, random);
            }
        });
        levelExitPresent = effectsSucceeded;
        return effectsSucceeded;
    }

    RunRecords records() const {
        return {gems.lifetime, gems.record, combo.state.record};
    }
    void reset(const RunRecords& saved = {}) {
        fallDepth={};
        oxygen=Fixed16::fromInteger(100);
        for(auto& sound:overlapSounds)sound=0;
        fieldWind={};
        audio().stopAll();
        gems = {};
        gems.lifetime = saved.lifetimeGems;
        gems.record = saved.bestRunGems;
        clock = {};
        combo = {};
        kills = 0;
        noKill = false;
        noDamage=true; // scrGlobalStuff does not write achAreaNoDamage.
        playerDamaged=false;
        combo.state.record = saved.bestCombo;
        stage = {};
        results = {};
        ammoHud = {};
        ammoGauge = {};
        consumables = {};
        upgrades = {};
        bodiesEaten = 0;
        shops.reset();
        wallBackground.clear();
        flashEffects.clear();
        smokeEffects.clear();
        bloodEffects.clear();
        clearCorpseExpiry();
        clearDamageNumbers();
        if(!timeFields.clear()){effectsSucceeded=false;return;}
        effectsSucceeded=true;
        if(!sideCameraLockers.clear()){effectsSucceeded=false;return;}
        gemHigh = false;
        gunType = 0;
        levelExit = {};
        goalField = {};
        startField = {};startFieldPresent=false;
        goalFieldBirthBoundary = 0;
        levelExitPresent = false;
        goalStopped = exitControlsLocked = false;
        exitDitherRequested = levelResultRequested = false;
        exitDither = {};
        levelClear = {};
        levelClearFrame = {};
        levelClearActive = nextLevelRequested = false;
    }
    ResultsDestination restart(DeathMenuSequence::Action action, const RunRecords& saved) {
        const auto destination = resultsDestination(action);
        if (destination == ResultsDestination::none) return destination;
        reset(saved);
        if (destination == ResultsDestination::gameplay) stage.advance(1);
        return destination;
    }
};
} // namespace downwell
#endif
