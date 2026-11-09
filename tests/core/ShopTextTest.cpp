#include "core/ShopText.h"
#include "core/CompactShopSpeech.h"
#include "downwell-shop-text.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace downwell;
int main(){
    ShopInventory stock;stock.items[0].upgrade=100;stock.items[1].upgrade=104;stock.items[2].upgrade=105;
    const char* keys[]={"ugName100","ugDesc100","ugName104","ugDesc104","ugName105","ugDesc105",
        "shopExit","shopPurchased","menuBack","shopSold","shopGreet0","shopGreet1","shopGreet2",
        "choose","shopBye1","shopPoor","shopWhat0","shopWhat1"};
    for(unsigned reject=0;reject<=18;++reject){
        ShopText text;unsigned farewell=9;
        struct Ops {
            const char* const* keys;unsigned reject,calls=0;
            bool event(const char* key){assert(std::strcmp(key,keys[calls])==0);return ++calls!=reject;}
            bool lookup(const char* key,const char*& out){if(!event(key))return false;out="translated";return true;}
            bool chooseFarewell(unsigned& out){out=1;return event("choose");}
        } ops{keys,reject};
        assert(text.assign(stock,farewell,ops)==!reject&&ops.calls==(reject?reject:18));
        assert(farewell==(reject&&reject<=14?9u:1u));
        if(!reject)assert(text.index==4&&text.itemDescription[2]&&text.thank&&text.huh&&std::strcmp(text.buy,"BUY")==0);
    }
    assert(shop_language::count>0);
    const char* language=shop_language::entries[0].language;
    ShopLanguageLookup lookup{shop_language::entries,shop_language::count,language};
    for(unsigned i=0;i<shop_language::count;++i){
        const auto& entry=shop_language::entries[i];language=entry.language;const char* actual=nullptr;
        assert(lookup.lookup(entry.key,actual)&&std::strcmp(actual,entry.text)==0&&!std::strchr(actual,'*'));
    }
    const char* missing=nullptr;language="__missing_language__";
    assert(lookup.lookup("shopExit",missing)&&std::strcmp(missing," ")==0);
    // W1.8 compact merchant speech: synthetic table, no language payload.
    {
        static const ShopLanguageEntry table[]={
            {"a","shopSold","s"},{"a","shopGreet0","g0"},{"a","shopGreet1","g1"},{"a","shopGreet2","g2"},
            {"a","shopBye0","b0"},{"a","shopBye1","b1"},{"a","shopBye2","b2"},{"a","shopPoor","p"},
            {"a","shopWhat0","w"},{"a","shopWhat1","h"}};
        CompactShopSpeechText text;
        assert(text.configure(table,10,"a"));
        using M=ShopFeedback::Message;
        const M messages[]={M::welcome,M::hello,M::hoho,M::maido,M::poor,M::question,M::what,M::huh};
        const char* expected[]={"g0","g1","g2","s","p","?","w","h"};
        for(unsigned i=0;i<8;++i)assert(std::strcmp(text.text(messages[i],0),expected[i])==0);
        for(unsigned farewell=0;farewell<3;++farewell){
            const char bye[]={'b',char('0'+farewell),0};
            assert(std::strcmp(text.text(M::thank,farewell),bye)==0);
        }
        assert(!text.text(M::thank,3)&&!text.text(M::none,0));
        // A missing language falls back to langString's " ", not a failure.
        assert(text.configure(table,10,"b")&&std::strcmp(text.text(M::maido,0)," ")==0);
        assert(!text.configure(nullptr,0,"a"));
        // fxRisingText: flashing from Alarm 1 (60), destroyed by Alarm 0 (90),
        // rising by ysp *= 0.85 from -3, and kept ten pixels inside the view.
        CompactShopSpeech r;r.rise.live=true;r.rise.x=20;r.rise.y=long(100)<<16;r.length=5;
        unsigned updates=0,hidden=0;
        while(r.rise.live){
            ++updates;
            for(unsigned index=0;index<2;++index)(void)r.rise.alarmEvent(index);
            if(!r.rise.live)break;
            r.rise.step();hidden+=!r.rise.visible();
            if(updates<60)assert(r.rise.visible());
        }
        assert(updates==90&&hidden==15&&r.rise.pixelY()<100&&r.rise.pixelY()>=80);
        assert(r.left(0,160)==30&&(r.rise.x=150,r.left(0,160))==130&&(r.rise.x=80,r.left(0,160))==80);
    }
    std::puts("Shop text assignment order, failure prefixes, generated language lookup and merchant speech passed");
}
