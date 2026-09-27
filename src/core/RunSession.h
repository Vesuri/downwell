#ifndef DOWNWELL_CORE_RUN_SESSION_H
#define DOWNWELL_CORE_RUN_SESSION_H
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
#include "PlayerDamageNumbers.h"
#include "RunnerPointDirection.h"
#include "PortableAtan.h"
#include "ForcedPlayerJump.h"
#include "Profile.h"
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
    RunGemStatistics gems;
    RunClock clock;
    RunCombo combo;
    // global.killCount: scrGlobalStuff resets it for a new run, not rmMain
    // entry. DeathMenu/EndMenu display the cumulative total across levels.
    std::uint32_t kills = 0;
    RunStage stage;
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
        const auto px = coordinate(playerX, originX), py = coordinate(playerY, originY);
        auto cx = MotionBinary64::fromInteger(callerX), cy = MotionBinary64::fromInteger(callerY);
        MotionBinary64 direction;
        bool directed;
        {
            DW_PROFILE_SCOPE(profileContactDirection);
            directed=cx.add(MotionBinary64::fromInteger(originX)) &&
                cy.add(MotionBinary64::fromInteger(originY)) &&
                RunnerPointDirection::evaluate(cx,cy,px,py,direction,portableAtan);
        }
        if (!directed) {
            effectsSucceeded = false;
            return false;
        }
        return createDamageNumber(px,py,MotionBinary64::fromInteger(1),direction) != 0;
    }
    template<class LengthdirX> bool stepDamageNumbers(PlayerDamageNumbers<8>::Phase phase,LengthdirX lengthdirX){
        DamageNumberHooks hooks;
        if(effectsSucceeded)effectsSucceeded=damageNumbers.step(phase,lengthdirX,hooks);
        return effectsSucceeded;
    }
    bool stepDamageNumbers(PlayerDamageNumbers<8>::Phase phase){
        DW_PROFILE_SCOPE(profileDamageNumberStep);
        return stepDamageNumbers(phase,[](MotionBinary64 speed,MotionBinary64 direction,MotionBinary64& x){
            RunnerImageValue length,angle,radian;
            MotionBinary64 cosine;
            return length.assign(speed)&&angle.assign(direction)&&SmokeDirection::radians(angle,radian)&&
                smokeCosine(radian.promoted(),cosine)&&SmokeDirection::component(length,cosine,false,x);
        });
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
    bool adoptTimeField(int x,int y,std::uint32_t radiusWord){
        if(effectsSucceeded)effectsSucceeded=timeFields.adoptCreate(x,y,radiusWord);
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
        if (result == CurrencyStepResult::collected && upgrades.levels[4])
            player.rechargeFromGem(amount);
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
        timeFields.clear();
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
        if (!levelClearActive || nextLevelRequested) return false;
        exitDither.advanceAlarm();
        const auto effect = [&](LevelClearSelection::Effect event, unsigned value) {
            if (event == LevelClearSelection::Effect::dither) {
                exitDitherRequested = true;
                exitDither.create(0, levelClear.motion.viewY());
            }
            if (event == LevelClearSelection::Effect::nextLevel) nextLevelRequested = true;
            emit(event, value);
        };
        levelClear.advanceAlarms(effect);
        const bool ok = levelClear.step(confirm, leftPressed, rightPressed, heartGoal,
            upgrades, consumables, currency, player, health, combo.healthReserve, random, effect);
        exitDither.step(levelClear.motion.viewY(), 284);
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
        gems = {};
        gems.lifetime = saved.lifetimeGems;
        gems.record = saved.bestRunGems;
        clock = {};
        combo = {};
        kills = 0;
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
        timeFields.clear();
        effectsSucceeded=true;
        sideCameraLockers.clear();
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
