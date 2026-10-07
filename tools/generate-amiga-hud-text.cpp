// Host-only derivation from the retained renderer and its source-ordered layers.
#include "platform/amiga/AmigaHudText.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <cassert>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
struct AmigaHudTextTables {
    static void word(UWORD v){std::printf("%u,",unsigned(v));}
    static void background(const char* name,const AmigaHudStage& s){
        std::printf("inline constexpr std::uint16_t %s[3][%u][%u]={\n",name,s.rows,s.words);
        for(unsigned p=0;p<3;++p){
            std::printf("{");
            for(unsigned y=0;y<s.rows;++y){
                std::printf("{");
                for(unsigned x=0;x<s.words;++x)word(UWORD((p==0?s.low[y][x]:p==1?s.high[y][x]:s.mask[y][x])&s.mask[y][x]));
                std::printf("},");
            }
            std::printf("},\n");
        }
        std::printf("};\n");
    }
    static void emit(AmigaHudText& t){
        std::printf("#pragma once\n#include <cstdint>\nnamespace downwellHudPrepared {\n");
        background("currencyBackground",t.currencyBackground_);
        background("streakEmpty",t.streakEmpty_);
        background("streakBackground",t.streakBackground_);
        std::printf("inline constexpr unsigned char streakIconRows[6][2]={");
        for(unsigned x=0;x<6;++x){
            unsigned first=24,end=0;
            for(unsigned y=0;y<24;++y)
                if(t.streakEmpty_.low[y][x]!=t.streakBackground_.low[y][x] ||
                   t.streakEmpty_.high[y][x]!=t.streakBackground_.high[y][x] ||
                   t.streakEmpty_.mask[y][x]!=t.streakBackground_.mask[y][x]){
                    if(first==24)first=y;end=y+1;
                }
            std::printf("{%u,%u},",first,end);
        }
        std::printf("};\n");
        std::printf("inline constexpr std::uint16_t currency[10][2][3][10][2]={\n");
        for(unsigned digit=0;digit<10;++digit){
            std::printf("{");
            for(unsigned phase=0;phase<2;++phase){
                assert(t.compose(digit,phase));
                const auto& c=t.currencyDigits_[digit][phase];
                if(phase==1)for(unsigned y=0;y<10;++y)
                    assert(!(c.low[y][1]|c.high[y][1]|c.mask[y][1]));
                std::printf("{");
                for(unsigned p=0;p<3;++p){
                    std::printf("{");
                    for(unsigned y=0;y<10;++y){
                        std::printf("{");
                        for(unsigned x=0;x<2;++x)word(p==0?c.low[y][x]:p==1?c.high[y][x]:c.mask[y][x]);
                        std::printf("},");
                    }
                    std::printf("},");
                }
                std::printf("},");
            }
            std::printf("},\n");
        }
        std::printf("};\n");
        // Streak layers combine by precedence white > black > red across
        // the entire string, not five independently composed layers per digit.
        assert(t.streakLeft_==-4 && t.streakTop_==0 && t.streakHeight_==10);
        std::printf("inline constexpr std::uint16_t streak[10][4][3][12][2]={\n");
        for(unsigned digit=0;digit<10;++digit){
            std::printf("{");
            for(unsigned phase=0;phase<4;++phase){
                UWORD pixels[3][12][2]{};
                auto set=[&](unsigned p,unsigned x,unsigned y){
                    assert(x<32&&y<12);pixels[p][y][x/16]|=UWORD(0x8000u>>(x&15));
                };
                const unsigned shift=2+phase*4;
                for(unsigned y=0;y<10;++y)for(unsigned x=0;x<16;++x)
                    if(t.streakCoverage_[digit][y]&(0x8000u>>x)){
                        assert(x<8);
                        set(0,shift+x+1,y+1);
                        set(1,shift+x+2,y+1);set(1,shift+x+1,y+2);
                        set(2,shift+x,y+1);set(2,shift+x+1,y);
                    }
                if(phase<2)for(unsigned p=0;p<3;++p)for(unsigned y=0;y<12;++y)
                    assert(!pixels[p][y][1]);
                std::printf("{");
                for(unsigned p=0;p<3;++p){
                    std::printf("{");
                    for(unsigned y=0;y<12;++y){
                        std::printf("{");for(unsigned x=0;x<2;++x)word(pixels[p][y][x]);std::printf("},");
                    }
                    std::printf("},");
                }
                std::printf("},");
            }
            std::printf("},\n");
        }
        std::printf("};\n}\n");
    }
};
static std::vector<UBYTE> load(const char* path){
    std::ifstream file(path,std::ios::binary);
    std::vector<UBYTE> result((std::istreambuf_iterator<char>(file)),{});
    assert(!result.empty());return result;
}
int main(int argc,char** argv){
    assert(argc==3);auto hud=load(argv[1]),game=load(argv[2]);
    AmigaHudText text{};
    assert(text.bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
    AmigaHudTextTables::emit(text);
    std::printf("namespace downwellHudPrepared { inline constexpr unsigned hudBytes=%zu, gameBytes=%zu, gameChipBytes=%u; }\n",hud.size(),game.size(),unsigned(amigaRendererChipPackBytes(game.data(),game.data()+game.size())));
}
