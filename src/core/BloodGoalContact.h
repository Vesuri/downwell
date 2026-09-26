#ifndef DOWNWELL_CORE_BLOOD_GOAL_CONTACT_H
#define DOWNWELL_CORE_BLOOD_GOAL_CONTACT_H
#include "BloodCollisionBounds.h"
#include "BloodMaskSample.h"
#include "GoalTimeField.h"
namespace downwell {
// Source compatibility-mode sprite-84 versus sprite-616, field scale 2,
// integral field centre, positive scales and nearest-even bbox conversion.
struct BloodGoalContact {
    static bool query(const BloodEffect& blood,const GoalTimeField& field,bool& hit){
        // Bound the integer-field specialization so float pixel subtraction
        // and /2 are exact, and all translated pixel bounds fit signed int.
        if(field.x < -1048576 || field.x > 1048576 ||
           field.y < -1048576 || field.y > 1048576)return false;
        ContactBounds box;
        if(!BloodCollisionBounds::calculate(blood,box))return false;
        const int left=box.left>field.x-128?box.left:field.x-128;
        const int right=box.right<field.x+128?box.right:field.x+128;
        const int top=box.top>field.y-128?box.top:field.y-128;
        const int bottom=box.bottom<field.y+128?box.bottom:field.y+128;
        hit=false;
        if(left>=right || top>=bottom)return true;
        BloodMaskSample sample;
        if(!sample.prepare(blood))return false;
        // Runner traverses X outermost; stop at the first shared solid pixel.
        for(int x=left;x<right;++x)for(int y=top;y<bottom;++y){
            bool bloodPixel=false;
            if(!sample.contains(x,y,bloodPixel))return false;
            if(!bloodPixel)continue;
            const int sx=(x-field.x+128)/2,sy=(y-field.y+128)/2;
            if(sx>=goalMaskRows[sy][0] && sx<goalMaskRows[sy][1]){hit=true;return true;}
        }
        return true;
    }
};
}
#endif
