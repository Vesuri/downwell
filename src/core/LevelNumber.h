#ifndef DOWNWELL_CORE_LEVEL_NUMBER_H
#define DOWNWELL_CORE_LEVEL_NUMBER_H
#include "AlarmBank.h"
#include "FixedPoint.h"
namespace downwell {
// Object 372's event state. The text owner supplies original string_char_at
// tokens and string_width(string_hash_to_newline(levelTxt)); do not substitute
// fixed-width measurement or store copied game text here.
template<unsigned Capacity> struct LevelNumber {
    static_assert(Capacity<=0x0fffffffu,"text spacing must fit signed coordinates");
    struct Character {unsigned token=0;int enabled=0,offset=-2;};
    Character characters[Capacity+1]{}; // Source arrays are one-based.
    unsigned length=0,shown=1,exit=0,i=1;
    int appearInterval=4,exitInterval=4,remainTime=120;
    Fixed16 width{},textX{},textY{};
    // scrDrawBorderTextBlack writes these variables on its calling instance.
    Fixed16 sx{},sy{};unsigned txt=0;
    AlarmBank alarms;
    bool good=true,destroyRequested=false,created=false;
    template<class Read> bool create(unsigned count,Fixed16 measuredWidth,Read read){
        if(!good||count>Capacity)return good=false;
        length=count;width=measuredWidth;shown=1;exit=0;i=1;destroyRequested=false;
        for(;i<=length;++i){
            unsigned token;
            if(!read(i,token))return good=false;
            characters[i]={token,0,-2};
        }
        appearInterval=exitInterval=4;remainTime=120;
        alarms.reset();alarms.set(0,appearInterval);created=true;return true;
    }
    template<class Destroy> bool advanceAlarm(unsigned index,Destroy destroy){
        if(!good||index>=AlarmBank::count)return good=false;
        if(destroyRequested||index>2||!alarms.advanceOne(index))return true;
        return fireAlarm(index,destroy);
    }
    // Stored runners have already decremented the registered alarm slot.
    template<class Destroy> bool fireAlarm(unsigned index,Destroy destroy){
        if(!good||index>2||destroyRequested)return good=false;
        if(index==0){
            if(shown<length){characters[shown].enabled=1;++shown;alarms.set(0,appearInterval);}
            else alarms.set(1,remainTime);
        }else if(index==1){exit=1;alarms.set(2,exitInterval);}
        else if(exit<length){
            // The VM uses txtShown here, NOT txtExit.
            if(shown>Capacity)return good=false;
            characters[shown].enabled=1;++exit;alarms.set(2,exitInterval);
        }else{
            if(!destroy())return good=false;
            destroyRequested=true;
        }
        return true;
    }
    template<class Ops> bool draw(Ops& ops){
        if(!good)return false;
        if(destroyRequested)return true;
        bool exists=false;
        if(!ops.exists(exists))return good=false;
        if(!exists)return true; // Draw owns the offsets, not a simulation Step.
        if(!ops.begin())return good=false;
        // view + 80 - round(width / 2) - view, and view + 84 - view.
        Fixed16 view;const std::int32_t centered=Fixed16::roundQ16(width.v/2);
        if(!ops.view(0,textX))return good=false;
        textX=textX+Fixed16::fromInteger(80-centered);
        if(!ops.view(0,view))return good=false;
        textX=textX-view;
        if(!ops.view(1,textY))return good=false;
        textY=textY+Fixed16::fromInteger(84);
        if(!ops.view(1,view))return good=false;
        textY=textY-view;
        for(i=1;i<=length;++i){
            auto& c=characters[i];
            if(exit&&i<=exit&&c.offset<=4)++c.offset;
            if((!exit&&i<=shown)||(exit&&c.offset<=3)){
                const auto x=textX+Fixed16::fromInteger(int((i-1)*8)),y=textY+Fixed16::fromInteger(c.offset);
                if(!borderedCharacter(ops,c.token,x,y))return good=false;
                if(!exit&&c.offset<=0)++c.offset;
            }
        }
        return good=ops.end()&&good;
    }
private:
    template<class Ops> bool borderedCharacter(Ops& ops,unsigned token,Fixed16 x,Fixed16 y){
        sx=x;sy=y;txt=token;
        if(!ops.color(0))return false;
        // Preserve the VM's four cardinal draws and final white draw. The
        // backend applies string_hash_to_newline to each character token.
        const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
        for(unsigned n=0;n<4;++n){
            if(!ops.character(txt,sx+Fixed16::fromInteger(dx[n]),sy+Fixed16::fromInteger(dy[n])))return false;
        }
        return ops.color(0xffffffu)&&ops.character(txt,sx,sy);
    }
};
}
#endif
