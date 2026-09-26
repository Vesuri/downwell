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
    using Dispatch=RunnerMaskDispatch;
    using Path=Dispatch::Path;
    Dispatch dispatch;
    unsigned dispatchCases=0;
    const float low=float(1.0-1e-5),high=float(1.0+1e-5);
    const float scaleCases[]={-1,0,low,std::nextafter(low,0.f),1,high,std::nextafter(high,2.f),2};
    const float angleCases[]={0,float(1e-5),std::nextafter(float(1e-5),1.f),-float(1e-5),-1};
    for(float ax:scaleCases)for(float ay:scaleCases)for(float bx:scaleCases)for(float by:scaleCases)
    for(float aa:angleCases)for(float ba:angleCases){
        const bool unit=ax>=low&&ax<=high&&ay>=low&&ay<=high&&bx>=low&&bx<=high&&by>=low&&by<=high;
        const Path expected=(!ax||!ay||!bx||!by)?Path::empty:
            (std::abs(double(aa))>1e-5||std::abs(double(ba))>1e-5)?Path::rotated:
            unit?Path::roundedPosition:Path::scaled;
        assert(dispatch.select({real(ax),real(ay),real(aa)},{real(bx),real(by),real(ba)})==expected);++dispatchCases;
    }
    assert(dispatch.select({{0x7fc00000},real(1),{}},{real(1),real(1),{}})==Path::unsupported);
    assert(dispatch.configure({0x80000000,0})); // Negative zero epsilon is zero.
    assert(dispatch.select({real(1),real(1),{}},{real(1),real(1),{}})==Path::roundedPosition);
    assert(dispatch.select({real(high),real(1),{}},{real(1),real(1),{}})==Path::scaled);
    assert(!dispatch.configure(MotionBinary64::fromInteger(-1)));
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
    {
        std::ifstream ordinaryFile("generated/downwell-time-field-mask.mask",std::ios::binary);
        const std::vector<unsigned char> pixels((std::istreambuf_iterator<char>(ordinaryFile)),{});
        assert(pixels.size()==288);
        OrdinaryTimeField ordinary;ordinary.x=240;ordinary.y=4032;
        unsigned count=0,ordinaryHits=0,rejected=0,corrected=0;
        const float ordinaryScales[]={-2,-1,-.75f,.5f,1,1.25f,2,low,high,std::nextafter(low,0.f),std::nextafter(high,2.f)};
        for(auto mask:{TimeStopGoalContact::Mask::Player,TimeStopGoalContact::Mask::Shard})
        for(unsigned i=0;i<24000;++i){
            const float x=ordinary.x+int(i*37%73)-36+(i%8)*.125f;
            const float y=ordinary.y+int(i*71%73)-36+(i%7)*.125f;
            const float xs=ordinaryScales[i%11],ys=ordinaryScales[i/11%11];
            const int first=mask==TimeStopGoalContact::Mask::Player?8:10;
            const int end=mask==TimeStopGoalContact::Mask::Player?20:13;
            const float a=-4*xs+x,c=(first-12)*ys+y;
            int l=int(std::nearbyint(a)),r=int(std::nearbyint(8*xs+a));
            int t=int(std::nearbyint(c)),b=int(std::nearbyint((end-first)*ys+c));
            if(l>r)std::swap(l,r);if(t>b)std::swap(t,b);
            l=std::max(l,ordinary.x-24);r=std::min(r,ordinary.x+24);
            t=std::max(t,ordinary.y-24);b=std::min(b,ordinary.y+24);
            const float ix=1/xs,iy=1/ys,px=x-(ix<0?1:0),py=y-(iy<0?1:0);
            const bool rounded=xs>=low&&xs<=high&&ys>=low&&ys<=high;
            bool expected=false,broad=false,oldExpected=false;
            for(int xx=l;xx<r;++xx)for(int yy=t;yy<b;++yy){
                const int fx=xx-ordinary.x+24,fy=yy-ordinary.y+24;
                if(!(pixels[fy*6+fx/8]&(128>>(fx%8))))continue;
                broad=true;
                const int oldX=int((float(xx)-px)*ix+12),oldY=int((float(yy)-py)*iy+12);
                oldExpected|=oldX>=8&&oldX<16&&oldY>=first&&oldY<end;
                const int sx=rounded?12-int(std::nearbyint(x))+xx:oldX;
                const int sy=rounded?12-int(std::nearbyint(y))+yy:oldY;
                expected|=sx>=8 && sx<16 && sy>=first && sy<end;
            }
            bool actual=!expected;
            assert(TimeStopGoalContact::query(mask,real(x),real(y),real(xs),real(ys),ordinary,actual));
            assert(actual==expected);++count;ordinaryHits+=actual;rejected+=broad&&!actual;corrected+=expected!=oldExpected;
        }
        assert(ordinaryHits && rejected);
        assert(corrected>0);
        std::printf("Ordinary field rounded-position correction: %u cases differ from scaled-only sampling\n",corrected);
        std::printf("Ordinary field contact: %u comparisons, %u hits, %u bbox false positives rejected\n",
            count,ordinaryHits,rejected);
    }
    bool hit=true;
    assert(!TimeStopGoalContact::query(TimeStopGoalContact::Mask::Player,{}, {},{},real(1),field,hit) && hit);
    assert(!TimeStopGoalContact::query(TimeStopGoalContact::Mask::Player,{0x7f800000}, {},real(1),real(1),field,hit));
    field.x=2147483647;
    assert(!TimeStopGoalContact::query(TimeStopGoalContact::Mask::Player,{}, {},real(1),real(1),field,hit));
    std::printf("Time-stop goal contact: %u comparisons, %u hits, %u bbox false positives rejected\n",cases,hits,broadRejected);
    std::printf("Pair-wide precise mask dispatch: %u tolerance-boundary comparisons passed\n",dispatchCases);
}
