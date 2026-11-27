#ifndef DOWNWELL_CORE_TIME_SHARD_DRAW_H
#define DOWNWELL_CORE_TIME_SHARD_DRAW_H
#include "TimeShardAnimation.h"
#include "DebrisDraw.h"
namespace downwell {
// fxTimeShard Draw: !dflash uses GameMaker truth (positive is true).
// Sprite 91 is also used by debris, including the same bitmap frame decoder.
struct TimeShardDraw {
    using Command=EffectSpriteDraw::Command;
    struct State {
        RunnerImageValue x{},y{},imageIndex{};
        int dflash=-1;bool destroyRequested=false;
    };
    template<class Effect> static bool prepare(const Effect& effect,const EffectSurfacePlacement::View& view,
        bool death,bool lowSpec,bool surfaceExists,Command& out){
        return EffectSpriteDraw::prepare(!effect.destroyRequested&&effect.dflash<=0,91,effect.imageIndex,
            effect.x,effect.y,{0x3f800000u},view,death,lowSpec,surfaceExists,out);
    }
    template<class Effect,class Emit> static bool surface(const Effect& effect,const EffectMaskAssets& assets,
        const EffectSurfacePlacement::View& view,Emit emit){
        Command command;unsigned frame;EffectMaskAssets::Asset asset;
        std::int32_t px,py;bool shown;
        if(EffectSpriteDraw::pixelFx(effect.x,effect.y,view,px,py,shown)){
            if(effect.destroyRequested||effect.dflash>0||!shown)return true;
            return DebrisDraw::frame(91,effect.imageIndex,frame)&&assets.debris(91,frame,asset)&&emit(asset,px,py);
        }
        if(!prepare(effect,view,false,false,true,command))return false;
        if(command.placement.target==EffectSurfacePlacement::hidden)return true;
        return DebrisDraw::frame(91,command.imageIndex,frame)&&assets.debris(91,frame,asset)&&
            EffectSpriteDraw::emitAt(emit,asset,effect.x,effect.y);
    }
};
}
#endif
