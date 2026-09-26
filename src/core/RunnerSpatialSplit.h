#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_SPLIT_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_SPLIT_H
#include "RunnerImageValue.h"
#include "MultiplyBinary64.h"
namespace downwell {
// Runner 0x5010e0/0x4fc680/0x500490: partition six entries plus one new
// entry. This is not a complete spatial index or an instance_place adapter.
// Finite normal/zero binary32 inputs only; failures leave output untouched.
struct RunnerSpatialSplit {
    using Real=RunnerImageValue;
    struct Box { Real left,top,right,bottom; };
    struct Result { unsigned group[7]={}; unsigned count[2]={}; };
    static bool partition(const Box (&boxes)[7],Result& output){
        Real areas[7];Box all=boxes[0];
        for(unsigned i=0;i<7;++i){
            if(!valid(boxes[i]) || !area(boxes[i],areas[i]))return false;
            all=unite(all,boxes[i]);
        }
        Real best,whole;
        if(!area(all,whole) || !sub(negate(whole),Real{0x3f800000u},best))return false;
        unsigned first=0,second=1;bool unset=true;
        for(unsigned i=0;i<6;++i)for(unsigned j=i+1;j<7;++j){
            Real waste;
            if(!area(unite(boxes[i],boxes[j]),waste) ||
               !sub(waste,areas[i],waste) || !sub(waste,areas[j],waste))return false;
            if(less(best,waste) || (unset && equal(waste,best))){
                first=i;second=j;best=waste;unset=false;
            }
        }
        Result next;for(auto& g:next.group)g=2;
        Box bounds[2]={boxes[first],boxes[second]};
        Real groupArea[2]={areas[first],areas[second]};
        next.group[first]=0;next.group[second]=1;next.count[0]=next.count[1]=1;
        unsigned assigned=2;
        while(assigned<7 && next.count[0]<5 && next.count[1]<5){
            Real largest{0xbf800000u};unsigned chosen=0,group=0;
            for(unsigned i=0;i<7;++i)if(next.group[i]==2){
                Real growth[2],difference;
                for(unsigned g=0;g<2;++g)
                    if(!area(unite(bounds[g],boxes[i]),growth[g]) ||
                       !sub(growth[g],groupArea[g],growth[g]))return false;
                if(!sub(growth[1],growth[0],difference))return false;
                const unsigned preferred=less(difference,Real{})?1:0;
                Real magnitude{difference.bits&0x7fffffffu};
                if(less(largest,magnitude) || (equal(largest,magnitude) &&
                   next.count[preferred]<next.count[group])){
                    largest=magnitude;chosen=i;group=preferred;
                }
            }
            next.group[chosen]=group;++next.count[group];++assigned;
            bounds[group]=unite(bounds[group],boxes[chosen]);
            if(!area(bounds[group],groupArea[group]))return false;
        }
        const unsigned remainder=next.count[0]>=5?1:0;
        for(unsigned i=0;i<7;++i)if(next.group[i]==2){
            next.group[i]=remainder;++next.count[remainder];
        }
        // The caller distributes entries in ORIGINAL array order, not the
        // order in which this routine assigned their groups.
        output=next;return true;
    }
private:
    static bool valid(Real x){const unsigned e=(x.bits>>23)&255;return e!=255 && (e || !(x.bits&0x7fffffff));}
    static bool valid(Box b){return valid(b.left)&&valid(b.top)&&valid(b.right)&&valid(b.bottom)&&!less(b.right,b.left)&&!less(b.bottom,b.top);}
    static bool equal(Real a,Real b){return a.bits==b.bits || (!(a.bits&0x7fffffff)&&!(b.bits&0x7fffffff));}
    static bool less(Real a,Real b){
        if(equal(a,b))return false;
        if((a.bits^b.bits)&0x80000000u)return (a.bits>>31)!=0;
        return a.bits&0x80000000u?a.bits>b.bits:a.bits<b.bits;
    }
    static Real negate(Real a){a.bits^=0x80000000u;return a;}
    static bool sub(Real a,Real b,Real& out){auto n=a.promoted();return n.add(negate(b).promoted())&&out.assign(n);}
    static Box unite(Box a,Box b){return {less(a.left,b.left)?a.left:b.left,less(a.top,b.top)?a.top:b.top,
        less(b.right,a.right)?a.right:b.right,less(b.bottom,a.bottom)?a.bottom:b.bottom};}
    static bool area(Box b,Real& out){
        Real w,h;MotionBinary64 product;
        return sub(b.right,b.left,w)&&sub(b.bottom,b.top,h)&&
            multiplyBinary64(w.promoted(),h.promoted(),product)&&out.assign(product);
    }
};
}
#endif
