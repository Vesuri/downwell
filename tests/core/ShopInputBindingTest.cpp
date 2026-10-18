#include "core/GameplayWorld.h"
#include "core/Replay.h"
#include <cassert>
#include <cstdio>
#include <memory>

int main(){
    auto world=std::make_unique<GameplayWorld>(300,true);
    downwell::Input held;held.fire=true;held.right=true;
    world->step(downwell::packReplayInput(held));
    assert(world->effectsSucceeded());
    assert(world->withShopStepState([&](auto& input,auto& currency){
        assert(&input==&world->shops().input&&input.confirm&&input.right);
        input.confirm=false;currency.currency=17;
        return world->withShopStepState([&](auto& nested,auto& account){
            return &nested==&input&&&account==&currency&&!nested.confirm&&account.currency==17;
        });
    }));
    assert(world->currency()==17&&!world->shops().input.confirm);
    world->step(downwell::packReplayInput(held));
    assert(world->effectsSucceeded()&&!world->shops().input.confirm&&!world->shops().input.right);
    world->step(0);world->step(downwell::packReplayInput(held));
    assert(world->effectsSucceeded()&&world->shops().input.confirm&&world->shops().input.right);
    unsigned calls=0;
    assert(!world->withShopStepState([&](auto&,auto&){++calls;return false;}));
    assert(!world->effectsSucceeded());
    assert(!world->withShopStepState([&](auto&,auto&){++calls;return true;})&&calls==1);
    std::puts("Native shared Shop input, currency, nested consumption and terminal failure passed");
}
