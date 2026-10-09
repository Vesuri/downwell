#ifndef DOWNWELL_RUNNER_LINE_TRAVERSAL_H
#define DOWNWELL_RUNNER_LINE_TRAVERSAL_H
#include "RunnerSpatialObjectPreparation.h"
#include "RunnerLineArguments.h"
namespace downwell {
// Object-selector collision_line traversal after family preparation
// (0x4fd370). Eligibility owns exclusions, not-me, kind and ancestry checks.
// Enumerators stop on false visitor results but return false only on failure.
struct RunnerLineTraversal {
    using Path=RunnerSpatialObjectPreparation::Path;
    struct Result {bool found=false;unsigned slot=0;};
    template<class Flush,class Eligible,class Candidate,class Spatial,class List>
    static bool run(Path path,const RunnerLineArguments& args,Flush flush,
        Eligible eligible,Candidate candidate,Spatial spatial,List list,Result& result){
        if(path==Path::failed)return false;
        if(path==Path::none){result={};return true;}
        const bool indexed=path==Path::spatial;
        if(indexed&&!flush())return false;
        Result next;bool good=true;
        auto visit=[&](unsigned slot){
            bool admitted=false;
            if(!eligible(slot,indexed,admitted)){good=false;return false;}
            if(!admitted)return true;
            bool hit=false;if(!candidate(slot,args,hit)){good=false;return false;}
            if(hit){next={true,slot};return false;}return true;
        };
        if(!(indexed?spatial(args,visit):list(visit))||!good)return false;
        result=next;return true;
    }
};
}
#endif
