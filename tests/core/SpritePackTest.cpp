#include "core/SpritePack.h"
#include "core/EnemyPresentation.h"
#include "core/RunnerSpriteBounds.h"
#include "core/SpriteCollisionPack.h"
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

void testGeneratedPack(const char* path) {
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
    for(unsigned id=0;id<pack.spriteCount();++id){
        downwell::PackedSprite full;downwell::PackedSpriteCollision small;
        require(pack.sprite(id,full)&&compact.sprite(id,small),"both metadata views resolve every ID");
        require(full.originX==small.originX&&full.originY==small.originY&&
            full.marginLeft==small.marginLeft&&full.marginTop==small.marginTop&&
            full.marginRight==small.marginRight&&full.marginBottom==small.marginBottom&&
            full.collisionMode==small.collisionMode&&full.collisionMaskCount==small.collisionMaskCount&&
            full.frameCount==small.frameCount,"all source collision fields agree across views");
    }
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
    if (argc == 2) testGeneratedPack(argv[1]);
    std::cout << "sprite pack tests passed\n";
    return 0;
}
