#ifndef DOWNWELL_CORE_ORDINARY_TIME_FIELD_H
#define DOWNWELL_CORE_ORDINARY_TIME_FIELD_H
#include "GameMakerRandom.h"
#include "HudReal.h"
#include "EnemyContact.h"
#include "../../generated/downwell-time-field-mask.h"
namespace downwell {
// Object 82 parentTimeField, sprite 597 at unit scale. Collision never uses
// the randomized Draw radius. Integer centres cover builder and published
// player-position creation; transformed instances need a separate query.
struct OrdinaryTimeField {
    int x=0,y=0;
    HudReal radius;
    static HudReal nextRadius(GameMakerRandom& random){
        // 24 + random(2), exact binary64: exponent 4 and a 2^-48 mantissa unit.
        return {(std::uint64_t{3}<<51)|(static_cast<std::uint64_t>(random.nextU32())<<17),4};
    }
    void create(int px,int py,GameMakerRandom& random){x=px;y=py;radius=nextRadius(random);}
    void advanceDraw(bool stopped,GameMakerRandom& random){if(!stopped)radius=nextRadius(random);}
    static bool meetsRelativeBounds(ContactBounds bounds){
        if(bounds.left>=bounds.right || bounds.top>=bounds.bottom ||
           bounds.right<=-24 || bounds.left>=24 || bounds.bottom<=-24 || bounds.top>=24)return false;
        const int l=(bounds.left < -24?-24:bounds.left)+24;
        const int r=(bounds.right > 24?24:bounds.right)+24;
        const int t=(bounds.top < -24?-24:bounds.top)+24;
        const int b=(bounds.bottom > 24?24:bounds.bottom)+24;
        for(int row=t;row<b;++row)
            if(timeFieldMaskRows[row][0]<timeFieldMaskRows[row][1] &&
               l<timeFieldMaskRows[row][1] && r>timeFieldMaskRows[row][0])return true;
        return false;
    }
    template<class Circle> void drawSnapshot(int cameraX,int roomWidth,Circle circle)const{
        if(cameraX<=160 || cameraX>=roomWidth-160)return;
        circle(x,y,radius,false);
        circle(x,y,radius.subtract(HudReal::integer(1)),false);
        circle(x,y,radius.add(HudReal::integer(1)),true);
    }
};
}
#endif
