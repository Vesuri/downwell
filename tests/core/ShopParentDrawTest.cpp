#include "ShopParentDrawChecks.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
static ShopWorld::Instance fixture(unsigned mode,unsigned cursor,bool stock){
    ShopWorld::Instance shop;
    shop.menu.active=mode!=3;shop.menu.menuState=mode;shop.menu.cursor=cursor;
    shop.menu.menuSelect=cursor%2;shop.menu.cursorWobbleX=6;shop.menu.cursorPhaseQuarters=12;
    shop.draw.wobbleY=MotionBinary64::fromInteger(2);shop.draw.amount=2;
    shop.text.exit="exit";shop.text.buy="buy";shop.text.sell="sell";shop.text.back="back";shop.text.purchased="sold";
    for(unsigned i=0;i<3;++i){shop.stock.items[i]={100+i,stock?1u:0u,300};shop.text.itemName[i]="name";shop.text.itemDescription[i]="desc";}
    return shop;
}
static bool equal(MotionBinary64 a,int b){const auto v=MotionBinary64::fromInteger(b);return a.high==v.high&&a.low==v.low;}
int main(){
    for(unsigned mode=0;mode<4;++mode)for(unsigned cursor=0;cursor<(mode==1?4u:2u);++cursor)
    for(bool stock:{false,true})for(bool poor:{false,true})for(bool crime:{false,true}){
        auto shop=fixture(mode,cursor,stock);ShopParentDrawProbe expected;expected.money=poor?0:10000;expected.crime=crime;
        assert(ShopParentDraw::draw(shop,expected));
        assert(equal(shop.draw.boxX,480)&&equal(shop.draw.boxY,1057)&&equal(shop.draw.textX,486)&&equal(shop.draw.nameY,1034));
        assert(equal(shop.draw.wobbleY,1)&&shop.menu.cursorWobbleX==5&&shop.menu.cursorPhaseQuarters==(mode==3?12:-3));
        assert(expected.frames==(mode==3?0u:2u)&&expected.prompts==!crime);
        assert(expected.redPrice==(mode==1&&cursor&&stock&&poor));
        if(mode==0)assert(equal(shop.draw.menuX[0],512)&&equal(shop.draw.menuX[2],608)&&equal(shop.draw.menuY,1082));
        if(mode==1)assert(equal(shop.draw.arrowX,501+19*int(cursor)));
        if(mode==2)assert(shop.text.index==3&&equal(shop.draw.sellY,1106)&&equal(shop.draw.iconY,1094));
        for(unsigned reject=1;reject<=expected.calls;++reject){
            auto failed=fixture(mode,cursor,stock);ShopParentDrawProbe ops;ops.reject=reject;ops.money=expected.money;ops.crime=crime;
            assert(!ShopParentDraw::draw(failed,ops)&&ops.calls==reject);
            for(unsigned i=0;i<reject;++i)assert(ops.trace[i]==expected.trace[i]);
        }
    }
    // Closed menus still perform both overwritten layout reads. No cursor,
    // panel or branch commands are evaluated.
    auto closed=fixture(3,0,false);ShopParentDrawProbe ops;assert(ShopParentDraw::draw(closed,ops));
    const char expected[]={'S','h','X','Y','Y','H','Y','Y','H','h','c','p'};
    assert(ops.calls==sizeof(expected));for(unsigned i=0;i<ops.calls;++i)assert(ops.trace[i]==expected[i]);
    std::puts("Shop parent Draw: all menu branches, retained layout, cursor wrap and rejected prefixes passed");
}
