#include "core/LevelNumber.h"
#include "core/LevelNumberInstances.h"
#include "core/FontPack.h"
#include "../../generated/downwell-area-titles.h"
#include "../../generated/downwell-title-glyphs.h"
#include "core/PaletteEffectSurface.h"
#include "core/LevelNumberSurfaceDraw.h"
#include "core/CurrencySmokeComposition.h"
#include "core/SmokeEffects.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <iterator>
#include <memory>
using namespace downwell;
static void titleSurface(){
    struct Target {
        bool present=true;unsigned begins=0,ends=0,views=0;
        bool exists(bool& out){out=present;return true;}
        bool begin(){++begins;return true;}bool end(){++ends;return true;}
        bool view(unsigned axis,MotionBinary64& out){++views;out=MotionBinary64::fromInteger(axis?30000:1000000);return true;}
    };
    SmokeEffects<1> smoke;EffectMaskAssets assets;
    auto composition=std::make_unique<CurrencySmokeComposition>();
    const EffectSurfacePlacement::View view{MotionBinary64::fromInteger(1000000),
        MotionBinary64::fromInteger(30000),MotionBinary64::fromInteger(160),MotionBinary64::fromInteger(284)};
    const unsigned tokens[]={'A','#','B'};
    std::vector<unsigned char> expected(160*284);
    for(unsigned letter:{0u,2u})for(unsigned pass=0;pass<5;++pass){
        EffectMaskAssets::Asset asset;bool ink;
        assert(title_glyphs::bank.resolve(2,tokens[letter],pass==4?0xffffff:0,asset,ink)&&ink);
        const int dx[]={1,-1,0,0,0},dy[]={0,0,1,-1,0};
        const int x=68+int(letter)*8+dx[pass]-asset.originX,y=82+dy[pass];
        for(unsigned sy=0;sy<asset.mask.height;++sy)for(unsigned sx=0;sx<asset.mask.width;++sx)
            if(asset.mask.data[sy*asset.mask.stride+sx/8]&(0x80>>(sx%8)))
                expected[(y+sy)*160+x+sx]=pass==4?1:3;
    }
    for(unsigned mode=0;mode<4;++mode){
        Target target;target.present=mode!=3;
        LevelNumber<3> title;assert(title.create(3,MotionBinary64::fromInteger(24),
            [&](unsigned i,unsigned& out){out=tokens[i-1];return true;}));title.shown=3;
        LevelNumberTextDrawState state{2,0,0,123,MotionBinary64::fromInteger(1)};
        CurrencySmokeComposition::Settings settings;settings.death=mode==1;settings.lowSpec=mode==2;
        assert(composition->prepareWithSurfaceProducers([](auto){},smoke,assets,view,settings,
            [](auto){return true;},[&](auto emit){
                LevelNumberSurfaceDraw<Target,decltype(emit)> draw(title_glyphs::bank,state,target,emit);
                return title.draw(draw);
            },[](auto){},[](MotionBinary64 value,std::int32_t& out){return value.roundPixel(out);}));
        std::vector<unsigned char> actual(160*284);
        composition->surface().visit([&](unsigned x,unsigned y,std::uint16_t alpha,std::uint16_t lo,std::uint16_t hi){
            for(unsigned b=0;b<16;++b)if(alpha&(0x8000>>b))actual[y*160+x+b]=((lo>>(15-b))&1)|(((hi>>(15-b))&1)<<1);
        });
        assert(actual==(mode==3?std::vector<unsigned char>(160*284):expected));
        assert(title.characters[1].offset==(mode==3?-2:-1));
        assert(state.color==(mode==3?123u:0xffffffu));
        assert(target.begins==(mode==3?0u:1u)&&target.ends==target.begins&&target.views==target.begins*4);
    }
    // Current renderer state is borrowed, not silently reset to supported
    // defaults. A rejected draw retains the source's partial mutation prefix.
    for(unsigned mode=0;mode<4;++mode){
        Target target;LevelNumber<1> title;
        assert(title.create(1,MotionBinary64::fromInteger(8),[](unsigned,unsigned& c){c='A';return true;}));
        LevelNumberTextDrawState state{mode==0?0u:2u,mode==1?1u:0u,mode==2?1u:0u,123,
            MotionBinary64::fromInteger(mode==3?0:1)};
        assert(!composition->prepareWithSurfaceProducers([](auto){},smoke,assets,view,{},
            [](auto){return true;},[&](auto emit){
                LevelNumberSurfaceDraw<Target,decltype(emit)> draw(title_glyphs::bank,state,target,emit);
                return title.draw(draw);
            },[](auto){},[](MotionBinary64 value,std::int32_t& out){return value.roundPixel(out);}));
        assert(!composition->visible()&&target.begins==1&&!target.ends&&state.color==0&&title.characters[1].offset==-2);
    }
}
static void glyphMasks(){
    std::ifstream file("generated/downwell-fonts.dwfn",std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
    FontPack fonts(bytes.data(),bytes.size());PackedFont font;assert(fonts.font(2,font));
    assert(font.glyphCount==sizeof(title_glyphs::glyphs)/sizeof(title_glyphs::glyphs[0]));
    for(unsigned i=0;i<font.glyphCount;++i){
        PackedGlyph glyph;assert(fonts.glyph(font.firstGlyph+i,glyph));
        EffectMaskAssets::Asset black,white;bool ink=false;
        assert(title_glyphs::bank.resolve(2,glyph.character,0,black,ink));
        assert(ink==(glyph.sourceWidth>0&&glyph.sourceHeight>0));
        if(!ink)continue;
        assert(title_glyphs::bank.resolve(2,glyph.character,0xffffff,white,ink)&&ink);
        assert(black.mask.width==unsigned(glyph.sourceWidth)&&black.mask.height==unsigned(glyph.sourceHeight));
        assert(black.originX==-glyph.offset&&black.originY==0);
        assert(black.mask.data==white.mask.data&&black.low==black.mask.data&&black.high==black.mask.data);
        assert(white.low==white.mask.data&&white.high==title_glyphs::zero);
        for(unsigned b=0;b<white.mask.bytes;++b)assert(!white.high[b]);
    }
    EffectMaskAssets::Asset asset;bool ink;
    assert(!title_glyphs::bank.resolve(0,'A',0xffffff,asset,ink));
    assert(!title_glyphs::bank.resolve(2,'A',0xff0000,asset,ink));
    PackedGlyph fallback;const bool hasFallback=fonts.findGlyph(font,0x25af,fallback);
    assert(title_glyphs::bank.resolve(2,0x10ffff,0xffffff,asset,ink));
    assert(ink==(hasFallback&&fallback.sourceWidth>0&&fallback.sourceHeight>0));
    // Paint a black border then white glyph at a non-word-aligned origin.
    // Compare coverage/colors against a separate per-pixel reference, including
    // opaque black ink (which must never erase alpha).
    PaletteEffectSurface<64,32> surface;
    unsigned char pixels[64*32]={};
    const int dx[]={1,-1,0,0,0},dy[]={0,0,1,-1,0};
    for(unsigned pass=0;pass<5;++pass){
        assert(title_glyphs::bank.resolve(2,'A',pass==4?0xffffff:0,asset,ink)&&ink);
        const int x=15+dx[pass]-asset.originX,y=7+dy[pass];
        PaletteEffectSurface<64,32>::Asset stamp{
            {asset.mask.data,asset.mask.bytes,asset.mask.width,asset.mask.height,asset.mask.stride},asset.low,asset.high};
        assert(surface.add(stamp,x,y));
        for(unsigned sy=0;sy<asset.mask.height;++sy)for(unsigned sx=0;sx<asset.mask.width;++sx)
            if(asset.mask.data[sy*asset.mask.stride+sx/8]&(0x80>>(sx%8))){
                const int px=x+int(sx),py=y+int(sy);
                if(px>=0&&px<64&&py>=0&&py<32)pixels[py*64+px]=pass==4?1:3;
            }
    }
    unsigned char actual[64*32]={};
    surface.visit([&](unsigned x,unsigned y,std::uint16_t alpha,std::uint16_t low,std::uint16_t high){
        for(unsigned b=0;b<16;++b)if(alpha&(0x8000>>b))
            actual[y*64+x+b]=((low>>(15-b))&1)|(((high>>(15-b))&1)<<1);
    });
    for(unsigned i=0;i<64*32;++i)assert(actual[i]==pixels[i]);
}
struct Draw {
    bool surface=true;unsigned views=0,ends=0,failCharacter=0;
    std::vector<int> tokens,xs,ys;
    std::vector<unsigned> colors;unsigned failColor=0;
    bool color(unsigned value){colors.push_back(value);return colors.size()!=failColor;}
    bool exists(bool& result){result=surface;return true;}
    bool begin(){return true;}
    bool view(unsigned axis,MotionBinary64& value){
        assert(axis==unsigned(views>=2));
        const int positions[]={100,101,200,202};value=MotionBinary64::fromInteger(positions[views++]);return true;
    }
    bool character(unsigned token,MotionBinary64 x,MotionBinary64 y){
        std::int32_t px,py;assert(x.roundPixel(px)&&y.roundPixel(py));
        tokens.push_back(token);xs.push_back(px);ys.push_back(py);return tokens.size()!=failCharacter;
    }
    bool end(){++ends;return true;}
};
using Numbers=LevelNumberInstances<2,8>;
struct Lifetime {
    unsigned births=0,marks=0,retired=0;bool rejectDestroy=false;
    bool birth(InstanceHandle,const Numbers::Record&){std::abort();}
    bool birthAt(InstanceHandle key,const Numbers::Record& r,RunnerImageValue x,RunnerImageValue y){
        assert(key.generation==r.lifetimeId&&r.sourceId>=1000&&!r.state.created&&
            x.bits==r.x.bits&&y.bits==r.y.bits);++births;return true;
    }
    bool activation(InstanceHandle,const Numbers::Record&,bool){return true;}
    bool membership(InstanceHandle,const Numbers::Record&,bool){std::abort();}
    bool markDestroyed(InstanceHandle,const Numbers::Record& r){
        assert(!r.destroyRequested&&!r.state.destroyRequested);++marks;return !rejectDestroy;
    }
    bool retire(InstanceHandle,const Numbers::Record&){++retired;return true;}
};
int main(){
    glyphMasks();
    titleSurface();
    std::ifstream fontFile("generated/downwell-fonts.dwfn",std::ios::binary);
    const std::vector<unsigned char> fontBytes{std::istreambuf_iterator<char>(fontFile),{}};
    FontPack fonts(fontBytes.data(),fontBytes.size());assert(fonts.valid());
    for(const auto& area:area_titles::entries)for(bool hard:{false,true}){
        LevelNumberText<64> text;LevelNumber<64> state;unsigned measures=0;
        assert(text.create(state,area_titles::entries,area_titles::count,area.language,area.area,3,hard,
            [&](unsigned count,auto at,MotionBinary64& width){
                ++measures;assert(count==area.length+(hard?3u:2u));
                for(unsigned i=0;i<area.length;++i){unsigned c;assert(at(i,c)&&c==area.characters[i]);}
                width=MotionBinary64::fromInteger(37);return true;
            }));
        assert(measures==1&&state.length==area.length+(hard?3u:2u)&&state.width.high==MotionBinary64::fromInteger(37).high);
        for(unsigned i=0;i<area.length;++i)assert(state.characters[i+1].token==area.characters[i]);
        assert(state.characters[area.length+1].token=='-'&&state.characters[state.length].token=='3');
        if(hard)assert(state.characters[area.length+2].token=='H');
        LevelNumberText<64> measuredText;LevelNumber<64> measured;
        assert(measuredText.create(measured,area_titles::entries,area_titles::count,area.language,area.area,3,hard,
            [&](unsigned count,auto at,MotionBinary64& width){
                std::int32_t pixels;if(!fonts.stringWidth(2,count,at,pixels))return false;
                width=MotionBinary64::fromInteger(pixels);return true;
            }));
        PackedFont font;assert(fonts.font(2,font));int expected=0;
        for(unsigned i=1;i<=measured.length;++i){
            PackedGlyph glyph;
            if(fonts.findGlyph(font,measured.characters[i].token,glyph)||fonts.findGlyph(font,0x25af,glyph))expected+=glyph.shift;
        }
        std::int32_t pixels;assert(measured.width.roundPixel(pixels)&&pixels==expected);
    }
    {
        const unsigned chars[]={'A','\r','\n','A','A','\n',0x10ffff};
        std::int32_t width=-1;PackedFont font;PackedGlyph a,fallback;assert(fonts.font(2,font)&&fonts.findGlyph(font,'A',a));
        const int missing=fonts.findGlyph(font,0x25af,fallback)?fallback.shift:0;
        assert(fonts.stringWidth(2,7,[&](unsigned i,unsigned& c){c=chars[i];return true;},width)&&width==(2*a.shift>missing?2*a.shift:missing));
        width=123;assert(!fonts.stringWidth(999,1,[](unsigned,unsigned&){std::abort();return false;},width)&&width==123);
    }
    {
        unsigned checked=0;
        for(unsigned f=0;f<fonts.fontCount();++f){
            PackedFont font;assert(fonts.font(f,font));
            for(unsigned i=0;i<font.glyphCount;++i){
                PackedGlyph left;assert(fonts.glyph(font.firstGlyph+i,left));
                for(unsigned k=0;k<left.kerningCount;++k){
                    PackedKerning kern;PackedGlyph right;
                    assert(fonts.kerning(left.firstKerning+k,kern));
                    if(!kern.shiftModifier||!fonts.findGlyph(font,kern.character,right))continue;
                    std::int32_t width;assert(fonts.stringWidth(f,2,[&](unsigned index,unsigned& c){
                        c=index?right.character:left.character;return true;
                    },width)&&width==left.shift+right.shift);++checked;
                }
            }
        }
        assert(checked>0);
    }
    {
        const unsigned tokens[]={0x0410,'*',0x30a2};const AreaTitle area{"test",1,tokens,3};
        LevelNumberText<32> text;LevelNumber<32> state;
        assert(text.create(state,&area,1,"test",1,-2147483647-1,true,
            [](unsigned count,auto at,MotionBinary64& width){
                unsigned c;assert(count==16&&at(1,c)&&c=='\n');width={};return true;
            }));
        assert(state.characters[1].token==0x0410&&state.characters[2].token=='#'&&state.characters[3].token==0x30a2&&
            state.characters[6].token=='-'&&state.characters[16].token=='8');
        LevelNumberText<8> missing;LevelNumber<8> fallback;
        assert(missing.create(fallback,nullptr,0,"missing",9,0,false,
            [](unsigned count,auto,MotionBinary64& width){assert(count==3);width={};return true;})&&fallback.characters[1].token==' ');
        LevelNumberText<2> shortText;LevelNumber<2> shortState;
        assert(!shortText.create(shortState,&area,1,"test",1,1,false,[](auto...){std::abort();return false;})&&!shortState.created);
        LevelNumberText<32> bad;LevelNumber<32> badState;
        assert(!bad.create(badState,&area,1,"test",1,1,false,[](auto...){return false;})&&!badState.created);
    }
    auto read=[](unsigned i,unsigned& token){token=100+i;return true;};
    for(bool rejectDestroy:{false,true}){
        Lifetime life;life.rejectDestroy=rejectDestroy;
        Numbers native(Numbers::Binding::bindPositionedDestruction(life));
        InstanceHandle key;
        assert(native.create(1000,MotionBinary64::fromInteger(241),MotionBinary64::fromInteger(603),
            [&](auto h,auto& state){assert(native.find(h)&&!state.created);return state.create(3,MotionBinary64::fromInteger(5),read);},key));
        assert(native.size()==1&&life.births==1);
        assert(native.activate(key,false));
        Draw inactive;
        assert(native.draw(key,inactive)&&inactive.tokens.empty());
        for(unsigned slot=0;slot<12;++slot)assert(native.advanceAlarm(key,slot));
        assert(native.find(key)->state.alarms.get(0)==4&&native.activate(key,true));
        LevelNumber<8> oracle;assert(oracle.create(3,MotionBinary64::fromInteger(5),read));
        for(unsigned tick=1;tick<=142;++tick){
            for(unsigned alarm=0;alarm<12;++alarm){
                const bool okay=native.advanceAlarm(key,alarm);
                assert(oracle.advanceAlarm(alarm,[]{return true;}));
                if(tick==142&&alarm==2&&rejectDestroy){assert(!okay);break;}
                assert(okay);
            }
            if(tick==142&&rejectDestroy)break;
            const auto& state=native.find(key)->state;
            assert(state.shown==oracle.shown&&state.exit==oracle.exit);
            for(unsigned alarm=0;alarm<12;++alarm)assert(state.alarms.get(alarm)==oracle.alarms.get(alarm));
            Draw actual,expected;assert(native.draw(key,actual)&&oracle.draw(expected));
            assert(actual.tokens==expected.tokens&&actual.xs==expected.xs&&actual.ys==expected.ys);
            for(unsigned i=1;i<=3;++i)assert(state.characters[i].offset==oracle.characters[i].offset);
        }
        assert(life.marks==1&&!life.retired&&native.find(key));
        if(rejectDestroy){
            assert(!native.valid()&&!native.find(key)->destroyRequested&&native.find(key)->state.alarms.get(2)==0);
            assert(!native.advanceAlarm(key,2)&&life.marks==1);continue;
        }
        assert(native.find(key)->destroyRequested&&native.retire(key)&&!native.find(key)&&!native.size());
        InstanceHandle replacement;
        assert(native.create(1001,{}, {},[&](auto,auto& state){return state.create(1,{},read);},replacement));
        assert(replacement.generation!=key.generation&&!native.find(key)&&native.clear()&&life.retired==2);
    }
    {
        Lifetime life;Numbers native(Numbers::Binding::bindPositionedDestruction(life));InstanceHandle key;
        assert(!native.create(1000,{}, {},[](auto,auto&){return true;},key));
        assert(!native.valid()&&!key.generation&&life.births==1&&native.size()==1&&
            native.find(native.sourceHandle(1000))->state.alarms.get(0)==-1);
    }
    LevelNumber<8> title;
    assert(title.create(3,MotionBinary64::fromInteger(5),read));
    assert(title.shown==1&&title.i==4&&title.alarms.get(0)==4);
    Draw absent;absent.surface=false;
    assert(title.draw(absent)&&!absent.views&&!absent.ends&&title.characters[1].offset==-2);
    Draw first;
    assert(title.draw(first)&&first.tokens==std::vector<int>({101,101,101,101,101})&&
        first.xs==std::vector<int>({78,76,77,77,77})&&first.ys==std::vector<int>({80,80,81,79,80})&&
        first.colors==std::vector<unsigned>({0,0xffffffu})&&
        first.views==4&&first.ends==1&&title.characters[1].offset==-1);
    for(unsigned n=0;n<3;++n){Draw next;assert(title.draw(next));}
    assert(title.characters[1].offset==1&&title.characters[2].offset==-2);
    unsigned destroyed=0;
    for(unsigned tick=1;tick<=142;++tick){
        for(unsigned alarm=0;alarm<12;++alarm)assert(title.advanceAlarm(alarm,[&]{++destroyed;return true;}));
        if(tick==4)assert(title.shown==2&&title.characters[1].enabled==1);
        if(tick==8)assert(title.shown==3&&title.characters[2].enabled==1);
        if(tick==12)assert(title.alarms.get(1)==119);
        if(tick==131)assert(title.exit==1&&title.alarms.get(2)==3);
        if(tick==134)assert(title.exit==2&&title.characters[3].enabled==1);
        assert(destroyed==unsigned(tick==142));
    }
    assert(title.destroyRequested);
    LevelNumber<8> exiting;assert(exiting.create(3,MotionBinary64::fromInteger(5),read));
    exiting.exit=1;exiting.characters[1].offset=3;
    Draw exitDraw;assert(exiting.draw(exitDraw)&&exiting.characters[1].offset==4&&
        exitDraw.tokens==std::vector<int>({102,102,102,102,102,103,103,103,103,103}));
    LevelNumber<8> failed;assert(failed.create(3,{},read));failed.shown=3;
    Draw rejected;rejected.failCharacter=7;
    assert(!failed.draw(rejected)&&!failed.good&&failed.characters[1].offset==-1&&
        failed.characters[2].offset==-2&&!rejected.ends&&failed.i==2);
    for(unsigned failure=1;failure<=7;++failure){
        LevelNumber<8> interrupted;assert(interrupted.create(1,MotionBinary64::fromInteger(5),read));
        Draw ops;
        if(failure<=5)ops.failCharacter=failure;
        else ops.failColor=failure-5;
        assert(!interrupted.draw(ops)&&!interrupted.good&&!ops.ends&&interrupted.txt==101&&
            interrupted.characters[1].offset==-2);
        std::int32_t x,y;assert(interrupted.sx.roundPixel(x)&&interrupted.sy.roundPixel(y)&&x==77&&y==80);
        assert(ops.tokens.size()==(failure<=5?failure:failure==6?0u:4u));
    }
    LevelNumber<2> overflow;assert(!overflow.create(3,{},read));
    LevelNumber<8> retirement;assert(retirement.create(1,{},read));
    retirement.exit=1;retirement.alarms.set(2,1);unsigned retireCalls=0;
    assert(!retirement.advanceAlarm(2,[&]{++retireCalls;return false;})&&!retirement.good&&
        !retirement.destroyRequested&&retirement.alarms.get(2)==0&&retireCalls==1);
    assert(!retirement.advanceAlarm(2,[&]{++retireCalls;return true;})&&retireCalls==1);
    LevelNumber<8> birthFailure;
    assert(!birthFailure.create(3,{},[](unsigned i,unsigned& token){token=i;return i!=2;})&&
        birthFailure.characters[1].token==1&&birthFailure.i==2&&birthFailure.alarms.get(0)==-1);
    std::puts("Level-number event state and native lifetime: alarms, Draw, activation, deferred retirement and failure prefixes passed");
}
