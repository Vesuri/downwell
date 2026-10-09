#ifndef DOWNWELL_CORE_LEVEL_NUMBER_H
#define DOWNWELL_CORE_LEVEL_NUMBER_H
#include "AlarmBank.h"
#include "DivideBinary64.h"
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
    MotionBinary64 width{},textX{},textY{};
    AlarmBank alarms;
    bool good=true,destroyRequested=false;
    template<class Read> bool create(unsigned count,MotionBinary64 measuredWidth,Read read){
        if(!good||count>Capacity||!measuredWidth.valid())return good=false;
        length=count;width=measuredWidth;shown=1;exit=0;i=1;destroyRequested=false;
        for(;i<=length;++i){
            unsigned token;
            if(!read(i,token))return good=false;
            characters[i]={token,0,-2};
        }
        appearInterval=exitInterval=4;remainTime=120;
        alarms.reset();alarms.set(0,appearInterval);return true;
    }
    template<class Destroy> bool advanceAlarm(unsigned index,Destroy destroy){
        if(!good||index>=AlarmBank::count)return good=false;
        if(destroyRequested||index>2||!alarms.advanceOne(index))return true;
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
        MotionBinary64 half,view;std::int32_t centered;
        if(!ops.view(0,textX)||!textX.add(MotionBinary64::fromInteger(80))||
           !divideBinary64(width,MotionBinary64::fromInteger(2),half)||!half.roundPixel(centered)||
           !subtract(textX,MotionBinary64::fromInteger(centered))||!ops.view(0,view)||!subtract(textX,view)||
           !ops.view(1,textY)||!textY.add(MotionBinary64::fromInteger(84))||
           !ops.view(1,view)||!subtract(textY,view))return good=false;
        for(i=1;i<=length;++i){
            auto& c=characters[i];
            if(exit&&i<=exit&&c.offset<=4)++c.offset;
            if((!exit&&i<=shown)||(exit&&c.offset<=3)){
                auto x=textX,y=textY;
                if(!x.add(MotionBinary64::fromInteger(int((i-1)*8)))||
                   !y.add(MotionBinary64::fromInteger(c.offset))||!ops.character(c.token,x,y))return good=false;
                if(!exit&&c.offset<=0)++c.offset;
            }
        }
        return good=ops.end()&&good;
    }
private:
    static bool subtract(MotionBinary64& value,MotionBinary64 other){
        if(!other.zero())other.high^=0x80000000u;
        return value.add(other);
    }
};
}
#endif
