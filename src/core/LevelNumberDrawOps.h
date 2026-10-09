#ifndef DOWNWELL_CORE_LEVEL_NUMBER_DRAW_OPS_H
#define DOWNWELL_CORE_LEVEL_NUMBER_DRAW_OPS_H
#include "MotionBinary64.h"
namespace downwell {
// A borrowed, synchronous bridge between a type-erased room registration and
// the renderer's concrete surface emitter. Never retain it after Draw returns.
class LevelNumberDrawOps {
public:
    template<class Ops> explicit LevelNumberDrawOps(Ops& ops):context_(&ops),
        exists_([](void* p,bool& value){return static_cast<Ops*>(p)->exists(value);}),
        begin_([](void* p){return static_cast<Ops*>(p)->begin();}),
        end_([](void* p){return static_cast<Ops*>(p)->end();}),
        color_([](void* p,unsigned value){return static_cast<Ops*>(p)->color(value);}),
        view_([](void* p,unsigned axis,MotionBinary64& value){return static_cast<Ops*>(p)->view(axis,value);}),
        character_([](void* p,unsigned token,MotionBinary64 x,MotionBinary64 y){return static_cast<Ops*>(p)->character(token,x,y);}){}
    bool exists(bool& value){return exists_(context_,value);}
    bool begin(){return begin_(context_);}bool end(){return end_(context_);}
    bool color(unsigned value){return color_(context_,value);}
    bool view(unsigned axis,MotionBinary64& value){return view_(context_,axis,value);}
    bool character(unsigned token,MotionBinary64 x,MotionBinary64 y){return character_(context_,token,x,y);}
private:
    void* context_;
    bool (*exists_)(void*,bool&);bool (*begin_)(void*);bool (*end_)(void*);
    bool (*color_)(void*,unsigned);bool (*view_)(void*,unsigned,MotionBinary64&);
    bool (*character_)(void*,unsigned,MotionBinary64,MotionBinary64);
};
}
#endif
