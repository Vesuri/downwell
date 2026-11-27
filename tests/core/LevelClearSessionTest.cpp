#include "core/RunSession.h"
#include "core/TimeFieldOutlines.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    using namespace downwell;
    {
        RunSession session;GameMakerRandom random(42);PlayerMotion player;CurrencyAccount currency;
        int health=4;bool heart=false,confirm=false;session.levelResultRequested=true;
        require(session.enterLevelClear(0,false,health,player,random),"wipe view-read fixture");
        session.levelClear.motion.cameraTarget=HudReal::integer(340);
        session.exitDither.create(0,384); // y=284, exactly the old view bottom.
        require(session.stepLevelClear(confirm,false,false,heart,currency,player,health,random,[](auto,unsigned){}),
            "Result update with a boundary wipe succeeds");
        require(session.exitDither.y==284&&session.levelClear.motion.viewY()==20,
            "wipe Step reads the previous view before Result yy advances");
    }
    {
        RunSession stopped;GameMakerRandom random(42);PlayerMotion player;CurrencyAccount currency;
        int health=4;bool heart=false,confirm=false;
        stopped.levelResultRequested=true;
        require(stopped.enterLevelClear(0,false,health,player,random),"Result Step publication fixture");
        stopped.levelClear.motion.cameraTarget=HudReal::integer(340);
        stopped.exitDither.create(0,0);
        const auto time=stopped.levelClearFrame.time;
        unsigned writes=0;
        require(!stopped.stepLevelClearPositioned(confirm,false,false,heart,currency,player,health,random,
            [](auto,unsigned){},[](int,int){return true;},[&](auto event){
                stopped.exitDither.advanceAlarm();return stopped.levelClear.advanceAlarmsChecked(event);
            },[&](std::int32_t y){++writes;require(y==162,"publish rounded new yy after motion");return false;},[]{return true;}),
            "failed Result y setter stops the update");
        require(!stopped.effectsSucceeded&&writes==1&&stopped.exitDither.alarm0==14&&
            stopped.exitDither.y==-92&&stopped.levelClearFrame.time==time&&
            stopped.levelClear.motion.cameraY.floor()==161,
            "failed setter retains earlier alarms, wipe Step and motion but suppresses later presentation");
    }
    {
        RunSession stopped;GameMakerRandom random(42);PlayerMotion player;CurrencyAccount currency;
        int health=4;bool heart=false,confirm=true;
        stopped.levelResultRequested=true;
        require(stopped.enterLevelClear(0,false,health,player,random),"wipe birth failure fixture");
        stopped.levelClear.selection.alarm0=1;
        unsigned births=0,events=0;
        require(!stopped.stepLevelClearPublished(confirm,false,false,heart,currency,player,health,random,
            [&](auto,unsigned){++events;},[&](int x,int y){
                ++births;require(x==0&&y==0&&!stopped.exitDither.active&&
                    stopped.levelClear.selection.alarm1==-1,"registered birth precedes scalar wipe and Alarm 1");
                return false;
            })&&!stopped.effectsSucceeded&&births==1&&events==0&&
            !stopped.exitDither.active&&!stopped.exitDitherRequested&&stopped.levelClear.selection.alarm1==-1,
            "failed wipe publication stops the source alarm before later effects");
    }
    for(auto failed:{LevelClearSelection::Effect::dither,LevelClearSelection::Effect::fadeWhoosh,
                    LevelClearSelection::Effect::nextLevel}){
        RunSession stopped;GameMakerRandom random(42);PlayerMotion player;CurrencyAccount currency;
        int health=4;bool heart=false,confirm=true;
        stopped.levelResultRequested=true;
        require(stopped.enterLevelClear(0,false,health,player,random),"checked Result fixture entry");
        stopped.levelClear.selection.alarm0=failed==LevelClearSelection::Effect::nextLevel?-1:1;
        stopped.levelClear.selection.alarm1=1;
        const auto draws=random.drawCount();
        const auto time=stopped.levelClearFrame.time;
        unsigned calls=0;
        auto emit=[&](LevelClearSelection::Effect event,unsigned){
            ++calls;if(event==failed)stopped.effectsSucceeded=false;
        };
        require(!stopped.stepLevelClear(confirm,false,false,heart,currency,player,health,random,emit)&&
            !stopped.effectsSucceeded,"Result callback failure is terminal");
        require(stopped.levelClearFrame.time==time&&random.drawCount()==draws&&
            stopped.levelClear.selection.alarm4==60&&confirm,
            "failed alarm does not run Result Step, presentation or later timers");
        const auto count=calls;
        require(!stopped.stepLevelClear(confirm,false,false,heart,currency,player,health,random,emit)&&calls==count,
            "failed Result phase is not retried");
    }
    for(int area:{1,2})for(int level:{1,2,3}){
        RunSession run;run.stage.area=area;run.stage.level=level;
        GameMakerRandom random(42),expected(42);
        const auto radius=GoalTimeField::nextRadius(expected);
        require(run.createStartField(240,192,random),"source start-field Create");
        const bool present=area!=1||level!=1;
        require(run.startFieldPresent==present&&run.startField.radius.equal(radius)&&random.drawCount()==1,
                "start field samples radius before first-level destruction");
        bool hit=false;
        require(run.playerGoalContact(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),1,hit)&&hit==present,
                "start descendant participates in goal-family clock contacts without an exit");
        RunnerImageValue unit{0x3f800000u};
        require(run.parentTimeFieldContact(TimeStopGoalContact::Mask::Player,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(176),unit,unit,hit)&&hit==present,"start descendant participates in parent contacts");
        run.gameplayPresentationStep(random,true);
        require(random.drawCount()==1,"pTimeStop suppresses start-field Draw RNG");
        run.gameplayPresentationStep(random);
        if(present)require(run.startField.radius.equal(GoalTimeField::nextRadius(expected)),"live start field resamples in Draw");
        require(random.drawCount()==expected.drawCount(),"destroyed start field consumes no Draw RNG");
        for(unsigned i=0;i<16;++i)require(random.stateWord(i)==expected.stateWord(i),"start field complete RNG state");
        TimeFieldOutlines snapshot;
        snapshot.set(run.timeFields,run.goalField,false,0,160,34,0,present?&run.startField:nullptr);
        require(snapshot.count==unsigned(present),"start-field presentation shares immutable field outlines");
        run.reset();require(!run.startFieldPresent,"run reset retires the start field");
    }
    for(unsigned failAt:{1u,2u}){
        RunSession run;GameMakerRandom random(42);
        struct Camera {int end=0;void setLevelExit(int y){end=y;}} camera;
        unsigned calls=0;
        auto publish=[&](LevelExit::Effect effect,int x,int y){
            ++calls;
            require(x==240&&y==(calls==1?4000:4032)&&
                effect==(calls==1?LevelExit::Effect::roomEndSetter:LevelExit::Effect::goalTimeField),
                "shared exit child source order");
            return calls!=failAt;
        };
        const auto draws=random.drawCount();
        require(!run.createLevelExit(480,4000,random,camera,publish)&&!run.effectsSucceeded&&!run.levelExitPresent&&
            calls==failAt&&camera.end==(failAt==1?0:4000)&&random.drawCount()==draws,
            "checked exit child failure retains earlier camera write but consumes no radius RNG");
        require(!run.createLevelExit(480,4000,random,camera,publish)&&calls==failAt,
            "failed session cannot retry exit children");
    }
    {
        RunSession run;GameMakerRandom random(84);
        for(unsigned i=0;i<31;++i)require(run.flashEffects.create({}, {},random)!=0,"flash capacity fixture");
        auto oracle=run.flashEffects;auto expectedRandom=random;
        require(!oracle.skeletonBurst({}, {},expectedRandom),"partial flash burst reaches capacity");
        require(!run.emitFlashBurst({}, {},random)&&!run.effectsSucceeded,"flash burst failure latches session");
        require(run.flashEffects.birthSequence().next==33,"successful partial flash birth remains");
        require(random.drawCount()==expectedRandom.drawCount(),"partial flash RNG retained");
        for(unsigned i=0;i<16;++i)require(random.stateWord(i)==expectedRandom.stateWord(i),"partial flash RNG state");
        require(!run.emitFlashBurst({}, {},random)&&random.drawCount()==expectedRandom.drawCount(),"failed flash burst is not retried");
        run.advanceFlashAnimation();
        for(unsigned i=0;i<32;++i)require(run.flashEffects.slot(i).effect.timeline.bits==0x3f800000u,"failed session does not animate flashes");
        run.reset();require(run.effectsSucceeded,"reset clears session failure");
        require(run.emitFlashBurst({}, {},random),"flash creation resumes after reset");
        // Fault injection tests propagation, not an original-game numeric input.
        auto& damaged=const_cast<FlashBallEffect&>(run.flashEffects.slot(0).effect);
        damaged.timeline.bits=0x7f800000u;
        run.advanceFlashAnimation();require(!run.effectsSucceeded,"unsupported flash animation latches failure");
    }
    {
        RunSession run;
        auto create=[&](){return run.createDamageNumber(MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(500),MotionBinary64::fromInteger(1),{});};
        const auto first=create();require(first && run.damageNumbers.size()==1,"shared damage-number birth");
        const auto phase=run.damageNumbers.beginPhase();
        require(run.stepDamageNumbers(phase,[](auto,auto,auto& out){out={};return true;}),"shared number Step");
        for(unsigned tick=0;tick<60;++tick)require(run.advanceDamageNumberAlarm(phase,0),"shared number alarm");
        require(run.damageNumbers.size()==0&&!run.damageNumbers.get(first),"shared number retirement");
        for(unsigned i=0;i<8;++i)require(create()!=0,"shared number capacity fixture");
        require(!create()&&!run.effectsSucceeded,"shared number capacity failure");
        require(!run.stepDamageNumbers(run.damageNumbers.beginPhase(),[](auto,auto,auto&){return false;}),"failed session stops number Step");
        run.reset();require(run.effectsSucceeded&&run.damageNumbers.size()==0,"reset cleans failed number owner");
        require(create()==10,"reset preserves damage-number lifetime keys");
    }
    {
        RunSession fields;
        RunnerImageValue unit,mirror;
        require(unit.assign(MotionBinary64::fromInteger(1)) && mirror.assign(MotionBinary64::fromInteger(-1)),"contact scales");
        bool hit=true;
        require(fields.parentTimeFieldContact(TimeStopGoalContact::Mask::Player,{}, {},unit,unit,hit) && !hit,
            "empty parent family does not invent a field");
        for(unsigned i=0;i<64;++i)
            require(fields.adoptTimeField(240,int(i)*40, i*67108863u),"ordinary field contact fixture");
        require(fields.parentTimeFieldContact(TimeStopGoalContact::Mask::Player,
            MotionBinary64::fromInteger(240),{},unit,unit,hit) && hit,"ordinary field triggers parent contact without exit");
        require(fields.playerGoalContact(MotionBinary64::fromInteger(240),{},1,hit) && !hit,
            "ordinary field does not change the goal-only run-clock query");
        GameMakerRandom random(42);
        struct Camera {void setLevelExit(int){}} camera;
        fields.createLevelExit(480,3000,random,camera);
        const auto draws=random.drawCount();
        for(auto mask:{TimeStopGoalContact::Mask::Player,TimeStopGoalContact::Mask::Shard})
        for(unsigned i=0;i<256;++i){
            auto x=MotionBinary64::fromInteger(208+int(i%65));
            auto y=MotionBinary64::fromInteger(int(i*13));
            require(x.add({0x3fd80000u,0}) && y.add({0x3fe40000u,0}),"fractional family query"); // +3/8, +5/8
            RunnerImageValue px,py;
            require(px.assign(x) && py.assign(y),"query narrowing");
            bool expected=false,part=false;
            for(unsigned j=0;j<fields.timeFields.size();++j){
                require(TimeStopGoalContact::query(mask,px,py,mirror,unit,*fields.timeFields.at(j),part),"ordinary reference query");
                expected|=part;
            }
            require(TimeStopGoalContact::query(mask,px,py,mirror,unit,fields.goalField,part),"goal reference query");
            expected|=part;
            require(fields.parentTimeFieldContact(mask,x,y,mirror,unit,hit) && hit==expected,
                "parent family includes all ordinary fields and goal child at fractional coordinates");
        }
        require(random.drawCount()==draws,"parent contact does not consume RNG");
        hit=true;
        require(!fields.parentTimeFieldContact(TimeStopGoalContact::Mask::Player,{}, {},{},unit,hit) &&
            hit && !fields.effectsSucceeded,"unsupported contact preserves output and latches failure");
        fields.reset(RunRecords{});
        require(fields.parentTimeFieldContact(TimeStopGoalContact::Mask::Player,{}, {},unit,unit,hit) && !hit,
            "room/run reset removes every field contact and failure latch");
    }
    {
        RunSession fields;GameMakerRandom actual(42),expected(42);
        struct Camera {void setLevelExit(int){}} camera;
        require(fields.adoptTimeField(240,800,1) && fields.adoptTimeField(240,900,2),"fields before exit birth");
        fields.createLevelExit(480,2000,actual,camera);
        expected.nextU32(); // Goal Create.
        require(fields.goalFieldBirthBoundary==2,"goal records its exact creation boundary");
        require(fields.createTimeField(240,1000,actual),"runtime field after exit birth");
        require(fields.timeFields.at(2)->radius.equal(OrdinaryTimeField::nextRadius(expected)),
            "runtime field Create samples exactly once before Draw");
        require(fields.goalFieldBirthBoundary==2,"runtime birth preserves exit Draw boundary");
        fields.gameplayPresentationStep(actual);
        require(fields.timeFields.at(2)->radius.equal(OrdinaryTimeField::nextRadius(expected)),"new field draws first");
        require(fields.goalField.radius.equal(GoalTimeField::nextRadius(expected)),"exit draws at birth boundary");
        require(fields.timeFields.at(1)->radius.equal(OrdinaryTimeField::nextRadius(expected)) &&
            fields.timeFields.at(0)->radius.equal(OrdinaryTimeField::nextRadius(expected)),"older fields draw newest first");
        require(actual.drawCount()==expected.drawCount() && actual.stateIndex()==expected.stateIndex(),"live field Draw cursor");
        for(unsigned i=0;i<16;++i)require(actual.stateWord(i)==expected.stateWord(i),"live field full RNG state");
        const auto count=actual.drawCount();
        fields.gameplayPresentationStep(actual,true);
        require(actual.drawCount()==count,"explicit pTimeStop freezes all field Draw sampling");
        fields.goalStopped=true;
        fields.gameplayPresentationStep(actual,false);
        require(actual.drawCount()==count+4,"goal stop is not a substitute for pTimeStop");
    }
    {
        RunSession fields;GameMakerRandom actual(123),expected(123);
        for(unsigned i=0;i<64;++i){
            require(fields.createTimeField(240,int(i),actual),"runtime field capacity fixture");
            require(fields.timeFields.at(i)->radius.equal(OrdinaryTimeField::nextRadius(expected)),
                "runtime birth retains synchronous Create radius");
        }
        require(!fields.createTimeField(240,65,actual) && !fields.effectsSucceeded && fields.timeFields.size()==64,
            "runtime field exhaustion is explicit");
        fields.timeFields.clear();
        require(!fields.createTimeField(240,66,actual) && fields.timeFields.size()==0,
            "free storage does not clear session failure");
        require(actual.drawCount()==expected.drawCount() && actual.stateIndex()==expected.stateIndex(),
            "rejected runtime births consume no random words");
        for(unsigned i=0;i<16;++i)require(actual.stateWord(i)==expected.stateWord(i),"runtime full RNG state");
        fields.reset();
        require(fields.createTimeField(240,700,actual) && fields.timeFields.identityAt(0)==65,
            "reset permits runtime births without reusing stale identities");
    }
    {
        RunSession fields;
        for(unsigned i=0;i<64;++i)require(fields.adoptTimeField(240,int(i),i),"field capacity fixture");
        require(!fields.adoptTimeField(240,65,65) && !fields.effectsSucceeded && fields.timeFields.size()==64,
            "field capacity failure is explicit and preserves existing instances");
        require(!fields.adoptTimeField(240,66,66),"field failure remains latched");
        fields.reset();require(fields.effectsSucceeded && fields.timeFields.size()==0 && !fields.timeFields.at(0),
            "new run retires time fields and clears capacity failure");
    }
    {
        RunSession failed;GameMakerRandom rng(123);
        require(failed.emitBloodBurst(MotionBinary64::fromInteger(240),{},rng),"failure fixture births");
        auto before=failed.bloodEffects;
        unsigned calls=0;
        failed.stepBloodBound(failed.bloodEffects.beginPhase(),true,480,
            [&](const auto& effect,bool&){
                ++calls;
                require(effect.savedSpeed.bits==effect.imageSpeed.bits,
                    "source captures image speed before the failing field query");
                return false;
            });
        require(!failed.effectsSucceeded && calls==1,"unsupported field query latches session failure");
        unsigned visited=0;
        failed.bloodEffects.visitNewest([&](auto handle,const auto& effect){
            const auto* old=before.get(handle);
            require(old && effect.savedSpeed.bits==(visited++==0?old->imageSpeed.bits:old->savedSpeed.bits),
                "query failure retains the first capture and does not visit later blood");
            require(effect.imageSpeed.bits==old->imageSpeed.bits && effect.x.bits==old->x.bits &&
                effect.y.bits==old->y.bits && effect.xx.high==old->xx.high && effect.xx.low==old->xx.low &&
                effect.yy.high==old->yy.high && effect.yy.low==old->yy.low,
                "failed query must not freeze or move an instance");
        });
        const auto draws=rng.drawCount();
        require(!failed.emitBloodBurst({}, {},rng) && rng.drawCount()==draws,"latched failure consumes no further RNG");
        failed.reset();require(failed.effectsSucceeded && failed.bloodEffects.size()==0,"reset clears failed ownership");
    }
    RunSession run;
    require(run.adoptTimeField(240,900,42),"old room owns an ordinary time field");
    require(run.addWallBackground(MotionBinary64::fromInteger(232),MotionBinary64::fromInteger(892)),"old room retains wall artwork");
    const auto oldNumber=run.createDamageNumber(MotionBinary64::fromInteger(240),
        MotionBinary64::fromInteger(900),MotionBinary64::fromInteger(1),{});
    require(oldNumber!=0,"old room owns a damage number");
    PlayerMotion player(false, 10000);
    GameMakerRandom random(17);
    CurrencyAccount currency;
    currency.currency = 123;
    run.stage.area = 1; run.stage.level = 3;
    run.kills = 37;
    run.upgrades.levels[0] = 1;
    run.gems.lifetime = 999;
    int health = 4;
    bool heart = false;
    require(!run.enterLevelClear(0, heart, health, player, random), "entry requires exit request");
    run.levelResultRequested = true;
    run.levelExitPresent = run.goalStopped = run.exitControlsLocked = true;
    run.exitDither.create(160, 1000);
    require(run.flashEffects.create(downwell::Fixed16::fromInteger(240),downwell::Fixed16::fromInteger(176),random)!=0,
            "old room owns a flash effect");
    require(run.smokeEffects.create(downwell::Fixed16::fromInteger(240),downwell::Fixed16::fromInteger(176),random)!=0,
            "old room owns a smoke effect");
    const auto draws = random.drawCount();
    require(run.emitBloodBurst(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),random),
            "old room owns blood effects");
    const auto bloodDraws=random.drawCount()-draws;
    require(bloodDraws==2,"blood script consumes exactly two angle draws");
    const auto bloodPhase=run.bloodEffects.beginPhase();
    run.advanceBloodAnimation();
    unsigned allowedQueries=0;
    run.stepBlood(bloodPhase,[&](auto,const auto&){++allowedQueries;return false;});
    require(allowedQueries==2 && run.effectsSucceeded,"session dispatches per-instance time-stop query");
    unsigned extraFields=0;
    run.stepBloodBound(bloodPhase,false,480,[&](const auto&,bool&){++extraFields;return false;});
    require(run.effectsSucceeded && extraFields==0,"inactive pTimeStop skips field queries");
    run.goalField.x=240;run.goalField.y=176;
    run.stepBloodBound(bloodPhase,true,480,[&](const auto&,bool&){++extraFields;return false;});
    require(run.effectsSucceeded && extraFields==0,"owned goal field allows effects during global stop");
    run.levelExitPresent=false;
    run.stepBloodBound(bloodPhase,true,480,[&](const auto&,bool& hit){++extraFields;hit=false;return true;});
    require(run.effectsSucceeded && extraFields==2,"outside all fields freezes both blood instances");
    run.levelExitPresent=true;
    run.bloodEffects.visitNewest([&](auto,const BloodEffect& effect){
        require(effect.imageIndex.bits==0x3f000000u && effect.imageSpeed.bits==0,
                "earlier animation remains visible when Step freezes movement");
    });
    require(!run.enterLevelClear(9, heart, health, player, random) && run.levelResultRequested &&
        random.drawCount() == draws+bloodDraws, "invalid entry leaves request and RNG intact");
    require(run.enterLevelClear(0, heart, health, player, random) && run.levelClearActive &&
        !run.levelExitPresent && !run.exitDither.active && !run.levelResultRequested,
        "accepted room boundary retires exit state");
    unsigned flashCount=0;
    run.flashEffects.visitNewest([&](const FlashBallEffect&){++flashCount;});
    require(flashCount==0,"room retirement clears flash effects without resetting the whole run");
    require(run.smokeEffects.size()==0,"room retirement clears smoke ownership");
    require(run.bloodEffects.size()==0,"room retirement clears blood ownership");
    require(run.timeFields.size()==0,"room retirement clears ordinary time fields");
    require(run.wallBackground.size()==0,"room retirement clears retained wall tiles");
    require(run.damageNumbers.size()==0&&!run.damageNumbers.get(oldNumber),"room retirement clears damage numbers");
    require(!run.enterLevelClear(0, heart, health, player, random) && random.drawCount() == draws + bloodDraws + 1,
        "duplicate room entry cannot consume another Create random word");
    unsigned tick = 0, advances = 0, dithers = 0;
    auto emit = [&](LevelClearSelection::Effect event, unsigned) {
        if (event == LevelClearSelection::Effect::dither) {
            require(tick == 121, "source departure dither alarm timing"); ++dithers;
        }
        if (event == LevelClearSelection::Effect::nextLevel) {
            require(tick == 210, "source next-room alarm timing includes the arming phase"); ++advances;
        }
    };
    for (tick = 1; tick <= 210; ++tick) {
        require(!run.advanceAfterLevelClear(), "loader cannot advance before departure completes");
        bool confirm = true;
        require(run.stepLevelClear(confirm, false, false, heart, currency, player, health, random, emit),
            "active result room advances shared controller");
        require(run.stage.area == 1 && run.stage.level == 3, "request does not construct next stage prematurely");
        if (tick == 60) require(run.levelClearFrame.circle == 3 && run.levelClearFrame.time == 1 &&
            run.levelClearFrame.choices, "choice reveal resets shadow phase before presentation");
        if (tick == 90) require(!run.levelClearFrame.choices && run.levelClearFrame.time == 30,
            "confirmation hides choices before the Draw clock can advance");
        if (tick == 121) require(run.exitDither.active && run.exitDither.x == 80,
            "result departure anchors its new wipe in the narrow room");
        if (tick == 136) require(run.exitDither.soundRequested, "departure wipe advances its own alarm");
    }
    require(advances == 1 && dithers == 1 && run.nextLevelRequested, "one transition request");
    bool confirm = true;
    require(!run.stepLevelClear(confirm, false, false, heart, currency, player, health, random, emit),
        "pending loader does not simulate additional old-room frames");
    require(run.advanceAfterLevelClear() && run.stage.area == 2 && run.stage.level == 1 &&
        !run.goalStopped && !run.exitControlsLocked && !run.exitDither.active,
        "consuming transition advances stage and releases gameplay locks");
    require(run.kills == 37, "kill total survives upgrade selection and area transition");
    require(!run.advanceAfterLevelClear(), "stage advancement is one-shot");
    require(run.upgrades.levels[0] == 1 && run.upgrades.count == 1 && run.gems.lifetime == 999 &&
        currency.currency >= 123, "level transition preserves run ownership and records");
    player.increaseAmmoCapacity(4);
    player.face(-1);
    Input shooting; shooting.fire = true; shooting.left = true;
    for (unsigned tick = 0; tick < 40; ++tick) player.step(shooting);
    const int capacity = player.ammoCapacity();
    require(player.shots() && player.ammo() < capacity, "room-entry fixture spent charge and moved");
    player.enterRoom(true);
    require(player.ammoCapacity() == capacity && player.ammo() == capacity && player.facing() == -1 &&
        player.offsetXFixed() == 0 && player.offsetYFixed() == 0 && player.speedXFixed() == 0 &&
        player.speedYFixed() == 0 && !player.grounded() && !player.shots() &&
        player.jumpShootLocked() && player.shotDelayed(),
        "room entry resets instance motion but preserves globals and refills charge");
    run.reset();
    require(run.kills == 0, "new-run reset clears kills, not a level transition");
    require(!run.levelClearActive && !run.nextLevelRequested && run.levelClear.selection.alarm4 == 60 &&
        run.upgrades.count == 0, "new-run reset clears result lifecycle");
    std::puts("Shared exit/result/next-stage lifecycle and persistent ownership passed");
}
