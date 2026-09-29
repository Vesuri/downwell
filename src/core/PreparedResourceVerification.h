#ifndef DOWNWELL_PREPARED_RESOURCE_VERIFICATION_H
#define DOWNWELL_PREPARED_RESOURCE_VERIFICATION_H
#include "PreparedObjectDefaults.h"
#include "ObjectAlarmMetadata.h"
#include "PreparedUnitSpriteBounds.h"
namespace downwell {
// Test workspace, never a gameplay owner or an Amiga task-stack local.
// Compare pre-game tables with direct resource reads on the executing CPU.
struct PreparedResourceVerification {
    PreparedObjectDefaults<386> defaults;
    ObjectAlarmMetadata<386> alarms;
    unsigned objectsChecked=0,alarmsChecked=0,spritesChecked=0;
    bool run(const ObjectPack& objects,const SpriteCollisionPack& sprites){
        objectsChecked=alarmsChecked=spritesChecked=0;
        if(objects.objectCount()!=386||sprites.spriteCount()!=741||
            !defaults.prepare(objects,sprites)||!alarms.bind(objects))return false;
        for(unsigned id=0;id<objects.objectCount();++id){
            PackedObject source;
            const auto* prepared=defaults.get(int(id));
            if(!prepared||!objects.findObject(id,source))return false;
            RunnerImageValue depth,creationDepth;
            // Use the general binary64 conversion as the independent path.
            if(!depth.assign(MotionBinary64::fromInteger(source.depth))||
                !creationDepth.assign(MotionBinary64::fromInteger(source.creationDepth)))return false;
            std::uint32_t frames=0;
            if(source.spriteId>=0)sprites.frameCount(unsigned(source.spriteId),frames);
            if(prepared->spriteId!=source.spriteId||prepared->maskId!=source.textureMaskId||
                prepared->depth.bits!=depth.bits||prepared->creationDepth.bits!=creationDepth.bits||
                prepared->frames!=frames||
                prepared->propertyFlags!=((source.solid()?1u:0u)|(source.visible()?2u:0u)|
                    (source.persistent()?4u:0u)|(source.managed()?0x80u:0u))||
                prepared->initialFlags!=(8u|(source.solid()?0x20u:0u)|(source.visible()?0x10u:0u)|
                    (source.persistent()?0x40u:0u)|(source.managed()?0x200000u:0u)))return false;
            unsigned expectedMask=0,actualMask=0;
            for(unsigned alarm=0;alarm<12;++alarm){
                PackedObjectEvent expected{},actual{};
                const bool found=objects.findEvent(int(id),2,int(alarm),expected);
                if(alarms.findEvent(int(id),alarm,actual)!=found)return false;
                if(found){
                    expectedMask|=1u<<alarm;
                    if(actual.ownerObjectId!=expected.ownerObjectId||actual.codeId!=expected.codeId||
                        actual.eventType!=expected.eventType||actual.eventSubtype!=expected.eventSubtype)return false;
                }
                ++alarmsChecked;
            }
            if(!alarms.mask(int(id),actualMask)||actualMask!=expectedMask)return false;
            ++objectsChecked;
        }
        for(unsigned id=0;id<sprites.spriteCount();++id){
            PackedSpriteCollision sprite;if(!sprites.sprite(id,sprite))return false;
            PreparedUnitSpriteBounds prepared;prepared.prepare(sprite);
            const int positions[]={-2048,0,32768};
            for(int position:positions)for(unsigned flip=0;flip<4;++flip){
                RunnerImageValue x,y;
                if(!x.assignInteger(position)||!y.assignInteger(position+17))return false;
                RunnerSpatialSplit::Box cached{},direct{};
                if(!prepared.at(x,y,cached,flip&1,flip&2)||!RunnerSpriteBounds::fromSpriteBinary32(sprite,x,y,
                    {(flip&1)?0xbf800000u:0x3f800000u},
                    {(flip&2)?0xbf800000u:0x3f800000u},false,{},{},true,
                    [](RunnerImageValue v,std::int32_t& p){return v.roundPixel(p);},direct)||
                    cached.left.bits!=direct.left.bits||cached.right.bits!=direct.right.bits||
                    cached.top.bits!=direct.top.bits||cached.bottom.bits!=direct.bottom.bits)return false;
            }
            ++spritesChecked;
        }
        return objectsChecked==386&&alarmsChecked==4632&&spritesChecked==741;
    }
};
}
#endif
