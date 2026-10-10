#include "core/ShopText.h"
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
    std::puts("Shop text assignment order, failure prefixes and generated language lookup passed");
}
