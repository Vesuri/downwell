#pragma once
#include "AmigaHudText.h"
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/amiga-hud-text.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/amiga-hud-text.h"
#endif

// Word dependencies are digit identities/positions, not a scan of retained
// pixels. The HUD owner must invalidate after another writer touches a region.
// Overlapping glyph layers are composed offline into disjoint byte columns.
// The generator certifies their union for every digit/blank triple and phase;
// runtime composition is retained only as a reference selector.
class AmigaPreparedHudText {
    struct Slot { unsigned char digit=0,phase=0,word=0; };
    struct State {
        UBYTE* target=nullptr;
        Slot slots[10]{};
        unsigned count=0;
        bool active=false;
    };
    State currency_,streak_;
    const UBYTE* hud_=nullptr;
    const UBYTE* game_=nullptr;
    bool valid_=false;
    static bool header(const UBYTE* pack,const UBYTE* end){
        static constexpr UBYTE source[]={0xc0,0x7d,0xaf,0x5f,0xbe,0x94,0x0a,0xe0};
        if(!pack||!end||end-pack<20||pack[0]!='D'||pack[1]!='W'||pack[2]!='A'||pack[3]!='P'||
           pack[4]!=0||pack[5]!=2||pack[6]!=0||pack[7]!=20)return false;
        for(unsigned i=0;i<8;++i)if(pack[12+i]!=source[i])return false;
        return true;
    }
    static unsigned affected(const State& old,const Slot* next,unsigned count){
        unsigned mask=0;
        const unsigned n=count>old.count?count:old.count;
        for(unsigned i=0;i<n;++i){
            if(i<count&&i<old.count&&next[i].digit==old.slots[i].digit&&
               next[i].phase==old.slots[i].phase&&next[i].word==old.slots[i].word)continue;
            if(i<count)mask|=3u<<next[i].word;
            if(i<old.count)mask|=3u<<old.slots[i].word;
        }
        return mask;
    }
    static void retain(State& state,UBYTE* target,const Slot* slots,unsigned count,bool active){
        state.target=target;state.count=count;state.active=active;
        for(unsigned i=0;i<count;++i)state.slots[i]=slots[i];
    }
    static UWORD read(const UBYTE* p){
#ifdef DOWNWELL_PLATFORM_AMIGA
        return *reinterpret_cast<const UWORD*>(p);
#else
        return UWORD((p[0]<<8)|p[1]);
#endif
    }
    static void write(UBYTE* p,UWORD value){
#ifdef DOWNWELL_PLATFORM_AMIGA
        *reinterpret_cast<UWORD*>(p)=value;
#else
        p[0]=UBYTE(value>>8);p[1]=UBYTE(value);
#endif
    }
    static void publish(UBYTE* p,UWORD low,UWORD high,UWORD mask){
        if(!mask)return;
        write(p,UWORD((read(p)&~mask)|(low&mask)));
        p+=downwellPlaneBytes;
        write(p,UWORD((read(p)&~mask)|(high&mask)));
    }
public:
    void release(){hud_=game_=nullptr;valid_=false;invalidate();}
    void invalidate(){currency_={};streak_={};}
    void invalidateStreak(){streak_={};}
    bool bind(const UBYTE* hud,const UBYTE* hudEnd,const UBYTE* game,const UBYTE* gameEnd){
        if(hud==hud_&&game==game_)return valid_;
        hud_=hud;game_=game;invalidate();
        return valid_=header(hud,hudEnd)&&header(game,gameEnd)&&
            unsigned(hudEnd-hud)==downwellHudPrepared::hudBytes&&
            (unsigned(gameEnd-game)==downwellHudPrepared::gameBytes||
             unsigned(gameEnd-game)==downwellHudPrepared::gameChipBytes);
    }
    bool currency(AmigaRenderer* renderer,unsigned value){
        if(!valid_||!renderer||!renderer->hud)return false;
        amigaRendererClearClipRect(renderer);
        Slot slots[5];unsigned divisor=1,count=1;
        while(value/divisor>=10&&count<5){divisor*=10;++count;}
        for(unsigned i=0;i<count;++i){
            const unsigned position=count-i-1;
            slots[i]={static_cast<unsigned char>((value/divisor)%10),
                      static_cast<unsigned char>(position&1),
                      static_cast<unsigned char>((329-int(position)*8-288)>>4)};
            divisor/=10;
        }
        const bool first=currency_.target!=renderer->hud;
        const unsigned dirty=first?31:affected(currency_,slots,count);
        for(unsigned w=0;w<5;++w){
            if(!(dirty&(1u<<w)))continue;
#ifdef DOWNWELL_REFERENCE_HUD_COMPOSITION
            const UWORD* parts[3][3]{};unsigned n=0;
            for(unsigned i=0;i<count;++i)if(w>=slots[i].word&&w<slots[i].word+2u){
                if(w!=slots[i].word&&slots[i].phase==1)continue;
                if(n==3)return false;
                for(unsigned p=0;p<3;++p)parts[n][p]=
                    &downwellHudPrepared::currency[slots[i].digit][slots[i].phase][p][0][w-slots[i].word];
                ++n;
            }
#else
            unsigned digits[3]={10,10,10};
            for(unsigned i=0;i<count;++i){
                const auto& s=slots[i];
                if(w==s.word)digits[s.phase?1:2]=s.digit;
                else if(w==s.word+1u&&!s.phase)digits[0]=s.digit;
            }
            const auto& left=downwellHudPrepared::currencyPairs[0][0][digits[0]][digits[1]];
            const auto& right=downwellHudPrepared::currencyPairs[0][1][digits[1]][digits[2]];
#endif
            for(unsigned y=first?0:8;y<(first?30:18);++y){
                UWORD low=downwellHudPrepared::currencyBackground[0][y][w];
                UWORD high=downwellHudPrepared::currencyBackground[1][y][w];
                UWORD mask=downwellHudPrepared::currencyBackground[2][y][w];
                if(y>=8&&y<18){
#ifdef DOWNWELL_REFERENCE_HUD_COMPOSITION
                  for(unsigned i=0;i<n;++i){
                    const unsigned row=(y-8)*2;
                    const UWORD top=parts[i][2][row];
                    low=UWORD((low&~top)|parts[i][0][row]);
                    high=UWORD((high&~top)|parts[i][1][row]);mask|=top;
                  }
#else
                    const auto* a=left[y-8];const auto* b=right[y-8];
                    const UWORD top=a[2]|b[2];
                    low=UWORD((low&~top)|a[0]|b[0]);high=UWORD((high&~top)|a[1]|b[1]);mask|=top;
#endif
                }
                publish(renderer->hud+(y+4)*downwellBytesPerPlaneLine+36+w*2,low,high,mask);
            }
        }
        retain(currency_,renderer->hud,slots,count,true);return true;
    }
    bool streak(AmigaRenderer* renderer,unsigned value,int yOffset=0){
        // Position is owned by the scene; moving it requires restoring the old
        // region and invalidating this retained word cache before publication.
        if(!valid_||!renderer||!renderer->hud||yOffset < -44||yOffset > 216)return false;
        amigaRendererClearClipRect(renderer);
        unsigned char digits[10];unsigned count=0;
        for(unsigned v=value;v;v/=10)digits[count++]=static_cast<unsigned char>(v%10);
        Slot slots[10];
        for(unsigned i=0;i<count;++i){
            const int left=42-int(count-1)*4+int(i)*8;
            if(left<0 || ((left&15)&3)!=2)return false;
            slots[i]={digits[count-i-1],static_cast<unsigned char>(((left&15)-2)/4),
                      static_cast<unsigned char>(left>>4)};
        }
        const bool active=value!=0,first=streak_.target!=renderer->hud;
        const unsigned dirty=first?63:affected(streak_,slots,count);
        const bool icon=first||active!=streak_.active;
        const auto& background=active?downwellHudPrepared::streakBackground:downwellHudPrepared::streakEmpty;
        for(unsigned w=0;w<6;++w){
            unsigned begin=24,end=0;
            if(first){begin=0;end=24;}
            else{
                if(dirty&(1u<<w)){begin=11;end=23;}
                if(icon){
                    const auto* rows=downwellHudPrepared::streakIconRows[w];
                    if(rows[0]<rows[1]){
                        if(rows[0]<begin)begin=rows[0];if(rows[1]>end)end=rows[1];
                    }
                }
            }
            if(begin>=end)continue;
#ifdef DOWNWELL_REFERENCE_HUD_COMPOSITION
            const UWORD* parts[3][3]{};unsigned n=0;
            for(unsigned i=0;i<count;++i)if(w>=slots[i].word&&w<slots[i].word+2u){
                if(w!=slots[i].word&&slots[i].phase<2)continue;
                if(n==3)return false;
                for(unsigned p=0;p<3;++p)parts[n][p]=
                    &downwellHudPrepared::streak[slots[i].digit][slots[i].phase][p][0][w-slots[i].word];
                ++n;
            }
#else
            unsigned digits[3]={10,10,10};
            const unsigned phase=count?(slots[0].phase&1):0;
            for(unsigned i=0;i<count;++i){
                const auto& s=slots[i];
                if(w==s.word)digits[s.phase<2?1:2]=s.digit;
                else if(w==s.word+1u&&s.phase>=2)digits[0]=s.digit;
            }
            const auto& left=downwellHudPrepared::streakPairs[phase][0][digits[0]][digits[1]];
            const auto& right=downwellHudPrepared::streakPairs[phase][1][digits[1]][digits[2]];
#endif
            for(unsigned y=begin;y<end;++y){
                UWORD low=background[0][y][w],high=background[1][y][w],mask=background[2][y][w];
                if(y>=11&&y<23){
#ifdef DOWNWELL_REFERENCE_HUD_COMPOSITION
                    UWORD white=0,black=0,red=0;const unsigned row=(y-11)*2;
                    for(unsigned i=0;i<n;++i){white|=parts[i][0][row];black|=parts[i][1][row];red|=parts[i][2][row];}
                    const UWORD top=UWORD(white|black|red);
                    low=UWORD((low&~top)|white|black);
                    high=UWORD((high&~top)|((black|red)&~white));mask|=top;
#else
                    const auto* a=left[y-11];const auto* b=right[y-11];
                    const UWORD top=a[2]|b[2];
                    low=UWORD((low&~top)|a[0]|b[0]);high=UWORD((high&~top)|a[1]|b[1]);mask|=top;
#endif
                }
                publish(renderer->hud+(int(y)+44+yOffset)*downwellBytesPerPlaneLine+18+w*2,low,high,mask);
            }
        }
        retain(streak_,renderer->hud,slots,count,active);return true;
    }
};
