#ifndef DOWNWELL_RUNNER_POINT_TRAVERSAL_H
#define DOWNWELL_RUNNER_POINT_TRAVERSAL_H
#include "RunnerSpatialObjectPreparation.h"
namespace downwell {
// Traversal after object-family preparation in position_meeting (0x52b780).
// Members supplies the selected registration list; query supplies the shared
// tree. Both must treat callback false as early termination, not corruption.
struct RunnerPointTraversal {
    template<class Flush,class Eligible,class Candidate,class Query,class Members>
    static bool run(RunnerSpatialObjectPreparation::Path path,RunnerImageValue x,RunnerImageValue y,
        bool compatibility,Flush flush,Eligible eligible,Candidate candidate,Query query,Members members,bool& hit){
        using Prep=RunnerSpatialObjectPreparation;using Real=RunnerImageValue;
        if(path==Prep::Path::failed)return false;
        if(path==Prep::Path::none){hit=false;return true;}
        const bool indexed=path==Prep::Path::spatial;
        if(indexed&&!flush())return false;
        bool good=true,result=false;
        auto visit=[&](unsigned slot){
            bool admitted=false;
            if(!eligible(slot,indexed,admitted)){good=false;return false;}
            if(!admitted)return true;
            if(!candidate(slot,x,y,result)){good=false;return false;}
            return !result;
        };
        if(indexed){
            const auto truncate=[](Real value){
                const int exponent=int((value.bits>>23)&255)-127;
                if(exponent<0)return Real{};
                if(exponent>=31)return Real{0xcf000000u};
                if(exponent<23)value.bits&=~((1u<<unsigned(23-exponent))-1u);
                return value;
            };
            const Real bx=compatibility?truncate(x):x,by=compatibility?truncate(y):y;
            if(!query(RunnerSpatialSplit::Box{bx,by,bx,by},visit))good=false;
        }else if(!members(visit))good=false;
        if(!good)return false;
        hit=result;return true;
    }
};
}
#endif
