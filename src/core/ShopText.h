#ifndef DOWNWELL_CORE_SHOP_TEXT_H
#define DOWNWELL_CORE_SHOP_TEXT_H
#include "ShopInventory.h"
namespace downwell {
struct ShopLanguageEntry {const char* language;const char* key;const char* text;};
// Generated text already applies langString's '*' -> '#' conversion. Storage
// must outlive cached Shop labels and all rising-text children that borrow it.
struct ShopLanguageLookup {
    const ShopLanguageEntry* entries;unsigned count;const char* const& language;
    static bool same(const char* a,const char* b){
        if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;
    }
    bool lookup(const char* key,const char*& out)const{
        if(!language||!key||(!entries&&count))return false;
        for(unsigned i=0;i<count;++i)if(same(language,entries[i].language)&&same(key,entries[i].key)){
            out=entries[i].text;return out!=nullptr;
        }
        out=" ";return true;
    }
};
struct ShopText {
    const char* itemName[3]{},*itemDescription[3]{};
    const char* exit=nullptr,*buy=nullptr,*sellBattery=nullptr,*sell=nullptr,*purchased=nullptr,*back=nullptr;
    const char* sold=nullptr,*greeting[3]{},*thank=nullptr,*poor=nullptr,*what=nullptr,*huh=nullptr;
    unsigned index=0; // scrShopAssignText's retained self.i
    template<class Ops> bool assign(const ShopInventory& stock,unsigned& farewell,Ops& ops){
        for(index=1;index<=3;++index){
            const auto id=stock.items[index-1].upgrade;
            if(id<100||id>105)return false; // Original Shop stock domain.
            char name[]="ugName100",description[]="ugDesc100";
            name[8]=description[8]=char('0'+id-100);
            if(!ops.lookup(name,itemName[index-1])||!ops.lookup(description,itemDescription[index-1]))return false;
        }
        if(!ops.lookup("shopExit",exit))return false;
        buy="BUY";sellBattery="SELL#BTRY#";sell="SELL";
        if(!ops.lookup("shopPurchased",purchased)||!ops.lookup("menuBack",back)||
           !ops.lookup("shopSold",sold)||!ops.lookup("shopGreet0",greeting[0])||
           !ops.lookup("shopGreet1",greeting[1])||!ops.lookup("shopGreet2",greeting[2]))return false;
        unsigned choice;if(!ops.chooseFarewell(choice)||choice>2)return false;
        farewell=choice;const char* bye[]={"shopBye0","shopBye1","shopBye2"};
        return ops.lookup(bye[choice],thank)&&ops.lookup("shopPoor",poor)&&
            ops.lookup("shopWhat0",what)&&ops.lookup("shopWhat1",huh);
    }
};
}
#endif
