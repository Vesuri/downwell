#include "core/ObjectPack.h"
#include "core/RunnerObjectFamilies.h"
#include "core/RunnerObjectMembership.h"
#include "core/RunnerSpatialObjectPreparation.h"
#include "core/RunnerSpatialIndex.h"
#include <cstdlib>
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

void appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void appendI32(std::vector<std::uint8_t>& bytes, std::int32_t value) {
    appendU32(bytes, static_cast<std::uint32_t>(value));
}

void appendU64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
    appendU32(bytes, static_cast<std::uint32_t>(value));
    appendU32(bytes, static_cast<std::uint32_t>(value >> 32));
}

void appendObject(std::vector<std::uint8_t>& bytes,
                  std::int32_t objectId,
                  std::int32_t parentId,
                  std::uint32_t firstBinding) {
    appendI32(bytes, objectId);
    appendI32(bytes, 31);
    appendI32(bytes, parentId);
    appendI32(bytes, 31);
    appendI32(bytes, -10);
    appendU32(bytes, 1);
    appendU32(bytes, firstBinding);
    appendU32(bytes, 1);
    appendU32(bytes, 0);
}

void appendBinding(std::vector<std::uint8_t>& bytes,
                   std::int32_t type,
                   std::int32_t subtype,
                   std::int32_t codeId) {
    appendI32(bytes, type);
    appendI32(bytes, subtype);
    appendI32(bytes, codeId);
    appendU32(bytes, 0);
}

std::vector<std::uint8_t> makePack() {
    std::vector<std::uint8_t> bytes;
    bytes.insert(bytes.end(), {'D', 'W', 'O', 'B'});
    appendU16(bytes, downwell::ObjectPack::formatVersion);
    appendU16(bytes, downwell::ObjectPack::headerSize);
    appendU32(bytes, downwell::ObjectPack::manifestSchema);
    appendU32(bytes, 2);
    appendU32(bytes, 2);
    appendU64(bytes, downwell::ObjectPack::sourceHashPrefix);
    appendU32(bytes, 0);
    appendU64(bytes, 0);

    appendObject(bytes, 0, -1, 0);
    appendObject(bytes, 1, 0, 1);
    appendBinding(bytes, 0, 0, 100);
    appendBinding(bytes, 3, 0, 200);
    return bytes;
}

void testDecodeAndInheritance() {
    const std::vector<std::uint8_t> bytes = makePack();
    const downwell::ObjectPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "valid object pack is accepted");
    downwell::RunnerObjectFamilies<2> spatialFlags;
    require(spatialFlags.bindSpatialFlags(pack) && *spatialFlags.flags(0)==0x10 && *spatialFlags.flags(1)==0x10,
            "objects with sprites but no collision events are not collision-pair prepared");
    auto collisionBytes=makePack();
    const auto eventOffset=downwell::ObjectPack::headerSize+2*downwell::ObjectPack::objectRecordSize;
    collisionBytes[eventOffset]=4; // Root's event targets root: child inherits both sides.
    downwell::ObjectPack rootCollision(collisionBytes.data(),collisionBytes.size());
    require(spatialFlags.bindSpatialFlags(rootCollision) && *spatialFlags.flags(0)==0x18 && *spatialFlags.flags(1)==0x18,
            "collision events mark inheriting owners and descendant targets");
    collisionBytes[eventOffset]=0;
    collisionBytes[eventOffset+downwell::ObjectPack::bindingRecordSize]=4;
    for(unsigned i=0;i<4;++i)collisionBytes[eventOffset+downwell::ObjectPack::bindingRecordSize+4+i]=0xff;
    downwell::ObjectPack missingTarget(collisionBytes.data(),collisionBytes.size());
    require(spatialFlags.bindSpatialFlags(missingTarget) && *spatialFlags.flags(0)==0x10 && *spatialFlags.flags(1)==0x18,
            "missing target still marks collision owner, never its ancestors");
    require(pack.objectCount() == 2 && pack.bindingCount() == 2,
            "object pack counts are decoded");

    downwell::PackedObject child;
    require(pack.findObject(1, child), "child object is found by resource ID");
    require(child.parentId == 0 && child.spriteId == 31 && child.visible(),
            "object properties and flags are decoded");

    downwell::PackedObjectEvent event;
    downwell::RunnerObjectFamilies<2> families;
    require(families.bind(pack,[](const auto&){return 0x10u;}),"bind immutable object topology");
    require(families.isA(1,0) && families.isA(1,1) && !families.isA(0,1) && !families.isA(-1,0),
            "family traversal follows parent links and validates selectors");
    require(families.markSpatialFamily(0) && *families.flags(0)==0x30 && *families.flags(1)==0x30,
            "preparing parent marks descendants");
    require(families.bind(pack,[](const auto& o){return o.objectId==0?8u:0x10u;}),"bind subtree barrier");
    require(families.markSpatialFamily(0) && *families.flags(0)==8 && *families.flags(1)==0x10,
            "collision-pair root prevents descendant preparation");
    require(families.markSpatialFamily(1) && *families.flags(1)==0x30,
            "explicit child query can prepare below an ancestor barrier");
    require(families.bind(pack,[](const auto& o){return o.objectId==0?8u:0x10u;}) &&
                families.prepareBirth(1) && *families.flags(0)==8 && *families.flags(1)==0x30,
            "birth inherits prepared ancestor flag without modifying the ancestor");
    require(!families.prepareBirth(2),"invalid birth object cannot mutate flags");
    auto copied=families;families.clear();
    require(!families.markSpatialFamily(0) && !families.flags(0) && copied.isA(1,0),
            "copied topology owns links independently and clear invalidates access");
    downwell::RunnerObjectFamilies<1> small;
    require(!small.bind(pack,[](const auto&){return 0u;}),"family capacity failure is explicit");
    require(families.bind(pack,[](const auto&){return 0x10u;}),"bind query-chain topology");
    downwell::RunnerObjectMembership<2> membership;
    require(membership.bind(0,1,101) && membership.bind(1,0,100),"register child before parent instance");
    auto isA=[&](int childId,int parentId){return families.isA(childId,parentId);};
    using Prepare=downwell::RunnerSpatialObjectPreparation;
    downwell::RunnerSpatialIndex<4> spatial;
    std::uint32_t instanceFlags[2]={0,0};
    Prepare::Box bounds[2]={};
    for(auto& box:bounds){box.right.bits=box.bottom.bits=0x3f800000u;}
    const auto path=Prepare::prepare(membership.count(0,isA),spatial.height(),*families.flags(0),false,
        [&]{return families.markSpatialFamily(0);},
        [&](auto emit){membership.visit(0,isA,[&](unsigned slot,std::uint32_t key){
            return emit(key,instanceFlags[slot],bounds[slot]);});},
        [](auto,auto&,auto&){return true;},
        [&](auto box,auto key){return spatial.insert(box,key);});
    require(path==Prepare::Path::spatial && *families.flags(1)==0x30,
            "query-chain preparation marks the object family");
    std::vector<unsigned> keys;
    require(spatial.query(bounds[0],[&](unsigned key){keys.push_back(key);return true;}),"query populated index");
    require(keys==std::vector<unsigned>({101,100}),
            "family filtering preserves cross-object registration order into spatial publication");
    require(pack.findEvent(1, 3, 0, event) && event.codeId == 200 &&
                event.ownerObjectId == 1,
            "direct child event resolves first");
    require(pack.findEvent(1, 0, 0, event) && event.codeId == 100 &&
                event.ownerObjectId == 0,
            "missing child event resolves through parent");
}

