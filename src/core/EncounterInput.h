#ifndef DOWNWELL_CORE_ENCOUNTER_INPUT_H
#define DOWNWELL_CORE_ENCOUNTER_INPUT_H
#include "Input.h"
#include "ShopInteraction.h"
namespace downwell {
// Device levels survive scene/run resets. The current mutable publication is
// borrowed by consumers; consumption never changes the device's raw history.
class EncounterInput {
public:
    void sample(const Input& held){
        fireReleased_=previous_.fire&&!held.fire;
        edges_={held.right&&!previous_.right,held.left&&!previous_.left,
                held.fire&&!previous_.fire,held.cancel&&!previous_.cancel};
        previous_=held;
    }
    void resetPublication(){edges_={};} // scrInitialize resets globals, not device levels.
    ShopInteraction::Input& publication(){return edges_;}
    const ShopInteraction::Input& publication()const{return edges_;}
    bool fireReleased()const{return fireReleased_;}
private:
    Input previous_{};
    ShopInteraction::Input edges_{};
    bool fireReleased_=false;
};
}
#endif
