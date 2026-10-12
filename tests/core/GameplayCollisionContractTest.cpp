#include "core/GameplayCollisionOwner.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

static std::vector<unsigned char> read(const char* path) {
    std::ifstream input(path,std::ios::binary);
    assert(input);
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}

int main() {
    using namespace downwell;
    using Owner=GameplayCollisionOwner;
    const auto ob=read("generated/downwell-objects.dwob");
    const auto sb=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());
    SpriteCollisionPack sprites(sb.data(),sb.size());
    assert(objects.valid()&&sprites.valid());
#ifdef DOWNWELL_EXTERNAL_COLLISION_BACKEND
    constexpr bool restricted=true;
#else
    constexpr bool restricted=false;
#endif
    const auto create=[&](Owner::Ownership kind,bool boxes,
                          Owner::ReferenceStorage* reference=nullptr,
                          Owner::NativeCollisionStorage* collision=nullptr){
        return std::make_unique<Owner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},
            nullptr,kind,boxes,true,reference,collision);
    };
    auto native=create(Owner::Ownership::nativeScenery,true);
    assert(native->valid()&&native->declareExternalCollisionBackend());
    assert(!native->hasNativeCollisionStorage()&&!native->hasPreparedCollisionGeometry());
    RunnerCollisionPairs<Owner::View::capacity,8> pairs;
    assert(!native->collectCollisionPairs(pairs)&&!native->valid());

    auto reference=std::make_unique<Owner::ReferenceStorage>();
    auto scalar=create(Owner::Ownership::reference,true,reference.get());
    assert(scalar->valid()!=restricted);
    auto collision=std::make_unique<Owner::NativeCollisionStorage>();
    auto full=create(Owner::Ownership::nativeScenery,true,nullptr,collision.get());
    assert(full->valid()!=restricted);
    auto withoutBoxes=create(Owner::Ownership::nativeScenery,false);
    assert(withoutBoxes->valid()!=restricted);
    // The restricted build's compile-time ownership must not turn rejected
    // constructor arguments into a silently accepted native scene.
    if(restricted)for(auto kind:{Owner::Ownership::reference,
            Owner::Ownership::nativeTerrain,Owner::Ownership::native}){
        auto incompatible=create(kind,true);
        assert(!incompatible->valid());
    }
}
