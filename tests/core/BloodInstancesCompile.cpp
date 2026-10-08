#include "core/BloodInstances.h"
// Freestanding cross-compiler instantiation check; runtime behavior is covered
// by the real resource-owner fixtures, not these deliberately inert callbacks.
bool instantiateBloodOwnership(downwell::BloodInstances<2>& owner){
    downwell::InstanceHandle key;
    return owner.create(105187,{}, {},{},[](auto){return true;},key)&&
        owner.step(key,[](const auto&,bool& allowed){allowed=true;return true;})&&
        owner.advanceAnimation(key)&&owner.markDestroyed(key)&&owner.retire(key);
}
