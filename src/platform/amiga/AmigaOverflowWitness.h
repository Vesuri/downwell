#ifndef DOWNWELL_AMIGA_OVERFLOW_WITNESS_H
#define DOWNWELL_AMIGA_OVERFLOW_WITNESS_H
#include "core/GameplayWorld.h"
#include "core/VerificationDigest.h"

// Verification only. Explicit endian/word-size independent observations, not
// a hash of struct padding and not a claim to cover all runner-owned state.
// Five groups localize a first mismatch: player/camera, RNG, enemies, currency,
// and projectiles/damage numbers/fields. Local lifetime keys are NOT source IDs.
inline void amigaOverflowWitness(const GameplayWorld& w,std::uint64_t (&out)[5]) {
    using downwell::VerificationDigest;
    VerificationDigest player;
    player.addU32(1); // Schema; change when the observation set changes.
    player.addU32(w.tick());w.player().appendStateDigest(player);
    player.addU64(w.sideCameraDigest());player.addI32(w.health());
    player.addU32(w.currency());player.addBool(w.dead());
    player.addBool(w.effectsSucceeded());player.addBool(w.generationSucceeded());
    out[0]=player.value();
    VerificationDigest random;
    random.addU64(w.randomDrawCount());random.addU32(w.randomState().seed());
    random.addU32(w.randomState().stateIndex());
    for(unsigned i=0;i<16;++i)random.addU32(w.randomState().stateWord(i));
    out[1]=random.value();
    VerificationDigest enemies;
    enemies.addU32(w.enemyCount());
    for(unsigned i=0;i<w.enemyCount();++i){const auto& e=w.enemyAt(i);
        enemies.addU32(e.lifetimeId);enemies.addI32(e.objectId);
        enemies.addI32(e.spriteId);enemies.addI32(e.maskId);
        enemies.addI32(e.x);enemies.addI32(e.y);enemies.addI32(e.health);
        enemies.addI32(e.preciseX);enemies.addI32(e.preciseY);
        enemies.addI32(e.speedX);enemies.addI32(e.speedY);
        enemies.addI32(e.alarm0);enemies.addI32(e.alarm1);
        enemies.addU32(e.imagePhase);enemies.addU32(e.imageFrame);
        enemies.addBool(e.active);enemies.addBool(e.destroyRequested);
    }
    out[2]=enemies.value();
    VerificationDigest gems;
    gems.addU32(w.gemSlotEnd());
    for(unsigned i=0;i<w.gemSlotEnd();++i){const auto& g=w.gemAt(i);
        gems.addBool(g.active);if(!g.active)continue;
        gems.addU32(i);gems.addU32(g.lifetimeId);gems.addI32(g.x);gems.addI32(g.y);
        gems.addI32(static_cast<long>(g.motion.x));gems.addI32(static_cast<long>(g.motion.y));
        gems.addI32(g.motion.vx);gems.addI32(g.motion.vy);
        gems.addU32(g.motion.lifetime.age);gems.addU32(g.motion.animation.phaseBits());
        gems.addU32(g.motion.animation.speedBits());gems.addBool(g.motion.attracted);
        gems.addBool(g.large);gems.addI32(g.amount);
    }
    out[3]=gems.value();
    VerificationDigest effects;
    // Owned native bullets remain marked until Draw cleanup; the unowned
    // host fixture retires immediately. Compare the ordered live sequence,
    // not physical holes or the tail before that separate ownership phase.
    effects.addU32(w.bulletCount());
    for(unsigned i=0;i<w.bulletSlotEnd();++i){const auto& b=w.bulletAt(i);
        if(!b.active)continue;
        effects.addU32(b.lifetimeId);
        effects.addI32(b.x);effects.addI32(b.y);effects.addBool(b.destroyRequested);
    }
    effects.addU32(w.damageNumbers().size());
    const bool valid=w.damageNumbers().visitNewest([&](unsigned key,const auto& n){
        effects.addU32(key);effects.addU32(n.drawBirth);
        effects.addU32(n.x.bits);effects.addU32(n.y.bits);effects.addI32(n.alarm0);
        const downwell::MotionBinary64 values[]={n.amount,n.direction,n.xSpeed,n.ySpeed};
        for(const auto value:values){
            effects.addU32(value.high);effects.addU32(value.low);
        }
        return true;
    });
    effects.addBool(valid);effects.addU32(w.timeFields().size());
    for(unsigned i=0;i<w.timeFields().size();++i){const auto* f=w.timeFields().at(i);
        effects.addBool(f!=nullptr);if(!f)continue;
        effects.addI32(f->x);effects.addI32(f->y);
        effects.addU64(f->radius.mantissa);effects.addI32(f->radius.exponent);
    }
    out[4]=effects.value();
}
#endif
