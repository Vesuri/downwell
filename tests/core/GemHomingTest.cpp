#include "core/GemHoming.h"
#include "core/GemMotion.h"
#include "core/CurrencyStep.h"
#include "core/AlarmBank.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
void require(bool value, const char* text) {
    if (!value) { std::cerr << text << '\n'; std::exit(1); }
}
int main() {
    // An independent event-driven oracle, not another sum of age thresholds.
    for(bool upgraded:{false,true})for(bool attracted:{false,true}){
        downwell::AlarmBank alarms;alarms.set(0,10);alarms.set(3,upgraded?30:15);
        downwell::GemLifetime life;
        bool obtainable=false,disappearing=false,suckable=false,alive=true;int flash=-1;
        for(unsigned tick=1;tick<=400&&alive;++tick){
            for(unsigned index=0;index<4;++index)if(alarms.advanceOne(index)){
                switch(index){
                case 0: obtainable=true;alarms.set(1,180);break;
                case 1: disappearing=true;alarms.set(2,120);break;
                case 2: if(!attracted)alive=false;break;
                case 3: suckable=true;break;
                }
            }
            require(life.advance(attracted)==alive,"live default gem lifetime matches immediate per-index dispatch");
            if(!alive){require(tick==308,"source chained alarms expire on update 308");break;}
            if(disappearing)flash=-flash;
            require(life.obtainable()==obtainable&&life.disappearing()==disappearing&&
                life.suckable(upgraded)==suckable&&life.visible()==(flash<0),
                "collection, attraction and Draw flash match the event-driven oracle");
            if(tick==10)require(alarms.get(1)==179,"Alarm 1 starts decrementing in Alarm 0's phase");
            if(tick==189)require(alarms.get(2)==119&&!life.visible(),"Alarm 2 starts during the first hidden flash update");
        }
    }
    for(int size:{0,1,2})for(unsigned choice=0;choice<unsigned(size?2:3);++choice){
        struct Choice {
            unsigned selected,count,calls=0;
            unsigned chooseIndexSmall(unsigned n){require(n==count,"source bounce-sound candidate count");++calls;return selected;}
        } random{choice,unsigned(size?2:3)};
        downwell::CurrencyStepSound state;
        require(!state.allSet&&state.bounceSound==14,"Create defers bounce-sound selection to Step");
        state.begin(size,random);
        require(state.allSet&&state.bounceSound==int(choice)+(size?163:168)&&state.mySound==state.bounceSound&&random.calls==1,
            "first Step publishes source bounceSnd then mySound and allSet");
        state.begin(size?0:1,random);
        require(random.calls==1,"later size changes do not repeat first-Step RNG");
    }
    {
        struct Random {
            unsigned randomIntegerSmall(unsigned) { return 0; }
            unsigned chooseIndexSmall(unsigned) { return 0; }
        } random;
        for (bool upgraded : {false, true}) {
            downwell::CurrencyAccount account;
            downwell::GemMotion gem;
            gem.lifetime.age = 9;
            const auto result = downwell::stepCurrency(gem, false, 2, 1000 * 256L, 0,
                0, 0, account, [](int, int) { return false; }, random, upgraded);
            const unsigned duration = upgraded ? 480 : 300;
            require(result == downwell::CurrencyStepResult::collected && account.timer == duration,
                    "live currency phase forwards the owned upgrade's extra 180 updates");
            for (unsigned tick = 1; tick < duration; ++tick) account.beginStep();
            require(account.streak == 2 && account.timer == 1, "streak survives its last protected update");
            account.beginStep();
            require(account.streak == 0 && account.timer == 0 && account.currency == 2,
                    "streak expires without spending the collected gems");
        }
    }
    {
        struct Random {
            unsigned randomIntegerSmall(unsigned) { return 0; }
            unsigned irandomSmall(unsigned) { return 0; }
            unsigned chooseIndexSmall(unsigned) { return 0; }
        } random;
        const auto empty = [](int, int) { return false; };
        for (bool upgraded : {false, true}) {
            const int radius = upgraded ? 48 : 24;
            const unsigned delay = upgraded ? 30 : 15;
            for (int distance : {radius - 1, radius, radius + 1}) {
                downwell::GemMotion gem;
                gem.initialize(0, 0, false, random, upgraded);
                gem.vx = gem.vy = 0;
                gem.lifetime.age = delay - 2;
                gem.step(distance * 256L, 0, empty, random);
                require(!gem.attracted, "Attractor cannot enable homing before its birth-time alarm");
                gem.x = gem.y = gem.vx = gem.vy = 0;
                gem.step(distance * 256L, 0, empty, random);
                require(gem.attracted == (distance < radius),
                        "default and upgraded acquisition use strict source radius and alarm edges");
                if (gem.attracted) require(gem.homing.speedEighths == 32,
                    "source acquisition speed is four even though upgraded Create sets unused atrsp=6");
            }
        }
    }
    {
        using namespace downwell;
        struct RandomProbe {
            unsigned calls = 0;
            unsigned randomIntegerSmall(unsigned) { ++calls; return 0; }
            unsigned chooseIndexSmall(unsigned) { ++calls; return 0; }
        } random;
        CurrencyAccount account;
        GemMotion gem;
        gem.lifetime.age = 8;
        unsigned terrainQueries = 0;
        const auto solid = [&](int, int) { ++terrainQueries; return false; };
        auto result = stepCurrency(gem, false, 2, 1000 * 256L, 0, 0, 0, account, solid, random);
        require(result == CurrencyStepResult::alive && gem.lifetime.age == 9 && account.currency == 0,
                "shared currency phase cannot collect before the obtainable alarm");
        require(gem.sound.allSet&&gem.sound.bounceSound==168&&gem.sound.mySound==168&&random.calls==1,
                "live small gem consumes and retains its first-Step choice");
        result = stepCurrency(gem, false, 2, 1000 * 256L, 0, 0, 0, account, solid, random);
        require(result == CurrencyStepResult::collected && account.currency == 2 &&
                    account.gameGem == 2 && account.streak == 2 && account.timer == 300,
                "collection uses the current player position, not the distant homing target");
        GemMotion expired;
        expired.lifetime.age = 307;
        const auto oldQueries = terrainQueries, oldRandom = random.calls;
        result = stepCurrency(expired, true, 10, 0, 0, 0, 0, account, solid, random);
        require(result == CurrencyStepResult::expired && account.currency == 2 &&
                    terrainQueries == oldQueries && random.calls == oldRandom,
                "expiry prevents both collision collection and terrain/RNG side effects");
        GemMotion large;
        large.lifetime.age = 9;
        result = stepCurrency(large, true, 10, 1000 * 256L, 0, 9 * 256L, 0, account, solid, random);
        require(result == CurrencyStepResult::collected && account.currency == 12,
                "shared phase preserves the large gem's wider collection mask");
        require(large.sound.bounceSound==163&&large.sound.mySound==163,
                "live large reward selects its own bounce sound after the size override");
        GemMotion small;
        small.lifetime.age = 9;
        result = stepCurrency(small, false, 2, 1000 * 256L, 0, 9 * 256L, 0, account, solid, random);
        require(result == CurrencyStepResult::alive && account.currency == 12,
                "small gem cannot collect through the large-only fringe");
        GemMotion flashing;
        CurrencyAccount flashingAccount;
        flashing.lifetime.age = 188;
        result = stepCurrency(flashing, false, 2, 1000 * 256L, 0, 0, 0, flashingAccount, solid, random);
        require(result == CurrencyStepResult::collected && !flashing.lifetime.visible() &&
                    flashingAccount.currency == 2,
                "hidden flash phases remain collectible and credit the same currency");
        GemMotion homing;
        homing.attracted = true;
        homing.lifetime.age = 307;
        const auto before = homing;
        GemMotion expected = before;
        expected.step(30 * 256L, 0, solid, random);
        result = stepCurrency(homing, false, 2, 30 * 256L, 0, -1000 * 256L, 0, account, solid, random);
        require(result == CurrencyStepResult::alive && homing.x == expected.x && homing.y == expected.y &&
                    homing.vx == expected.vx && homing.vy == expected.vy && account.currency == 12,
                "attracted gems survive expiry and home using the captured pre-movement position");
    }
    downwell::GemMotion contact;
    contact.lifetime.age = 10;
    require(contact.collectible(7 * 256, 0) && !contact.collectible(8 * 256, 0) &&
            contact.collectible(-7 * 256, 0) && !contact.collectible(-8 * 256, 0),
            "small gem and player rectangular masks overlap but edge-touch does not collect");
    require(contact.collectible(0, -11 * 256) && !contact.collectible(0, -12 * 256) &&
            contact.collectible(0, 7 * 256) && !contact.collectible(0, 8 * 256),
            "collection preserves the source player's asymmetric vertical mask");
    require(contact.collectible(9 * 256, 0, true) && !contact.collectible(10 * 256, 0, true) &&
            contact.collectible(0, -13 * 256, true) && !contact.collectible(0, -14 * 256, true) &&
            contact.collectible(0, 9 * 256, true) && !contact.collectible(0, 10 * 256, true),
            "large gem uses its own twelve-pixel source mask");
    contact.lifetime.age = 9;
    require(!contact.collectible(0, 0, true), "collision cannot collect before obtainable alarm");
    contact.lifetime.age = 10;
    contact.x = 6401;
    require(!contact.collectible(-7 * 256, 0), "rounding above a half does not lose precision via Q8");
    require(downwell::GemMotion::rounded(6400) == 0 && downwell::GemMotion::rounded(19200) == 2 &&
            downwell::GemMotion::rounded(-6400) == 0 && downwell::GemMotion::rounded(-19200) == -2 &&
            downwell::GemMotion::rounded(-6401) == -1, "authoritative gem rounding uses signed ties-to-even");
    for (unsigned choice = 0; choice < 4; ++choice) {
        struct Choice {
            unsigned value, calls = 0;
            unsigned chooseIndexSmall(unsigned count) {
                require(count == 4, "animation uses the recovered four-way choice");
                ++calls; return value;
            }
        } choiceRandom{choice};
        downwell::GemAnimation animation;
        animation.initialize(choiceRandom);
        const float rates[] = {-.5f, -.3f, .3f, .5f};
        unsigned speedBits; std::memcpy(&speedBits,&rates[choice],4);
        require(choiceRandom.calls == 1 && animation.speedBits() == speedBits && animation.frame() == 0,
                "each source animation choice consumes one draw and starts frame zero");
        float phase = 0;
        for (int tick = 1; tick <= 400; ++tick) {
            animation.advance();
            phase = float(phase + rates[choice]);
            if(phase<0 || phase>=4)phase=std::fmod(float(std::fmod(phase,4.f)+4.f),4.f);
            unsigned phaseBits; std::memcpy(&phaseBits,&phase,4);
            require(animation.phaseBits() == (phaseBits & 0x7fffffffu) &&
                    animation.frame() == static_cast<unsigned>(phase),
                    "signed animation retains runner float addition and wrap rounding");
        }
    }
    downwell::GemLifetime loose, attracted;
    int dflash = -1;
    require(attracted.visible(), "new currency draws with source dflash=-1");
    for (unsigned age = 1; age <= 400; ++age) {
        require(attracted.advance(true), "attracted gem survives its source expiry alarm");
        if (age >= 189) dflash *= -1;
        require(attracted.visible() == (dflash < 0),
                "gem visibility follows source Alarm 1 then Step negation, including attracted expiry");
        require(attracted.obtainable() == (age >= 10) &&
                attracted.suckable() == (age >= 15) &&
                attracted.disappearing() == (age >= 189),
                "default currency alarm boundaries match Create and Alarm 0/1/3");
        if (age <= 308)
            require(loose.advance(false) == (age < 308), "loose gem expires exactly at Alarm 2");
    }
    require(attracted.advance(false), "expired Alarm 2 is not repeated if attraction later clears");
    attracted.age = 65535;
    require(attracted.advance(true) && attracted.age == 65536 && attracted.obtainable(),
            "long-lived attracted gems do not wrap the former SDL 16-bit age counter");
    struct Random {
        unsigned call = 0;
        unsigned irandomSmall(unsigned) {
            const unsigned values[] = {0, 10, 60, 1};
            return values[call++ % 4];
        }
        unsigned chooseIndexSmall(unsigned count) {
            const unsigned values[] = {0, 10, 60, 1};
            return values[call++ % 4]%count;
        }
        unsigned randomIntegerSmall(unsigned) { return 25600; }
    } random;
    downwell::GemMotion motion;
    motion.initialize(10, 20, true, random);
    require(motion.homing.direction == 180L * 65536 && random.call == 4 &&
            motion.vx == 0 && motion.vy == 0 && motion.attracted,
            "VM evaluates second heading candidate first; stomp preserves Create homing defaults");
    motion = {};
    const auto empty = [](int, int) { return false; };
    for (unsigned tick = 1; tick <= 20; ++tick) {
        motion.step(10000 * 256L, 0, empty, random);
        require(motion.animation.phaseBits()==0,
                "ordinary currency Step does not perform engine animation");
        require(motion.vy == 448L * tick && motion.y == 224L * tick * (tick + 1) && motion.vx == 0,
                "free gravity is exact .035 and sign(0) does not invent horizontal motion");
    }
    motion = {};
    motion.lifetime.age = 14;
    motion.step(10 * 256L, 0, empty, random);
    require(motion.attracted && motion.homing.speedEighths == 32 && motion.vy == 448,
            "acquisition update still performs free motion; homing begins next update");
    motion = {};
    motion.lifetime.age = 307;
    motion.animation.advance();
    const auto expiryImage = motion.animation.phaseBits();
    require(!motion.step(10 * 256L, 0, empty, random) && !motion.attracted && motion.y == 0,
            "expiry alarm runs before motion and cannot acquire a nearby player on its last tick");
    require(expiryImage==0x3f000000u && motion.animation.phaseBits()==expiryImage,
            "expiry keeps the image already advanced by the earlier engine phase");
    motion = {};
    motion.attracted = true;
    motion.lifetime.age = 307;
    require(motion.step(10 * 256L, 0, empty, random) && motion.x != 0,
            "already attracted gem survives expiry and still moves that update");
    downwell::GemHoming gem;
    long vx, vy;
    gem.step(0, -25600, vx, vy);
    require(gem.direction == 5L * 65536 && gem.turnFifths == 28 &&
            gem.speedEighths == 9 && vx > 280 && vy < 0,
            "stomp gem begins at speed 1, turns only five degrees, then accelerates");
    gem.acquire(25600, 0);
    require(gem.direction == 0 && gem.speedEighths == 32 && gem.turnFifths == 160,
            "proximity acquisition aims at the player and starts speed four");
    gem.step(25600, 0, vx, vy);
    require(vx == 1056 && vy == 0, "first attracted update accelerates to 4.125");
    for (int i = 0; i < 100; ++i) gem.step(25600, 0, vx, vy);
    require(vx == 2304 && gem.speedEighths == 72 && gem.turnFifths == 160,
            "source speed and turn caps are independent of gem age");
    gem.direction = 359L * 65536;
    gem.turnFifths = 25;
    gem.step(25600, -447, vx, vy);
    require(gem.direction < 2L * 65536, "turn takes short arc across zero degrees");
    double maximumError = 0;
    for (int initial = 0; initial < 360; initial += 13) {
        gem = {};
        gem.direction = initial * 65536L;
        double angle = initial, turn = 5, speed = 1;
        for (int tick = 0; tick < 80; ++tick) {
            const long dx = 25600 - tick * 37, dy = -8700 + tick * 91;
            double target = std::atan2(-double(dy), double(dx)) * 180 / std::acos(-1.0);
            if (target < 0) target += 360;
            double delta = angle - target;
            if (delta > 180) delta -= 360;
            else if (delta < -180) delta += 360;
            if (delta > turn) angle -= turn;
            else if (delta < -turn) angle += turn;
            else angle = target;
            angle = std::fmod(angle + 360, 360);
            turn = std::fmin(32, turn + .6);
            speed = std::fmin(9, speed + .125);
            gem.step(dx, dy, vx, vy);
            const double radians = angle * std::acos(-1.0) / 180;
            const double error = std::fmax(std::fabs(vx - std::cos(radians) * speed * 256),
                                          std::fabs(vy + std::sin(radians) * speed * 256));
            maximumError = std::fmax(maximumError, error);
        }
    }
    require(maximumError < 1.5, "fixed-point homing stays within 1.5 Q8 velocity units of source math");
    std::cout << "gem homing tests passed; max Q8 velocity error " << maximumError << '\n';
}
