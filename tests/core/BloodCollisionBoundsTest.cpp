#include "core/BloodCollisionBounds.h"
#include "core/BloodMaskSample.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace downwell;
static float value(RunnerImageValue n){float f;std::memcpy(&f,&n.bits,4);return f;}
int main(){
    unsigned checked=0;
    for(int angle=-720;angle<=720;++angle){
        const float degrees=angle/4.0f;RunnerImageValue a;std::memcpy(&a.bits,&degrees,4);
        RunnerImageValue rad,c,s;MotionBinary64 cosine,sine;
        assert(SmokeDirection::radians(a,rad) && smokeTrig(rad.promoted(),cosine,sine) && c.assign(cosine) && s.assign(sine));
        // Native float arithmetic independently checks product/sum narrowing
        // and integer conversion; trig itself is covered by its separate audit.
        const float C=value(c),S=value(s);
        auto inverse=a;inverse.bits^=0x80000000u;
        RunnerImageValue ic,is;
        assert(SmokeDirection::radians(inverse,rad) && smokeTrig(rad.promoted(),cosine,sine) &&
               ic.assign(cosine) && is.assign(sine));
        const float IC=value(ic),IS=value(is);
        for(float x:{-256.5f,0.0f,159.75f,240.0f,320.25f})for(float y:{-4.5f,176.0f,4032.25f}){
            BloodEffect effect;std::memcpy(&effect.x.bits,&x,4);std::memcpy(&effect.y.bits,&y,4);effect.imageAngle=a;
            ContactBounds actual;assert(BloodCollisionBounds::calculate(effect,actual));
            BloodMaskSample sample;assert(sample.prepare(effect));
            for(int py=int(y)-4;py<=int(y)+4;++py)for(int px=int(x)-4;px<=int(x)+4;++px){
                const float dx=float(px)-x,dy=float(py)-y;
                const float sx=(dy*IS+dx*IC)+12;
                const float sy=(dy*IC-dx*IS)+12;
                bool hit=false;assert(sample.contains(px,py,hit));
                assert(hit==(sx>=10 && sx<14 && sy>=11 && sy<14));
            }
            const float cl=C*-2,cr=C*2,st=S*-1,sb=S*2,ct=C*-1,cb=C*2,sl=S*-2,sr=S*2;
            const float left=(x+std::min(cl,cr))+std::min(st,sb);
            const float right=(x+std::max(cl,cr))+std::max(st,sb);
            const float top=(y+std::min(ct,cb))-std::max(sl,sr);
            const float bottom=(y+std::max(ct,cb))-std::min(sl,sr);
            assert(actual.left==std::nearbyint(left) && actual.right==std::nearbyint(right) &&
                actual.top==std::nearbyint(top) && actual.bottom==std::nearbyint(bottom));++checked;
        }
    }
    std::printf("Blood collision bounds: %u rotated/fractional cases match binary32 scalar arithmetic\n",checked);
}
