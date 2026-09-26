#ifndef DOWNWELL_CORE_BLOOD_GOAL_CONTACT_H
#define DOWNWELL_CORE_BLOOD_GOAL_CONTACT_H
#include "BloodCollisionBounds.h"
#include "BloodMaskSample.h"
#include "GoalTimeField.h"
#include "OrdinaryTimeField.h"
namespace downwell {
// Source compatibility-mode sprite-84 versus sprite-616/597, field scale 2/1,
// integral field centre, positive scales and nearest-even bbox conversion.
struct BloodGoalContact {
    static bool query(const BloodEffect& blood,const GoalTimeField& field,bool& hit){
        return queryField(blood,field.x,field.y,128,2,goalMaskRows,hit);
    }
    static bool query(const BloodEffect& blood,const OrdinaryTimeField& field,bool& hit){
        return queryField(blood,field.x,field.y,24,1,timeFieldMaskRows,hit);
    }
private:
    template<class Rows> static bool queryField(const BloodEffect& blood,int fieldX,int fieldY,
        int extent,int pixelScale,const Rows& rows,bool& hit){
        // Bound the integer-field specialization so float pixel subtraction
        // and /2 are exact, and all translated pixel bounds fit signed int.
        if(fieldX < -1048576 || fieldX > 1048576 ||
           fieldY < -1048576 || fieldY > 1048576)return false;
        ContactBounds box;
        if(!BloodCollisionBounds::calculate(blood,box))return false;
        const int left=box.left>fieldX-extent?box.left:fieldX-extent;
        const int right=box.right<fieldX+extent?box.right:fieldX+extent;
        const int top=box.top>fieldY-extent?box.top:fieldY-extent;
        const int bottom=box.bottom<fieldY+extent?box.bottom:fieldY+extent;
        hit=false;
        if(left>=right || top>=bottom)return true;
        BloodMaskSample sample;
        const RunnerImageValue unit{0x3f800000},scale{pixelScale==1?0x3f800000u:0x40000000u};
        const RunnerMaskDispatch dispatch;
        const auto path=dispatch.select({unit,unit,blood.imageAngle},{scale,scale,{}});
        if(!sample.prepare(blood,path))return false;
        // Runner traverses X outermost; stop at the first shared solid pixel.
        for(int x=left;x<right;++x)for(int y=top;y<bottom;++y){
            bool bloodPixel=false;
            if(!sample.contains(x,y,bloodPixel))return false;
            if(!bloodPixel)continue;
            const int sx=(x-fieldX+extent)/pixelScale,sy=(y-fieldY+extent)/pixelScale;
            if(sx>=rows[sy][0] && sx<rows[sy][1]){hit=true;return true;}
        }
        return true;
    }
};
}
#endif
