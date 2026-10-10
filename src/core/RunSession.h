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
        MotionBinary64 volume,Audio& audio){
        if(!effectsSucceeded)return false;
        SoundPlayEnvironment env;env.gameVolume=volume;
        int result=-1;bool returned=false;
        // UI (channel 8) neither tests the view nor applies underwater pitch.
        return SoundPlayScript::overlap(caller,env,audio,[]{return false;},overlapSounds[8],
            167,60,false,true,"UI",result,returned,effectsSucceeded);
    }
    template<class Audio> bool playBoxChainSound(SoundPlayState& caller,
        MotionBinary64 volume,bool inWater,Audio& audio){
        SoundPlayEnvironment env;env.gameVolume=volume;env.inWater=inWater;
        int result=-1;
        return SoundPlayScript::plain(caller,env,audio,342,50,false,true,result,effectsSucceeded);
    }
    template<class Audio> bool playBoxBreakSound(SoundPlayState& caller,int sound,
        MotionBinary64 volume,Audio& audio){
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
    MotionBinary64 oxygen=MotionBinary64::fromInteger(100); // scrPlayerGlobalStat.
    bool setOxygen(MotionBinary64 value){
        if(!effectsSucceeded)return false;
        if(!value.valid())return effectsSucceeded=false;
        oxygen=value;return true;
    }
    bool floorTimeStopOxygen(){
        if(!effectsSucceeded)return false;
        if(!oxygen.valid())return effectsSucceeded=false;
        // Positive finite binary64 bit order is numeric order; all negatives
        // and signed zero are below ten. Preserve values above the floor.
        if((oxygen.high&0x80000000u)||oxygen.high<0x40240000u)
            oxygen=MotionBinary64::fromInteger(10);
        return true;
    }
    template<class Audio> bool duckMusic(MusicDuck& caller,int duration,MotionBinary64 amount,Audio& audio){
        if(!effectsSucceeded)return false;
        return effectsSucceeded=caller.execute(duration,amount,[&]{return stage.area;},
            [&]{return backgroundMusicId;},audio);
    }
    static bool areaMusic(int area,int& sound){
        if(area<1||area>5)return false;
        sound=193+area;return true;
    }
    void playerCreated(){noKill=true;noDamage=true;areaNoDamage=true;firstLand=false;oxygen=MotionBinary64::fromInteger(100);playerDamaged=false;}
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
        if(!audio.setGain(bgm,MotionBinary64::fromInteger(1),0)||!effectsSucceeded)return effectsSucceeded=false;
        int result=-1;bool returned=false;
        return SoundPlayScript::overlap(caller,env,audio,[]{return false;},overlapSounds[5],
            bgm,100,true,true,"music",result,returned,effectsSucceeded);
    }
    DeathResults results;
    AmmoHudMotion ammoHud;
    AmmoGaugeRows ammoGauge;
    Consumables consumables;
    UpgradeInventory upgrades;
    ShopWorld shops;
    WallBackgroundTiles<2048> wallBackground;
    bool addWallBackground(MotionBinary64 x,MotionBinary64 y){
        if(!effectsSucceeded)return false;
        return effectsSucceeded=wallBackground.append(x,y);
    }
    // Room-owned effects are reset on run/room retirement, not on a repaint.
    FlashBallEffects<32> flashEffects;
    SmokeEffects<32> smokeEffects;
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
    std::uint32_t createCorpseExpiry(MotionBinary64 x,MotionBinary64 y){
        if(!effectsSucceeded)return 0;
        if(!nextForegroundBirth){effectsSucceeded=false;return 0;}
        ExpiryHooks hooks;
        const auto child=corpseExpiryEffects.create(x,y,hooks);
        effectsSucceeded=child!=0;
        if(child)corpseExpiryEffects.get(child)->drawBirth=nextForegroundBirth++;
        return child;
    }
    bool emitCorpseExpiry(MotionBinary64 x,MotionBinary64 y){
        return createCorpseExpiry(x,y)!=0;
    }
    // Shared alarm adapter: complete child Create, assign the source-selected
    // sprite, then request parent retirement. The scheduler supplies verified
    // nonempty unit-track frame counts and lifetime removal, not event rules.
    template<class Frames,class Destroy>
    bool advanceCorpseAlarm(CorpseLifecycle& corpse,unsigned index,Frames frames,Destroy destroy){
        if(!effectsSucceeded)return false;
        struct Events {
            RunSession& session;Frames& frames;Destroy& retire;
            bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& child){
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
    bool emitFlashBurst(MotionBinary64 x,MotionBinary64 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=flashEffects.skeletonBurst(x,y,random);
        return effectsSucceeded;
    }
    bool emitBoxFlash(MotionBinary64 x,MotionBinary64 y,GameMakerRandom& random){
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
    void stepSmoke(SmokeEffects<32>::Phase phase,GameMakerRandom& random){
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
    bool emitSmokeBurst(MotionBinary64 x,MotionBinary64 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.skeletonBurst(x,y,random);
        return effectsSucceeded;
    }
    // The box script supplies y+4; do not add its offset a second time here.
    bool emitBoxSmoke(MotionBinary64 x,MotionBinary64 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.boxBurst(x,y,random);
        return effectsSucceeded;
    }
    SideCameraLockers sideCameraLockers;
    bool gemHigh = false;
    void selectWeaponLevel(const CurrencyAccount& currency) {
        gemHigh = machineGunProfileForStreak(currency.streak).level != 0;
    }
    // Pickup-only upgrade effects: nonGemGainGem (combo rewards) does not
    // invoke scrRechargeGem. Ownership is sampled at collection, not birth.
    template<class Solid, class Random>
    CurrencyStepResult collectCurrency(GemMotion& motion, bool large, unsigned amount,
        PlayerMotion& player, CurrencyAccount& currency, Solid solid, Random& random) {
        const auto result = stepCurrency(motion, large, amount,
            player.frameStartXFixed(), player.frameStartYFixed(),
            player.offsetXFixed(), player.offsetYFixed(), currency, solid, random,
            upgrades.levels[20] != 0);
        if (result == CurrencyStepResult::collected) {
            gemStreakRise.reset();
            if(upgrades.levels[4])player.rechargeFromGem(amount);
        }
        return result;
    }
    // Shop Create snapshots prices from run ownership; acquiring a card later
    // does not reprice already-created stock. Other style/difficulty modes
    // remain separate from the current usual-style, normal run adapter.
    template<class Range, class Choice>
    ShopWorld::Instance* createShop(int x, int y, Range random, Choice chooseFarewell, int health) {
        return shops.create(x, y, random, chooseFarewell, health, stage.area > 0 ? stage.area : 1,
                            false, 0, upgrades.levels[14] != 0);
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
        const bool integral=px.promoted().roundPixel(ix) && py.promoted().roundPixel(iy) &&
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
    bool goalStopped = false, exitControlsLocked = false;
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
    }

    // Called only after the backend retires the old room's display/DMA.
    // No run reset: upgrades, currency, records and stage survive this boundary.
    bool enterLevelClear(unsigned style, bool heartGoal, int health,
                         PlayerMotion& player, GameMakerRandom& random) {
        if (!levelResultRequested || levelClearActive) return false;
        if (!levelClear.create(style, heartGoal, health, upgrades, player, random)) return false;
        levelClearActive = true;
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
                exitDither.advanceAlarm();return levelClear.advanceAlarmsChecked(event);
            });
    }
    template<class Emit,class DitherBirth,class Alarms>
    bool stepLevelClearScheduled(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
                        CurrencyAccount& currency, PlayerMotion& player, int& health,
                        GameMakerRandom& random, Emit emit,DitherBirth birth,Alarms alarms) {
        return stepLevelClearPositioned(confirm,leftPressed,rightPressed,heartGoal,currency,player,health,
            random,emit,birth,alarms,[](std::int32_t){return true;},[]{return true;});
    }
    template<class Emit,class DitherBirth,class Alarms,class Position,class Wipe>
    bool stepLevelClearPositioned(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
                        CurrencyAccount& currency, PlayerMotion& player, int& health,
                        GameMakerRandom& random, Emit emit,DitherBirth birth,Alarms alarms,Position position,Wipe wipe) {
        if (!effectsSucceeded || !levelClearActive || nextLevelRequested) return false;
        const auto effect = [&](LevelClearSelection::Effect event, unsigned value) {
            if (event == LevelClearSelection::Effect::dither) {
                if(!birth(0,levelClear.motion.viewY())||!effectsSucceeded){effectsSucceeded=false;return;}
                exitDitherRequested = true;
                exitDither.create(0, levelClear.motion.viewY());
            }
            if (event == LevelClearSelection::Effect::nextLevel) nextLevelRequested = true;
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
        exitDither.advanceAlarm();
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
        oxygen=MotionBinary64::fromInteger(100);
        for(auto& sound:overlapSounds)sound=0;
        fieldWind={};
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
