#ifndef DOWNWELL_CORE_GENERATED_BREAKABLE_CREATE_H
#define DOWNWELL_CORE_GENERATED_BREAKABLE_CREATE_H
#include "GameMakerRandom.h"
namespace downwell {
// Synchronous objBox_n Create output. No deferred command, alarm bank or
// default instance state: native pool slots already own those defaults.
// Keep the original command executor as the differential oracle.
struct GeneratedBreakableCreate {
    unsigned frame,sound,gems;
    static GeneratedBreakableCreate evaluate(GameMakerRandom& random){
#ifdef DOWNWELL_REFERENCE_BOX_RANDOM
        return evaluateReference(random);
#else
        std::uint32_t words[4];
        random.nextWords(words,4);
        GeneratedBreakableCreate result{16+(words[1]&1u),171+(words[0]&3u),0};
        // Same positive-63-bit modulus as irandomSmall(50). Only stream
        // consumption is grouped; neither arithmetic nor draw order changes.
        auto remainder=SmallUnsignedRemainder::full(words[3]&0x7fffffffU,51);
        remainder=SmallUnsignedRemainder::append(remainder,std::uint16_t(words[2]>>16),51);
        remainder=SmallUnsignedRemainder::append(remainder,std::uint16_t(words[2]),51);
        if(!remainder){
            random.chooseIndexSmall(2);
            result.gems=2;result.frame=22;
        }
        return result;
#endif
    }
    static GeneratedBreakableCreate evaluateReference(GameMakerRandom& random){
        GeneratedBreakableCreate result;
        result.sound=171+random.chooseIndexSmall(4);
        result.frame=16+random.chooseIndexSmall(2);
        result.gems=0;
        if(random.irandomSmall(50)<1){
            random.chooseIndexSmall(2);
            result.gems=2;result.frame=22;
        }
        return result;
    }
};
}
#endif
