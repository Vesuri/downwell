#ifndef DOWNWELL_RUNNER_SPRITE_SETTERS_H
#define DOWNWELL_RUNNER_SPRITE_SETTERS_H
#include "RunnerSpatialSplit.h"
#include "RunnerTransformField.h"
namespace downwell {
// Ordinary narrowed transform setters, 0x46b6d0/0x46b780/0x46b560.
// The owner supplies lifetime validation and dirty-queue publication.
struct RunnerSpriteSetters {
    template<class Sprite> static bool transform(Sprite& sprite,RunnerTransformField field,
        RunnerImageValue value,RunnerImageValue alpha,std::uint32_t blend,
        std::uint32_t& flags,bool& changed){
        using Math=RunnerSpatialSplit;
        changed=false;RunnerImageValue* target=nullptr;
        switch(field){
            case RunnerTransformField::scaleX:target=&sprite.scaleX;break;
            case RunnerTransformField::scaleY:target=&sprite.scaleY;break;
            case RunnerTransformField::angle:target=&sprite.angle;break;
            default:return false;
        }
        if(!Math::valid(*target)||!Math::valid(value))return false;
        if(Math::equal(*target,value))return true;
        if(!Math::valid(sprite.scaleX)||!Math::valid(sprite.scaleY)||
            !Math::valid(sprite.angle)||!Math::valid(alpha))return false;
        *target=value;
        const RunnerImageValue one{0x3f800000u};
        const bool ordinary=Math::equal(sprite.scaleX,one)&&Math::equal(sprite.scaleY,one)&&
            Math::equal(sprite.angle,RunnerImageValue{})&&Math::equal(alpha,one)&&blend==0xffffffu;
        flags=(flags&~0x4000u)|(ordinary?0x4000u:0u)|8u;
        changed=true;return true;
    }
};
}
#endif
