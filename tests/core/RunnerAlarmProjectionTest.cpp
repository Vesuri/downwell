#include "core/RunnerAlarmProjection.h"
#include <cassert>
#include <vector>
using downwell::RunnerAlarmProjection;
int main(){
    RunnerAlarmProjection<129> projection;
    unsigned seed=7;
    for(unsigned pass=0;pass<500;++pass){
        seed=seed*1664525u+1013904223u;
        const unsigned count=seed%130;
        assert(projection.reset(count));
        unsigned masks[129]{};
        for(unsigned i=0;i<count;++i){
            seed=seed*1664525u+1013904223u;masks[i]=(seed>>15)&0xfffu;
            assert(projection.include(i,masks[i]));
        }
        for(unsigned alarm=0;alarm<12;++alarm)for(unsigned start=0;start<=count;++start){
            unsigned expected=start;
            while(expected<count&&!(masks[expected]&(1u<<alarm)))++expected;
            assert(projection.next(alarm,start)==expected);
        }
        assert(!projection.include(count,1));
        if(count)assert(!projection.include(0,0x1000));
        assert(projection.next(12,0)==count);
    }
    assert(!projection.reset(130));
    // Growing capacity for unrelated future families does not add visits to
    // an unchanged captured scene, including after reuse of a larger scene.
    RunnerAlarmProjection<4096> larger;
    assert(larger.reset(4096)&&larger.include(4095,0xfff));
    assert(larger.reset(129)&&projection.reset(129));
    for(unsigned i=0;i<129;++i){
        const unsigned mask=i%7==0?1u<<(i%12):0;
        assert(larger.include(i,mask)&&projection.include(i,mask));
    }
    for(unsigned alarm=0;alarm<12;++alarm){
        unsigned small=0,large=0;
        do{
            small=projection.next(alarm,small);large=larger.next(alarm,large);
            assert(small==large);
            if(small==129)break;
            ++small;++large;
        }while(true);
    }
    // External groups execute before their first greater object, even when
    // that object has no event. A callback changes a later captured object's
    // binding: abandon projection immediately and visit the original suffix.
    for(unsigned mutation=0;mutation<4;++mutation){
        auto run=[&](bool projected){
            const unsigned objects[]={35,40,61,91,93,100,200,211,220,240};
            unsigned masks[]={2,0,0,0,0,0,0,0,0,2};
            assert(projection.reset(10));
            for(unsigned i=0;i<10;++i)assert(projection.include(i,masks[i]));
            assert(projection.include(4,0xfff)&&projection.include(7,0xfff));
            std::vector<unsigned> events;bool unchanged=true;
            for(unsigned alarm=0;alarm<2;++alarm){
                bool boxes=alarm==0,enemies=alarm==1;
                auto next=[&](unsigned i){return projected&&unchanged?projection.next(alarm,i):i;};
                for(unsigned i=next(0);i<10;i=next(i+1)){
                    if(boxes&&objects[i]>92){
                        boxes=false;events.push_back(920);
                        if(mutation==1){masks[5]=3;unchanged=false;}
                    }
                    if(enemies&&objects[i]>210){
                        enemies=false;events.push_back(2101);
                        if(mutation==2){masks[8]=2;unchanged=false;}
                    }
                    if(masks[i]&(1u<<alarm)){
                        events.push_back(objects[i]*10+alarm);
                        if(mutation==3&&i==0&&alarm==1){masks[2]=2;unchanged=false;}
                    }
                }
                if(boxes)events.push_back(920);
                if(enemies)events.push_back(2101);
            }
            return events;
        };
        const auto reference=run(false);assert(run(true)==reference);
    }
}
