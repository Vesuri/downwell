#ifndef DOWNWELL_TEST_OWNED_TERRAIN_FIXTURE_H
#define DOWNWELL_TEST_OWNED_TERRAIN_FIXTURE_H
#include "core/GameplayTerrainInstances.h"
#include <cassert>
namespace downwell {
// Test oracle explicitly owns reference backing. Shipping native scenes do not.
class OwnedTerrainFixture : public GameplayTerrainInstances {
    Storage storage_;
public:
    explicit OwnedTerrainFixture(Binding binding={},Presentation presentation={},WallCollision collision={}):
        GameplayTerrainInstances(binding,presentation,collision){
        const bool bound=bindStorage(storage_);assert(bound);(void)bound;
    }
};
}
#endif
