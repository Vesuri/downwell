#ifndef DOWNWELL_CORE_SHOP_RISING_TEXT_H
#define DOWNWELL_CORE_SHOP_RISING_TEXT_H
#include "MotionBinary64.h"
namespace downwell {
// scrRisingTextShop caller fields, separate from the child's text and builtins.
// Strings borrow persistent language storage through the child's retirement.
struct ShopRisingText {
    MotionBinary64 baseX{},baseY{};
    const char* drawText=nullptr;
    template<class Ops> bool replace(std::uint32_t& current,MotionBinary64 x,MotionBinary64 y,
        const char* text,Ops& ops){
        if(!text)return false;
        baseX=x;baseY=y;drawText=text;
        if(!ops.destroyPrevious(current))return false;
        unsigned next=0;
        if(!ops.create(baseX,baseY,next))return false;
        current=next;
        return ops.text(current,drawText);
    }
};
}
#endif
