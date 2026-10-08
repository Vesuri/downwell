// Host-only derivation from the retained renderer and its source-ordered layers.
#include "platform/amiga/AmigaHudText.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <cassert>
#include <array>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
struct AmigaHudTextTables {
    template<unsigned Rows,unsigned Phases,class Compose>
    static void pairs(const char* name,Compose compose){
        // A word spans three digits, but each of its bytes spans only two.
        // Certify this decomposition against every complete source composite.
        for(unsigned phase=0;phase<Phases;++phase)
        for(unsigned a=0;a<11;++a)for(unsigned b=0;b<11;++b)for(unsigned c=0;c<11;++c){
            const auto whole=compose(phase,a,b,c),left=compose(phase,a,b,10),right=compose(phase,10,b,c);
            for(unsigned i=0;i<Rows*3;++i)
                assert(whole[i]==((left[i]&0xff00u)|(right[i]&0x00ffu)));
        }
        std::printf("inline constexpr std::uint16_t %s[%u][2][11][11][%u][3]={\n",name,Phases,Rows);
        for(unsigned phase=0;phase<Phases;++phase){
            std::printf("{");
            for(unsigned half=0;half<2;++half){
                std::printf("{");
                for(unsigned a=0;a<11;++a){
                    std::printf("{");
                    for(unsigned b=0;b<11;++b){
                        const auto column=half?compose(phase,10,a,b):compose(phase,a,b,10);
                        std::printf("{");
                        for(unsigned y=0;y<Rows;++y){
                            std::printf("{");for(unsigned p=0;p<3;++p)word(column[y*3+p]&(half?0xffu:0xff00u));
                            std::printf("},");
                        }
                        std::printf("},\n");
                    }
                    std::printf("},\n");
                }
                std::printf("},\n");
            }
            std::printf("},\n");
        }
        std::printf("};\n");
    }
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
        UWORD currencyPixels[10][2][3][10][2]{};
        UWORD streakPixels[10][4][3][12][2]{};
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
                        for(unsigned x=0;x<2;++x){
                            const UWORD value=p==0?c.low[y][x]:p==1?c.high[y][x]:c.mask[y][x];
                            currencyPixels[digit][phase][p][y][x]=value;word(value);
                        }
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
                        std::printf("{");for(unsigned x=0;x<2;++x){
                            streakPixels[digit][phase][p][y][x]=pixels[p][y][x];word(pixels[p][y][x]);
                        }std::printf("},");
                    }
                    std::printf("},");
                }
                std::printf("},");
            }
            std::printf("},\n");
        }
        std::printf("};\n");
        pairs<10,1>("currencyPairs",[&](unsigned,unsigned a,unsigned b,unsigned c){
            const unsigned digits[]={a,b,c};std::array<UWORD,30> column{};
            for(unsigned y=0;y<10;++y)for(unsigned i=0;i<3;++i)if(digits[i]<10){
                const auto& s=currencyPixels[digits[i]][i==1?1:0];const unsigned x=i==0?1:0;
                const UWORD mask=s[2][y][x];
                column[y*3]=UWORD((column[y*3]&~mask)|s[0][y][x]);
                column[y*3+1]=UWORD((column[y*3+1]&~mask)|s[1][y][x]);column[y*3+2]|=mask;
            }
            return column;
        });
        pairs<12,2>("streakPairs",[&](unsigned phase,unsigned a,unsigned b,unsigned c){
            const unsigned digits[]={a,b,c};std::array<UWORD,36> column{};
            for(unsigned y=0;y<12;++y){
                UWORD white=0,black=0,red=0;
                for(unsigned i=0;i<3;++i)if(digits[i]<10){
                    const auto& s=streakPixels[digits[i]][phase+(i==1?0:2)];const unsigned x=i==0?1:0;
                    white|=s[0][y][x];black|=s[1][y][x];red|=s[2][y][x];
                }
                column[y*3]=white|black;column[y*3+1]=UWORD((black|red)&~white);column[y*3+2]=white|black|red;
            }
            return column;
        });
        std::printf("static_assert(sizeof(currencyPairs)+sizeof(streakPairs)==49368,\"HUD pair column footprint\");\n}\n");
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
