#ifndef DOWNWELL_CORE_LEVEL_NUMBER_TEXT_H
#define DOWNWELL_CORE_LEVEL_NUMBER_TEXT_H
#include "LevelNumber.h"
#include "FontPack.h"
#include "RunStage.h"
namespace downwell {
struct AreaTitle {
    const char* language;
    int area;
    const unsigned* characters;
    unsigned length;
};
// Text inputs for object 372. Generated language data holds Unicode character
// tokens (not UTF-8 bytes). Measurement is supplied by the current font owner.
template<unsigned Capacity> struct LevelNumberText {
    static_assert(Capacity>0,"title text requires capacity");
    unsigned characters[Capacity]{},length=0;
    bool good=true;
    template<class Measure> bool create(LevelNumber<Capacity>& state,const AreaTitle* titles,unsigned count,
        const char* language,int area,int level,bool hard,Measure measure){
        if(!good||!language||(!titles&&count))return good=false;
        length=0;const AreaTitle* title=nullptr;
        for(unsigned i=0;i<count;++i)if(titles[i].area==area&&sameLanguage(language,titles[i].language)){
            title=&titles[i];break;
        }
        if(title){
            if(!title->characters&&title->length)return good=false;
            for(unsigned i=0;i<title->length;++i)if(!append(title->characters[i]=='*'?'#':title->characters[i]))return false;
        }else if(!append(' '))return false; // langString's missing-key default.
        if(!append('-')||(hard&&!append('H')))return false;
        const bool negative=level<0;
        unsigned magnitude=negative?0u-static_cast<unsigned>(level):static_cast<unsigned>(level);
        unsigned digits[10],n=0;
        do{digits[n++]=magnitude%10;magnitude/=10;}while(magnitude);
        if(negative&&!append('-'))return false;
        while(n)if(!append('0'+digits[--n]))return false;
        Fixed16 width;
        // string_width receives hash-converted text, while string_char_at
        // receives the original title. Do not replace stored hashes by LF.
        auto measuredCharacter=[&](unsigned i,unsigned& c){
            if(i>=length)return false;c=characters[i]=='#'?'\n':characters[i];return true;
        };
        if(!measure(length,measuredCharacter,width))return good=false;
        return good=state.create(length,width,[&](unsigned i,unsigned& c){
            if(!i||i>length)return false;c=characters[i-1];return true;
        });
    }
private:
    static bool sameLanguage(const char* a,const char* b){
        if(!b)return false;
        while(*a&&*a==*b){++a;++b;}return *a==*b;
    }
    bool append(unsigned c){if(length==Capacity)return good=false;characters[length++]=c;return true;}
};
// Borrow the live language/font/settings state. Read it only after the child
// record has been published, inside synchronous Create, not before birth.
struct LevelNumberTextContext {
    const FontPack& fonts;
    const AreaTitle* titles;
    unsigned titleCount;
    const char* const& language;
    const unsigned& fontId;
    const bool& hardMode;
    template<unsigned Capacity> bool initialize(LevelNumber<Capacity>& state,const RunStage& stage)const{
        LevelNumberText<Capacity> text;
        return text.create(state,titles,titleCount,language,stage.area,stage.level,hardMode,
            [&](unsigned length,auto read,Fixed16& width){
                std::int32_t pixels;if(!fonts.stringWidth(fontId,length,read,pixels))return false;
                if(pixels<0||pixels>32767)return false;
                width=Fixed16::fromInteger(pixels);return true;
            });
    }
};
}
#endif
