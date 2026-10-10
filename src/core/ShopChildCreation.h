#ifndef DOWNWELL_CORE_SHOP_CHILD_CREATION_H
#define DOWNWELL_CORE_SHOP_CHILD_CREATION_H
#include <cstdint>
namespace downwell {
// Generated Shop 277's synchronous child sequence. Parent registration,
// inherited Create and radio setup precede beforeStock. Stock RNG belongs
// between the two phases; farewell text RNG follows afterStock. Callbacks
// retain real source IDs and complete child Create, never reserve a block of
// IDs or reconstruct returned IDs from the allocator after nested births.
struct ShopChildCreation {
    std::uint32_t table=0,cases[3]{},wall=0,keeper=0,risingText=0,items[3]{};
    int keeperX=0;
    template<class Create,class ReadX>
    bool beforeStock(int x,int y,Create create,ReadX readX){
        if(started_)return good_=false;
        started_=true;
        if(x>0x7fffffff-83||y>0x7fffffff-8||y<(-0x7fffffff-1)+9)return good_=false;
        if(!birth(table,278,x+64,y,create))return false;
        for(unsigned i=0;i<3;++i)
            if(!birth(cases[i],279,x+45+int(i)*19,y-9,create))return false;
        if(!birth(wall,98,x,y+8,create)||!birth(keeper,280,x+18,y,create)||
           !readX(keeper,keeperX))return good_=false;
        if(!birth(risingText,212,-100,-100,create))return false;
        beforeComplete_=true;return true;
    }
    template<class Stock,class Create,class ReadPosition,class SetUpgrade,class SetStored>
    bool afterStock(const Stock& stock,Create create,ReadPosition position,SetUpgrade upgrade,SetStored stored){
        if(!good_||!beforeComplete_||afterStarted_)return good_=false;
        afterStarted_=true;
        for(unsigned i=0;i<3;++i){
            int x=0,y=0;
            if(!position(cases[i],x,y)||y==0x7fffffff||!birth(items[i],283,x,y+1,create)||
               !upgrade(items[i],stock.items[i].upgrade)||!stored(cases[i],1))return good_=false;
        }
        return true;
    }
    bool succeeded()const{return good_;}
private:
    template<class Create> bool birth(std::uint32_t& slot,int object,int x,int y,Create& create){
        if(!good_)return false;
        const auto id=create(object,x,y);
        if(!id||id>0x7fffffffu)return good_=false;
        slot=id;return true;
    }
    bool started_=false,beforeComplete_=false,afterStarted_=false,good_=true;
};
}
#endif
