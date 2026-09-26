#include "core/TimeStopGoalContact.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
using namespace downwell;
static RunnerImageValue real(float v){RunnerImageValue n;std::memcpy(&n.bits,&v,4);return n;}
int main(){
    std::ifstream file("generated/downwell-goal-mask.mask",std::ios::binary);
    const std::vector<unsigned char> bitmap((std::istreambuf_iterator<char>(file)),{});
    assert(bitmap.size()==2048);
    GoalTimeField field;field.x=240;field.y=4032;
    unsigned cases=0,hits=0,broadRejected=0;
    const float scales[]={-2,-1,-.75f,.5f,1,1.25f,2};
    for(auto mask:{TimeStopGoalContact::Mask::Player,TimeStopGoalContact::Mask::Shard})
    for(unsigned i=0;i<24000;++i){
        const float x=field.x+int(i*37%281)-140+(i%8)*.125f;
        const float y=field.y+int(i*71%281)-140+(i%7)*.125f;
        const float xs=scales[i%7],ys=scales[i/7%7];
        const int first=mask==TimeStopGoalContact::Mask::Player?8:10;
        const int end=mask==TimeStopGoalContact::Mask::Player?20:13;
        const float a=-4*xs+x,c=(first-12)*ys+y;
        int l=int(std::nearbyint(a)),r=int(std::nearbyint(8*xs+a));
        int t=int(std::nearbyint(c)),b=int(std::nearbyint((end-first)*ys+c));
        if(l>r)std::swap(l,r);if(t>b)std::swap(t,b);
        l=std::max(l,field.x-128);r=std::min(r,field.x+128);
        t=std::max(t,field.y-128);b=std::min(b,field.y+128);
        const float ix=1/xs,iy=1/ys,px=x-(ix<0?1:0),py=y-(iy<0?1:0);
        bool expected=false,broad=false;
        for(int xx=l;xx<r;++xx)for(int yy=t;yy<b;++yy){
            const int fx=(xx-field.x+128)/2,fy=(yy-field.y+128)/2;
            if(!(bitmap[fy*16+fx/8]&(128>>(fx%8))))continue;
            broad=true;
            const int sx=int((float(xx)-px)*ix+12),sy=int((float(yy)-py)*iy+12);
            expected|=sx>=8 && sx<16 && sy>=first && sy<end;
        }
        bool actual=!expected;
        assert(TimeStopGoalContact::query(mask,real(x),real(y),real(xs),real(ys),field,actual));
        assert(actual==expected);++cases;hits+=actual;broadRejected+=broad&&!actual;
    }
    assert(hits && broadRejected);
    bool hit=true;
    assert(!TimeStopGoalContact::query(TimeStopGoalContact::Mask::Player,{}, {},{},real(1),field,hit) && hit);
    assert(!TimeStopGoalContact::query(TimeStopGoalContact::Mask::Player,{0x7f800000}, {},real(1),real(1),field,hit));
    field.x=2147483647;
    assert(!TimeStopGoalContact::query(TimeStopGoalContact::Mask::Player,{}, {},real(1),real(1),field,hit));
    std::printf("Time-stop goal contact: %u comparisons, %u hits, %u bbox false positives rejected\n",cases,hits,broadRejected);
}