void testValidation() {
    std::vector<std::uint8_t> bytes = makePack();
    for(unsigned flags=0;flags<32;++flags){
        bytes=makePack();bytes[downwell::ObjectPack::headerSize+20]=flags;
        const downwell::ObjectPack properties(bytes.data(),bytes.size());
        downwell::RunnerObjectFamilies<2> family;
        require(family.bindInstanceFlags(properties),"bind all serialized property combinations");
        const unsigned expected=0x10u|((flags&2)?1u:0u)|((flags&1)?2u:0u)|
            (flags&4u)|((flags&16)?0x80u:0u);
        require(*family.flags(0)==expected,"physics is not managed and visible/solid bits are transposed");
    }
    bytes=makePack();bytes[4]=2;
    require(downwell::ObjectPack(bytes.data(),bytes.size()).error()==downwell::ObjectPackError::unsupportedVersion,
            "legacy pack without managed properties is rejected");
    bytes=makePack();
    bytes[4] = 1;
    require(downwell::ObjectPack(bytes.data(), bytes.size()).error() ==
                downwell::ObjectPackError::unsupportedVersion,
            "legacy pack without compatibility depths is rejected");
    bytes = makePack();
    bytes[0] = 'X';
    require(downwell::ObjectPack(bytes.data(), bytes.size()).error() ==
                downwell::ObjectPackError::badMagic,
            "bad magic is rejected");

    bytes = makePack();
    bytes.pop_back();
    require(downwell::ObjectPack(bytes.data(), bytes.size()).error() ==
                downwell::ObjectPackError::sizeOverflow,
            "truncated object pack is rejected");

    bytes = makePack();
    // Make object zero inherit from object one, closing the 0 -> 1 -> 0 cycle.
    const std::size_t parentOffset = downwell::ObjectPack::headerSize + 8;
    bytes[parentOffset + 0] = 1;
    bytes[parentOffset + 1] = 0;
    bytes[parentOffset + 2] = 0;
    bytes[parentOffset + 3] = 0;
    require(downwell::ObjectPack(bytes.data(), bytes.size()).error() ==
                downwell::ObjectPackError::inheritanceCycle,
            "inheritance cycles are rejected");
}

