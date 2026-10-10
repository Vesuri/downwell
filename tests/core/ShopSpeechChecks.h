#ifndef DOWNWELL_TEST_SHOP_SPEECH_CHECKS_H
#define DOWNWELL_TEST_SHOP_SPEECH_CHECKS_H
#include "core/ShopRisingText.h"
inline bool checkShopSpeechScript(){
    using namespace downwell;
    for(unsigned reject=0;reject<=3;++reject){
        ShopRisingText state;std::uint32_t current=123;
        struct Ops {
            ShopRisingText& state;std::uint32_t& current;unsigned reject,calls=0;
            bool event(){return ++calls!=reject;}
            bool destroyPrevious(unsigned id){
                if(id!=123||state.baseX.high!=0x3ff00000u||state.baseY.high!=0x40000000u||!state.drawText)return false;
                state.baseY=MotionBinary64::fromInteger(3);state.drawText="changed";return event();
            }
            bool create(MotionBinary64 x,MotionBinary64 y,unsigned& id){
                if(current!=123||x.high!=0x3ff00000u||y.high!=0x40080000u)return false;
                id=124;return event();
            }
            bool text(unsigned id,const char* value){
                return id==124&&current==124&&value==state.drawText&&event();
            }
        } ops{state,current,reject};
        if(state.replace(current,MotionBinary64::fromInteger(1),MotionBinary64::fromInteger(2),"hello",ops)!=!reject||
           ops.calls!=(reject?reject:3)||current!=(reject==1||reject==2?123u:124u))return false;
    }
    return true;
}
#endif
