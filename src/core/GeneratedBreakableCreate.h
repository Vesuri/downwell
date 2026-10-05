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