void testGeneratedPack(const char* path) {
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "generated object pack opens");
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                          std::istreambuf_iterator<char>());
    const downwell::ObjectPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated object pack passes native validation");
    require(pack.objectCount() == 386 && pack.bindingCount() == 2075,
            "generated object pack has audited totals");
    downwell::RunnerObjectFamilies<386> collisionFamilies;
    require(collisionFamilies.bindSpatialFlags(pack),"derive initial spatial flags from recovered collisions");
    downwell::RunnerObjectFamilies<386> instanceFamilies;
    require(instanceFamilies.bindInstanceFlags(pack),"derive runtime instance property flags");
    unsigned participants=0;
    for(unsigned id=0;id<pack.objectCount();++id){
        bool participates=false;
        for(unsigned owner=0;owner<pack.objectCount();++owner){
            downwell::PackedObject object;pack.object(owner,object);
            for(unsigned j=0;j<object.bindingCount;++j){
                downwell::PackedObjectEvent event;pack.binding(object.firstBinding+j,event);
                if(event.eventType!=4)continue;
                // Independent ancestor walks, not the implementation's subtree
                // links/marking or its already-marked subtree optimization.
                for(int ancestor=int(id);ancestor>=0;){
                    if(ancestor==int(owner) || ancestor==event.eventSubtype)participates=true;
                    downwell::PackedObject parent;pack.findObject(ancestor,parent);ancestor=parent.parentId;
                }
            }
        }
        require(*collisionFamilies.flags(id)==(participates?0x18u:0x10u),
                "initial collision flags match expanded event endpoint ancestry");
        downwell::PackedObject properties;require(pack.object(id,properties),"read property flags");
        require(properties.managed(),"all pinned OBJT resources retain their managed flag");
        const unsigned expected=*collisionFamilies.flags(id)|(properties.solid()?1u:0u)|
            (properties.visible()?2u:0u)|(properties.persistent()?4u:0u)|0x80u;
        require(*instanceFamilies.flags(id)==expected,"runtime flags map serialized properties without conflating bit layouts");
        participants+=participates;
    }
    require(participants>0 && participants<pack.objectCount(),"collision flags do not indiscriminately index every object");

    downwell::PackedObject player;
    require(pack.findObject(35, player) && player.spriteId == 31 &&
                player.parentId == 34 && player.bindingCount == 29,
            "recovered player object properties are decoded");
    require(player.depth == 0 && player.creationDepth == -50000,
            "player compatibility depth is distinct from serialized depth");
    downwell::PackedObject field;
    require(pack.findObject(82, field) && field.depth == 0 && field.creationDepth == 10000,
            "time-field compatibility table is resolved by name, not array slot 48");
    downwell::PackedObjectEvent event;
    require(pack.findEvent(35, 0, 0, event) && event.codeId == 836 &&
                event.ownerObjectId == 35,
            "recovered player Create event resolves to its VM code ID");
    require(pack.findEvent(35, 14, 0, event) && event.codeId == 864,
            "recovered player PreCreate event resolves to its VM code ID");
    // Independent parent-walk model: preexisting 0x28 nodes block the whole
    // descendant subtree; sibling traversal order does not enter the model.
    for(unsigned root=0;root<pack.objectCount();++root){
        downwell::RunnerObjectFamilies<386> families;
        auto initial=[](const auto& o){return 0x10u | (o.objectId%7==0?8u:0u) | (o.objectId%11==0?0x20u:0u);};
        require(families.bind(pack,initial) && families.markSpatialFamily(root),"mark recovered object family");
        for(unsigned id=0;id<pack.objectCount();++id){
            downwell::PackedObject object;pack.object(id,object);
            auto expected=initial(object);
            bool blocked=false,reached=false;
            for(int at=int(id);at>=0;){
                downwell::PackedObject ancestor;pack.findObject(at,ancestor);
                blocked|=(initial(ancestor)&0x28u)!=0;
                if(unsigned(at)==root){reached=true;break;}
                at=ancestor.parentId;
            }
            if(reached && !blocked)expected|=0x20u;
            require(*families.flags(id)==expected,"recovered family marking matches independent ancestry model");
        }
        // A birth changes only the named object even when its parent or a
        // more distant ancestor was prepared. Reinitialize between births.
        require(families.bind(pack,initial),"reset flags for recovered birth transition");
        downwell::PackedObject born;pack.object(root,born);
        auto birthExpected=initial(born);
        for(int parent=born.parentId;parent>=0;){
            downwell::PackedObject ancestor;pack.findObject(parent,ancestor);
            if(!(birthExpected&0x28u) && (initial(ancestor)&0x28u))birthExpected|=0x20u;
            parent=ancestor.parentId;
        }
        require(families.prepareBirth(root),"prepare recovered birth");
        for(unsigned id=0;id<pack.objectCount();++id){
            downwell::PackedObject other;pack.object(id,other);
            require(*families.flags(id)==(id==root?birthExpected:initial(other)),
                    "birth preparation changes neither siblings nor descendants");
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    testDecodeAndInheritance();
    testValidation();
    if (argc == 2)
        testGeneratedPack(argv[1]);
    std::cout << "object pack tests passed\n";
    return 0;
}
