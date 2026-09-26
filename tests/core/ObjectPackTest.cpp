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
