#include "core/SpritePack.h"
#include "core/EnemyPresentation.h"
#include "core/RunnerSpriteBounds.h"
#include "core/SpriteCollisionPack.h"
#include "core/RunnerSpatialInstances.h"
#include "core/RunnerObjectFamilies.h"
#include "core/StoredCollisionLifecycle.h"
#include "core/RoomLoader.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testBasicValidation() {
    const downwell::SpritePack empty(nullptr, 0);
    require(empty.error() == downwell::SpritePackError::tooShort,
            "empty sprite pack is rejected");
    const std::uint8_t bad[] = {'N', 'O', 'P', 'E', 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0};
    require(downwell::SpritePack(bad, sizeof(bad)).error() ==
                downwell::SpritePackError::badMagic,
            "bad sprite-pack magic is rejected");
}

void testGeneratedPack(const char* path,const char* maskPath,const char* objectPath) {
    std::ifstream input(path, std::ios::binary);
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                          std::istreambuf_iterator<char>());
    const downwell::SpritePack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated sprite pack passes native validation");
    std::string collisionPath(path);collisionPath.replace(collisionPath.find_last_of('.'),std::string::npos,".dwsc");
    std::ifstream collisionInput(collisionPath,std::ios::binary);
    const std::vector<std::uint8_t> compactBytes((std::istreambuf_iterator<char>(collisionInput)),
                                                std::istreambuf_iterator<char>());
    const downwell::SpriteCollisionPack compact(compactBytes.data(),compactBytes.size());
    require(compact.valid()&&compact.spriteCount()==pack.spriteCount(),"compact collision table validates");
    require(compactBytes.size()==26700,"collision-only table excludes atlas payload and frame mappings");
    {
        std::ifstream objectInput(objectPath,std::ios::binary);
        const std::vector<std::uint8_t> objectBytes((std::istreambuf_iterator<char>(objectInput)),{});
        const downwell::ObjectPack objects(objectBytes.data(),objectBytes.size());
        downwell::RunnerObjectFamilies<386> families;
        require(families.bindInstanceFlags(objects),"load complete object binding flags");
        using Owner=downwell::RunnerSpatialInstances<2,8>;
        for(unsigned id=0;id<objects.objectCount();++id)for(bool registered:{false,true}){
            Owner owner;Owner::Properties properties;
            properties.sprite.spriteId=-999;properties.sprite.maskId=597;
            properties.sprite.imageIndex={0x41000000u};properties.timeline={0x41000000u};
            require(owner.attach({0,1},0,{}),"attach object-binding lifetime");
            require(owner.bindObjectFromPacks({0,1},int(id),registered,properties,objects,compact,families,false),
                    "bind complete original object through both resource packs");
            downwell::PackedObject original;require(objects.object(id,original),"read independent object record");
            require(properties.sprite.spriteId==original.spriteId&&properties.sprite.maskId==original.textureMaskId,
                    "binding restores object artwork and explicit mask defaults");
            Owner::Real depth;require(depth.assign(downwell::MotionBinary64::fromInteger(original.depth)),"convert original depth");
            require(properties.depth.bits==depth.bits,"binding uses serialized depth, not compatibility creation override");
            downwell::PackedSprite artwork;
            const unsigned frames=original.spriteId>=0&&pack.sprite(unsigned(original.spriteId),artwork)?artwork.frameCount:0;
            const bool reset=8>=frames;
            require(properties.sprite.imageIndex.bits==(reset?0u:0x41000000u)&&
                    properties.timeline.bits==(reset&&frames?0u:0x41000000u),"binding composes original sprite resets");
            const unsigned flags=8u|(original.solid()?0x20u:0u)|(original.visible()?0x10u:0u)|
                (original.persistent()?0x40u:0u)|(original.managed()?0x200000u:0u);
            require(owner.resolve({0,1})->flags==flags,"binding installs source instance flags without changing lifetime");
            const bool queued=original.solid()||(*families.flags(id)&0x28u);
            require(owner.pendingSpatial()==unsigned(queued)&&
                    owner.pendingCollision()==unsigned(queued&&(*families.flags(id)&8u)),"binding queues only eligible families");
        }
        Owner incomplete;Owner::Properties unchanged;
        require(incomplete.attach({0,1},0,{}),"attach incomplete-graph fixture");
        downwell::RunnerObjectFamilies<386> spatialOnly;require(spatialOnly.bindSpatialFlags(objects),"load deliberately incomplete flags");
        require(!incomplete.bindObjectFromPacks({0,1},0,true,unchanged,objects,compact,spatialOnly,false)&&
                !incomplete.valid()&&unchanged.sprite.spriteId==-1,"reject missing property flags before mutation");
        using StoredOwner=downwell::RunnerSpatialInstances<4,8>;
        StoredOwner storedOwner;StoredOwner::Properties properties[4];
        downwell::InstanceStore<4> records;
        auto trig=[](auto,auto&,auto&){return false;}; // Zero angle must bypass trigonometry.
        auto round=[](auto value,int& out){return value.promoted().roundPixel(out);};
        downwell::StoredCollisionLifecycle<4,8,decltype(families),decltype(trig),decltype(round)>
            lifecycle{storedOwner,properties,objects,compact,families,trig,round};
        downwell::PackedObject field;require(objects.findObject(82,field),"read placed field defaults");
        downwell::RoomInstanceDefinition definition{123,82,false};
        definition.spriteId=field.spriteId;definition.maskId=field.textureMaskId;
        definition.x=32;definition.y=64;definition.scaleX=0.5f;definition.depth=-123;
        unsigned callbacks=0;
        auto event=[&](downwell::InstanceRecord&,downwell::RoomInstancePhase phase){
            if(phase!=downwell::RoomInstancePhase::preCreate)return;
            ++callbacks;const auto h=records.handleAt(0);const auto* state=storedOwner.resolve(h);
            require(state&&!(state->flags&8u),"resource-backed bounds published before PreCreate");
            int depth;
            require(properties[h.slot].sprite.scaleX.bits==0x3f000000u&&properties[h.slot].depth.promoted().roundPixel(depth)&&depth==-123,
                    "placed scale and layer depth override object defaults before callbacks");
        };
        auto room=[](int){};auto deferred=[](auto&){};
        require(downwell::RoomLoader<4>::enter(records,7,&definition,1,true,event,room,deferred,lifecycle)==downwell::LoadRoomResult::success&&callbacks==1,
                "real-resource lifecycle composes with room loading");
        const auto old=records.handleAt(0);
        require(downwell::RoomLoader<4>::enter(records,8,nullptr,0,false,event,room,deferred,lifecycle)==downwell::LoadRoomResult::success&&
                !storedOwner.resolve(old)&&records.empty(),"room transition retires the resource-backed owner");
        definition.rotation=30.0f;
        require(downwell::RoomLoader<4>::enter(records,9,&definition,1,false,event,room,deferred,lifecycle)==downwell::LoadRoomResult::lifecycleFailure&&
                callbacks==1&&!storedOwner.valid(),"unsupported bounds calculation stops before PreCreate and invalidates collision owner");
    }
    {
        using Owner=downwell::RunnerSpatialInstances<2,8>;
        unsigned empty=0;
        for(unsigned id=0;id<pack.spriteCount();++id){
            downwell::PackedSprite original;require(pack.sprite(id,original),"read original frame count");
            for(int eighth:{-9,-1,0,7,int(original.frameCount*8),int(original.frameCount*8+1)}){
                Owner owner;Owner::State clean;clean.flags=0;
                require(owner.attach({0,1},1,clean),"attach sprite resource test");
                Owner::Sprite sprite;sprite.spriteId=-999;sprite.maskId=597;
                const float index=float(eighth)/8.0f;
                std::memcpy(&sprite.imageIndex.bits,&index,4);
                const auto before=sprite.imageIndex;
                Owner::Real timeline{0x40000000u},cached{0x3f800000u};
                require(owner.changeSpriteFromPack({0,1},sprite,timeline,cached,int(id),compact,0x38),"resource-backed sprite change succeeds");
                const bool reset=eighth/8>=int(original.frameCount);
                require(sprite.spriteId==int(id)&&sprite.maskId==597&&sprite.imageIndex.bits==(reset?0u:before.bits),
                    "resource count drives reset without replacing explicit collision mask");
                require(timeline.bits==(reset&&original.frameCount?0u:0x40000000u)&&cached.bits==0xbf800000u,
                    "empty resources preserve sequence position while invalidating cached frame");
                require(owner.pendingSpatial()==1&&owner.pendingCollision()==1,"resource change queues both eligible updates");
            }
            empty+=original.frameCount==0;
        }
        require(empty==3,"all three original empty sprites covered");
        Owner owner;require(owner.attach({0,1},1,{}),"attach missing-resource fixture");
        Owner::Sprite sprite;sprite.spriteId=208;Owner::Real timeline{0x40000000u},cached{};
        const downwell::SpriteCollisionPack invalid(nullptr,0);
        require(owner.changeSpriteFromPack({0,1},sprite,timeline,cached,208,invalid,0x38)&&owner.valid(),
            "same-ID setter does not consult unavailable metadata");
        require(owner.changeSpriteFromPack({0,1},sprite,timeline,cached,999999,compact,0x38)&&timeline.bits==0x40000000u,
            "absent sprite ID is a zero-frame resource, not a pack error");
        const auto before=sprite.spriteId;
        require(!owner.changeSpriteFromPack({0,1},sprite,timeline,cached,208,invalid,0x38)&&!owner.valid()&&sprite.spriteId==before,
            "unavailable pack fails before sprite mutation");
    }
    std::ifstream maskInput(maskPath,std::ios::binary);
    const std::vector<std::uint8_t> maskBytes((std::istreambuf_iterator<char>(maskInput)),{});
    const downwell::SpriteMaskPack masks(maskBytes.data(),maskBytes.size());
    require(masks.valid(),"all-partner mask pack resolves");
    {
        using Collisions=downwell::RunnerSpatialInstances<4,8>;
        Collisions collisions;Collisions::Sprite visuals[2];
        visuals[0].spriteId=84;visuals[1].spriteId=597;
        visuals[1].imageIndex.bits=0xc0f80000; // -7.75 truncates then wraps onto the one mask.
        require(collisions.attach({0,1},3,{})&&collisions.attach({1,1},1,{}),"attach collision partners");
        std::vector<unsigned> refreshOrder;
        auto round=[](auto v,std::int32_t& pixel){return v.promoted().roundPixel(pixel);};
        auto noTrig=[](auto,auto&,auto&){require(false,"unrotated pair must not request trig");return false;};
        auto refresh=[&](downwell::InstanceHandle h,bool publish){
            refreshOrder.push_back(h.slot);
            return collisions.refreshSprite(h,0x30,false,compact,visuals[h.slot],true,noTrig,round,publish);
        };
        bool hit=false;
        auto collide=[&](bool update,const downwell::SpriteMaskPack& pixels){
            return collisions.collideCompatibility({0,1},{1,1},visuals[0],visuals[1],compact,pixels,{},
                update,refresh,noTrig,round,hit);
        };
        require(collide(true,masks)&&hit&&refreshOrder==std::vector<unsigned>({1,0}),
            "source collision dispatch refreshes candidate before self then samples original masks");
        struct Graph {
            std::uint32_t value=0x30;
            bool valid()const{return true;}
            const std::uint32_t* flags(unsigned)const{return &value;}
            bool isA(int child,int parent)const{return child==parent;}
            bool markSpatialFamily(unsigned){return true;}
        } graph;
        Collisions::Result selected;
        require(collisions.place({0,1},1,{}, {},false,graph,refresh,
            [&](auto a,auto b,bool update,bool& found){
                require(a.slot==0&&b.slot==1,"query resolves lifetime-safe candidate handles");
                return collisions.collideCompatibility(a,b,visuals[a.slot],visuals[b.slot],compact,masks,{},
                    update,refresh,noTrig,round,found);
            },selected)&&selected.found&&selected.handle.slot==1&&selected.handle.generation==1,
            "instance-place owner reaches resource-backed pair dispatch and returns field identity");
        require(collisions.move({0,1},{0x42200000},{},0x30),"move blood away from the field");
        refreshOrder.clear();
        require(collide(false,masks)&&!hit&&refreshOrder.empty()&&(collisions.resolve({0,1})->flags&8),
            "update-disabled collision leaves dirty caches untouched and uses current sampling position");
        require(collide(true,masks)&&!hit&&refreshOrder==std::vector<unsigned>({0}),
            "updated separated pair fails broad phase");
        require(collisions.move({0,1},{},{},0x30),"restore blood position");
        visuals[1].spriteId=84;visuals[1].imageIndex.bits=0x7f800000;
        require(refresh({1,1},true),"refresh second rectangular resource");
        downwell::SpriteMaskPack unavailable(nullptr,0);
        require(collide(true,unavailable)&&hit,"rectangular pair requires neither mask pixels nor image-index conversion");
        visuals[1].maskId=597;visuals[1].imageIndex={};
        require(refresh({1,1},true)&&collide(true,masks)&&hit,"explicit precise mask selects stored pixels despite rectangular art");
        require(collisions.setExclusions({1,1},2),"set source flag 2");
        require(collide(true,masks)&&hit,"direct pair gate excludes 0x100001, unlike query candidate gate 0x100003");
        require(collisions.setExclusions({1,1},1),"exclude candidate");
        refreshOrder.clear();
        require(collide(true,unavailable)&&!hit&&refreshOrder.empty(),"excluded pair returns before refresh/resource reads");
        require(collisions.setExclusions({1,1},0),"restore eligibility");
        visuals[1].maskId=9999;
        require(collide(true,masks)&&!hit,"invalid explicit mask does not fall back in pair dispatch");
        visuals[1].maskId=597;hit=true;
        require(!collide(true,unavailable)&&!collisions.valid()&&hit,"missing mask payload fails owner without returning a false miss");
    }
    using Owner=downwell::RunnerSpatialInstances<2,4>;
    Owner owner;
    require(owner.attach({0,1},0,{}),"attach metadata-backed collision instance");
    auto round=[](downwell::RunnerImageValue v,std::int32_t& pixel){return v.promoted().roundPixel(pixel);};
    auto noTrig=[](auto,auto&,auto&){require(false,"zero angle must not invoke trigonometry");return false;};
    for(unsigned id=0;id<pack.spriteCount();++id){
        downwell::PackedSprite full;downwell::PackedSpriteCollision small;
        require(pack.sprite(id,full)&&compact.sprite(id,small),"both metadata views resolve every ID");
        require(full.originX==small.originX&&full.originY==small.originY&&
            full.marginLeft==small.marginLeft&&full.marginTop==small.marginTop&&
            full.marginRight==small.marginRight&&full.marginBottom==small.marginBottom&&
            full.collisionMode==small.collisionMode&&full.collisionMaskCount==small.collisionMaskCount&&
            full.frameCount==small.frameCount,"all source collision fields agree across views");
        Owner::Sprite visual;visual.spriteId=id;
        downwell::RunnerSpriteBounds::Box expected;
        require(downwell::RunnerSpriteBounds::fromSprite(full,{}, {},visual.scaleX,visual.scaleY,
            false,{}, {},true,round,expected),"full metadata bounds resolve");
        require(owner.refreshSprite({0,1},0x30,false,compact,visual,true,noTrig,round,true),
            "compact metadata drives owner publication");
        const auto* actual=owner.resolve({0,1});
        require(actual->bounds.left.bits==expected.left.bits&&actual->bounds.top.bits==expected.top.bits&&
            actual->bounds.right.bits==expected.right.bits&&actual->bounds.bottom.bits==expected.bottom.bits&&
            actual->flags==(0x8000u|(full.collisionMode==1?0x200u:0u)),
            "all sprite resources update cached bounds, precise and indexed flags");
    }
    Owner::Sprite visual;visual.spriteId=84;visual.maskId=597;
    require(owner.refreshSprite({0,1},0x30,false,compact,visual,true,noTrig,round,true)&&
        (owner.resolve({0,1})->flags&0x200),"explicit precise mask overrides rectangular artwork in owner");
    visual.maskId=9999;
    require(owner.refreshSprite({0,1},0x30,false,compact,visual,true,noTrig,round,true)&&
        owner.resolve({0,1})->flags==0x8000&&owner.resolve({0,1})->bounds.left.bits==0,
        "missing explicit mask collapses cache without discarding existing index membership");
    require(owner.retire({0,1}),"resource-backed stale bounds retire with root retry");
    require(owner.attach({0,2},0,{}),"reuse retired slot for rotated metadata adapter");
    visual.maskId=-1;visual.angle.bits=0x42b40000; // 90 degrees, explicit narrowed trig input.
    auto quarterTurn=[](auto angle,auto& cosine,auto& sine){
        require(angle.bits==0x42b40000,"trig receives the original binary32 degrees");
        cosine={};sine={0x3f800000};return true;
    };
    require(owner.refreshSprite({0,2},0x30,false,compact,visual,true,quarterTurn,round,false),
        "rotated refresh can update cache without publishing");
    const auto* rotated=owner.resolve({0,2});
    require(rotated->flags==0&&rotated->bounds.left.bits==0xbf800000&&
        rotated->bounds.right.bits==0x3f800000&&rotated->bounds.top.bits==0xc0000000&&
        rotated->bounds.bottom.bits==0x3f800000,"rotated adapter uses collision margins and update flag");
    require(!owner.refreshSprite({0,2},0x30,false,downwell::SpriteCollisionPack(nullptr,0),
        visual,true,quarterTurn,round,true)&&!owner.valid(),"invalid asset pack fails loudly instead of resolving no sprite");
    owner.clear();Owner::State special;special.special=true;
    require(owner.attach({0,3},0,special),"attach unsupported special state");
    visual.maskId=9999;visual.angle.bits=0x7f800000;
    require(owner.refreshSprite({0,3},0x30,false,compact,visual,true,noTrig,round,true),
        "missing resource resolves before angle or special-state processing");
    visual.maskId=84;visual.angle={};
    require(owner.refreshSprite({0,3},0x30,false,compact,visual,true,noTrig,round,true),
        "explicit ordinary mask can override special artwork state");
    visual.maskId=-1;
    require(!owner.refreshSprite({0,3},0x30,false,compact,visual,true,noTrig,round,true)&&!owner.valid(),
        "resolved special state cannot silently use ordinary sprite bounds");
    owner.clear();require(owner.attach({0,4},0,{}),"attach fractional bounds case");
    visual.angle.bits=0x80000000;visual.scaleX.bits=0xbf000000; // Negative zero angle, mirrored half scale.
    unsigned roundCalls=0;
    auto rejectRound=[&](auto,std::int32_t&){++roundCalls;return false;};
    require(owner.refreshSprite({0,4},0x30,false,compact,visual,false,noTrig,rejectRound,false)&&!roundCalls,
        "fractional mode and negative-zero angle bypass rounding and trigonometry");
    const auto fractional=owner.resolve({0,4})->bounds;
    require(!owner.refreshSprite({0,4},0x30,false,compact,visual,true,noTrig,rejectRound,true)&&roundCalls==1,
        "rounding failure propagates without publication");
    require(!owner.valid()&&owner.resolve({0,4})->bounds.left.bits==fractional.left.bits&&
        owner.resolve({0,4})->bounds.right.bits==fractional.right.bits,
        "failed bounds calculation does not replace the last cached rectangle");
    require(!downwell::SpriteCollisionPack(nullptr,0).valid(),"null compact table rejected");
    for(auto length:{std::size_t(0),std::size_t(23),compactBytes.size()-1})
        require(!downwell::SpriteCollisionPack(compactBytes.data(),length).valid(),"truncated compact table rejected");
    for(unsigned offset:{0u,4u,8u,16u,20u,48u}){
        auto bad=compactBytes;bad[offset]=255;
        require(!downwell::SpriteCollisionPack(bad.data(),bad.size()).valid(),"corrupt compact header/mode rejected");
    }
    downwell::PackedSpriteCollision selected;
    require(compact.collisionSprite(84,597,selected)&&selected.collisionMode==1,"compact mask override resolves");
    require(!compact.collisionSprite(84,9999,selected)&&selected.collisionMode==1,"compact invalid mask does not fall back");
    require(compact.collisionSprite(84,-1,selected)&&selected.marginLeft==10,"compact negative mask falls back");
    downwell::RunnerSpriteBounds::Box compactBounds;
    require(downwell::RunnerSpriteBounds::fromSprite(selected,{}, {},{0x3f800000},{0x3f800000},false,{}, {},true,
        [](downwell::RunnerImageValue v,std::int32_t& pixel){return v.promoted().roundPixel(pixel);},compactBounds)&&
        compactBounds.left.bits==0xc0000000&&compactBounds.right.bits==0x3f800000,
        "compact metadata feeds the same geometry adapter");
    require(pack.spriteCount() == 741 && pack.frameCount() == 3152 &&
                pack.pageItemCount() == 3161 && pack.texturePageCount() == 2,
            "generated sprite pack has audited totals");
    downwell::PackedSprite sprite;
    require(pack.sprite(84,sprite) && sprite.marginLeft==10 && sprite.marginTop==11 &&
        sprite.marginRight==13 && sprite.marginBottom==13 && sprite.collisionMode==0 && sprite.collisionMaskCount==1,
        "blood sprite retains original collision margins, not full artwork bounds");
    require(pack.sprite(597,sprite) && sprite.marginLeft==0 && sprite.marginTop==0 &&
        sprite.marginRight==47 && sprite.marginBottom==47 && sprite.collisionMode==1 && sprite.collisionMaskCount==1,
        "time-field precise collision metadata is retained");
    unsigned precise=0;
    for(unsigned id=0;id<pack.spriteCount();++id){
        require(pack.sprite(id,sprite),"every source sprite resolves");precise+=sprite.collisionMode==1;
    }
    require(precise==4,"all four precise-mask resources are represented");
    require(pack.collisionSprite(84,-1,sprite)&&sprite.spriteId==84,"negative mask selects artwork");
    downwell::RunnerSpriteBounds::Box bounds;
    require(downwell::RunnerSpriteBounds::fromSprite(sprite,{}, {},{0x3f800000},{0x3f800000},false,{}, {},true,
        [](downwell::RunnerImageValue value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},bounds),
        "decoded margins feed shared bounds arithmetic");
    require(bounds.left.bits==0xc0000000 && bounds.top.bits==0xbf800000 &&
        bounds.right.bits==0x3f800000 && bounds.bottom.bits==0x3f800000,"blood collision bounds use decoded origin and margins");
    require(pack.collisionSprite(84,597,sprite)&&sprite.spriteId==597,"explicit mask overrides artwork");
    require(!pack.collisionSprite(84,9999,sprite)&&sprite.spriteId==597,"missing explicit mask does not fall back or modify output");
    require(!pack.collisionSprite(-1,-1,sprite),"missing artwork and mask resolve no resource");
    auto corrupt=bytes;corrupt[4]=1;
    require(downwell::SpritePack(corrupt.data(),corrupt.size()).error()==downwell::SpritePackError::unsupportedVersion,
        "old record layout is rejected");
    corrupt=bytes;corrupt[downwell::SpritePack::headerSize+48]=2;
    require(downwell::SpritePack(corrupt.data(),corrupt.size()).error()==downwell::SpritePackError::badSprite,
        "unknown collision mode is rejected");
    // Include the Step-selected hit/hang/wake poses, not only each enemy's
    // default art. Both runtimes wrap animation with this shared table.
    for (int id : {153, 154, 157, 162, 163, 164, 166, 168, 170, 173,
                   177, 178, 186, 187, 188, 278})
        require(pack.sprite(id, sprite) &&
                    sprite.frameCount == downwell::enemySpriteFrameCount(id),
                "shared enemy animation lengths match serialized source resources");
    require(pack.sprite(733, sprite) && sprite.width == 8 && sprite.height == 8 &&
                sprite.frameCount == 12,
            "sprDitherFade metadata is decoded");
    downwell::PackedTexturePageItem frame;
    require(pack.spriteFrame(sprite, 0, frame) && frame.itemId == 2469 &&
                frame.texturePageId == 1 && frame.sourceX == 382 &&
                frame.sourceY == 1640 && frame.sourceWidth == 8 &&
                frame.sourceHeight == 8,
            "sprDitherFade frame zero resolves to its recovered atlas rectangle");
}
} // namespace

int main(int argc, char** argv) {
    testBasicValidation();
    require(argc==4,"sprite, collision-mask and object pack paths required");
    testGeneratedPack(argv[1],argv[2],argv[3]);
    std::cout << "sprite pack tests passed\n";
    return 0;
}
