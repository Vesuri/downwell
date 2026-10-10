#ifndef DOWNWELL_CORE_SHOP_KEEPER_ANIMATION_H
#define DOWNWELL_CORE_SHOP_KEEPER_ANIMATION_H
#include "UnitSequenceFrames.h"
namespace downwell {
struct ShopKeeperAnimation {
    static unsigned frames(int sprite){
        switch(sprite){case 485:case 496:return 4;case 486:return 5;
            case 487:case 497:return 2;default:return 0;}
    }
    template<class Publish> static bool advance(RunnerImageValue& timeline,int sprite,
        RunnerImageValue written,RunnerImageValue speed,Publish publish){
        const unsigned count=frames(sprite);if(!count)return false;
        RunnerImageValue image,candidate=timeline;bool reconcile=true,crossed;
        if(UnitSequenceFrames::evaluate(candidate,count,image)&&
           !UnitSequenceFrames::needsReconcile(written,image,reconcile))return false;
        if(reconcile){
            // Source Step writes zero; Alarm 0 writes one on sprite 487.
            // Inverse mapping divides by the frame count, separates cycles,
            // then multiplies by key count. Zero and 1/2 are exact throughout.
            if(written.bits!=0 && !(sprite==487&&written.bits==0x3f800000u))return false;
            candidate=written;
        }
        if(!UnitSequenceFrames::advance(candidate,speed,count,image,crossed)||!publish(image))return false;
        timeline=candidate;return true; // No Animation End event on this object.
    }
};
}
#endif
