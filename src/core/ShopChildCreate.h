#ifndef DOWNWELL_CORE_SHOP_CHILD_CREATE_H
#define DOWNWELL_CORE_SHOP_CHILD_CREATE_H
#include "RunnerImageValue.h"
namespace downwell {
// User variables are shared by stored and native ownership. Builtin setters
// stay with the owner so their publication occurs inside synchronous Create.
struct ShopChildState {
    enum class Material { unset,metal,rock };
    Material material=Material::unset;
    int wallHp=0,previousWallHp=0,keeper=0,storedItem=0,xsp=0;
    int opened=0,destroyed=0,flashing=0,thankful=0,goodbye=0,confused=0;
    unsigned whichUg=0;
    MotionBinary64 ysp{},decreaseBy{};
    MotionBinary64 textWidth{},drawX{},overXview{},underXview{};
    const char* text=nullptr;
    int drawing=0;
    bool initialized=false;
    template<class Builtins> bool create(int object,Builtins& builtins){
        if(initialized)return false;
        switch(object){
        case 278:material=Material::metal;break;
        case 98:material=Material::rock;break;
        case 279:
            material=Material::metal;
            if(!builtins.speed(RunnerImageValue{}))return false;
            wallHp=previousWallHp=1000000;break;
        case 280:{
            RunnerImageValue speed;
            if(!speed.assign({0x3fb70a3du,0x70a3d70au})||!builtins.speed(speed))return false;
            break;
        }
        case 212:
            ysp=MotionBinary64::fromInteger(-3);text="+100 GEMS#+50 AMMO";
            if(!builtins.alarm(0,90)||!builtins.alarm(1,60))return false;
            drawing=true;decreaseBy={0x3feb3333u,0x33333333u};break;
        case 283:
            if(!builtins.index({0x3f800000u})||!builtins.speed(RunnerImageValue{}))return false;
            break;
        default:return false;
        }
        initialized=true;return true;
    }
};
}
#endif
