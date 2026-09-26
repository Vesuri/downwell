#include "core/BloodGoalContact.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static RunnerImageValue real(float v){RunnerImageValue n;std::memcpy(&n.bits,&v,4);return n;}
static float value(RunnerImageValue n){float v;std::memcpy(&v,&n.bits,4);return v;}
int main(){
    std::ifstream file("generated/downwell-goal-mask.mask",std::ios::binary);
    const std::vector<unsigned char> mask((std::istreambuf_iterator<char>(file)),{});
    assert(mask.size()==2048);
    GoalTimeField field;field.x=240;field.y=4032;
    unsigned checked=0,rejectedBroad=0,hits=0;
    for(float angle:{0.0f,0.25f,10.5f,19.75f,160.25f,179.75f,180.0f}){
        RunnerImageValue rad,c,s;MotionBinary64 dc,ds;
        assert(SmokeDirection::radians(real(angle),rad) && smokeTrig(rad.promoted(),dc,ds) && c.assign(dc) && s.assign(ds));
        const float C=value(c),S=value(s);
        for(int dy=-136;dy<=136;dy+=4)for(int dx=-136;dx<=136;dx+=4){
            const float x=240+dx+.25f,y=4032+dy+.5f;
            BloodEffect blood;blood.x=real(x);blood.y=real(y);blood.imageAngle=real(angle);
            const int left=int(std::nearbyint((x+std::min(-2*C,2*C))+std::min(-S,2*S)));
            const int right=int(std::nearbyint((x+std::max(-2*C,2*C))+std::max(-S,2*S)));
            const int top=int(std::nearbyint((y+std::min(-C,2*C))-std::max(-2*S,2*S)));
            const int bottom=int(std::nearbyint((y+std::max(-C,2*C))-std::min(-2*S,2*S)));
            bool expected=false,broad=false;
            for(int px=left;px<right;++px)for(int py=top;py<bottom;++py){
                if(px<112 || px>=368 || py<3904 || py>=4160)continue;
                const int fx=(px-112)/2,fy=(py-3904)/2;
                if(!(mask[fy*16+fx/8]&(128>>(fx%8))))continue;
                broad=true;
                const float sx=((float(py)-y)*(-S)+(float(px)-x)*C)+12;
                const float sy=((float(py)-y)*C-(float(px)-x)*(-S))+12;
                expected|=sx>=10 && sx<14 && sy>=11 && sy<14;
            }
            bool actual=false;assert(BloodGoalContact::query(blood,field,actual));
            assert(actual==expected);++checked;hits+=actual;rejectedBroad+=broad&&!actual;
        }
    }
    assert(hits && rejectedBroad); // Detect an enclosing-box-only substitute.
    field.x=2147483647;bool hit=false;
    assert(!BloodGoalContact::query({},field,hit));
    std::printf("Blood/goal contact: %u exact mask comparisons, %u hits, %u broad-box false positives rejected\n",checked,hits,rejectedBroad);
}
