inline void* operator new(__SIZE_TYPE__,void* address) noexcept{return address;}
#include "core/CorpseVerification.h"
#include "core/CorpseExpiryEffect.h"
downwell::CorpseVerification compileCorpseVerification(){return downwell::verifyCorpses();}
bool compileCorpseExpiry(downwell::CorpseExpiryEffect& effect,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,int sprite,unsigned frames){
    return effect.create(x,y)&&effect.setSprite(sprite,frames,[](const auto&){return true;})&&effect.advanceImage();
}
