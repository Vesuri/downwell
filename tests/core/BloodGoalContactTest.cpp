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
    GoalTimeField field;field.x=240;field.y=4032;
    OrdinaryTimeField ordinary;ordinary.x=field.x;ordinary.y=field.y;
    unsigned checked=0,rejectedBroad=0,hits=0,changed=0;
    for(unsigned kind=0;kind<2;++kind){
    const int extent=kind?24:128,pixelScale=kind?1:2,canvas=kind?48:128;
    std::ifstream file(kind?"generated/downwell-time-field-mask.mask":"generated/downwell-goal-mask.mask",std::ios::binary);
    const std::vector<unsigned char> mask((std::istreambuf_iterator<char>(file)),{});
    assert(mask.size()==unsigned(canvas*canvas/8));
    for(float angle:{0.0f,0.000009f,0.000011f,-0.000009f,0.25f,10.5f,19.75f,160.25f,179.75f,180.0f}){
        RunnerImageValue rad,c,s;MotionBinary64 dc,ds;
        assert(SmokeDirection::radians(real(angle),rad) && smokeTrig(rad.promoted(),dc,ds) && c.assign(dc) && s.assign(ds));
        const float C=value(c),S=value(s);
        for(int dy=-extent-8;dy<=extent+8;dy+=4)for(int dx=-extent-8;dx<=extent+8;dx+=4){
            const float x=240+dx+.25f,y=4032+dy+.5f;
            BloodEffect blood;blood.x=real(x);blood.y=real(y);blood.imageAngle=real(angle);
            const int left=int(std::nearbyint((x+std::min(-2*C,2*C))+std::min(-S,2*S)));
            const int right=int(std::nearbyint((x+std::max(-2*C,2*C))+std::max(-S,2*S)));
            const int top=int(std::nearbyint((y+std::min(-C,2*C))-std::max(-2*S,2*S)));
            const int bottom=int(std::nearbyint((y+std::max(-C,2*C))-std::min(-2*S,2*S)));
            bool expected=false,broad=false,oldExpected=false;
            for(int px=left;px<right;++px)for(int py=top;py<bottom;++py){
                if(px<field.x-extent || px>=field.x+extent || py<field.y-extent || py>=field.y+extent)continue;
                const int fx=(px-field.x+extent)/pixelScale,fy=(py-field.y+extent)/pixelScale;
                if(!(mask[fy*(canvas/8)+fx/8]&(128>>(fx%8))))continue;
                broad=true;
                const float sx=((float(py)-y)*(-S)+(float(px)-x)*C)+12;
                const float sy=((float(py)-y)*C-(float(px)-x)*(-S))+12;
                oldExpected|=sx>=10&&sx<14&&sy>=11&&sy<14;
                if(std::abs(double(angle))<=1e-5){
                    const int mx=kind?12-int(std::nearbyint(x))+px:int((float(px)-x)+12);
                    const int my=kind?12-int(std::nearbyint(y))+py:int((float(py)-y)+12);
                    expected|=mx>=10&&mx<14&&my>=11&&my<14;
                }else expected|=sx>=10 && sx<14 && sy>=11 && sy<14;
            }
            bool actual=false;assert(kind?BloodGoalContact::query(blood,ordinary,actual):BloodGoalContact::query(blood,field,actual));
            assert(actual==expected);++checked;hits+=actual;rejectedBroad+=broad&&!actual;changed+=expected!=oldExpected;
        }
    }
    }
    assert(hits && rejectedBroad); // Detect an enclosing-box-only substitute.
    assert(changed>0);
    field.x=2147483647;bool hit=false;
    assert(!BloodGoalContact::query({},field,hit));
    std::printf("Blood/field contact: %u exact mask comparisons, %u hits, %u broad-box false positives rejected, %u sampling corrections\n",checked,hits,rejectedBroad,changed);
}
