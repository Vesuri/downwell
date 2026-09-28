#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_SPLIT_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_SPLIT_H
#include "RunnerImageValue.h"
#include "MultiplyBinary64.h"
#include "MultiplyBinary32.h"
#include "SubtractBinary32.h"
#include "Profile.h"
namespace downwell {
// Runner 0x5010e0/0x4fc680/0x500490: partition six entries plus one new
// entry. This is not a complete spatial index or an instance_place adapter.
// Finite normal/zero binary32 inputs only; failures leave output untouched.
struct RunnerSpatialSplit {
private:
    struct PixelBox {std::int32_t left,top,right,bottom;};
public:
    using Real=RunnerImageValue;
    struct Box { Real left,top,right,bottom; };
    struct Result { unsigned group[7]={}; unsigned count[2]={}; };
    static bool partition(const Box (&boxes)[7],Result& output){
        DW_PROFILE_SCOPE(profileSpatialPartition);
        if(partitionPixels(boxes,output))return true;
        return partitionBinary32(boxes,output);
    }
    static bool partitionBinary32(const Box (&boxes)[7],Result& output){
        DW_PROFILE_SCOPE(profileSpatialPartitionFallback);
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
    // 0x4fff98: least enlargement, then least area, then earliest slot.
    static bool chooseChild(const Box* boxes,unsigned count,Box incoming,unsigned& chosen){
        DW_PROFILE_SCOPE(profileSpatialChoose);
        if(choosePixelChild(boxes,count,incoming,chosen))return true;
        return chooseBinary32Child(boxes,count,incoming,chosen);
    }
    // Retained reference/fallback. Do not change rounding or tie ordering when
    // widening the eligibility of the integer path below.
    static bool chooseBinary32Child(const Box* boxes,unsigned count,Box incoming,unsigned& chosen){
        DW_PROFILE_SCOPE(profileSpatialChooseFallback);
        if(!count || count>6 || !valid(incoming))return false;
        unsigned best=0;Real bestGrowth{},bestArea{};
        for(unsigned i=0;i<count;++i){
            Real oldArea,growth;
            if(!valid(boxes[i]) || !area(boxes[i],oldArea) ||
               !area(unite(boxes[i],incoming),growth) || !sub(growth,oldArea,growth))return false;
            if(!i || less(growth,bestGrowth) || (equal(growth,bestGrowth)&&less(oldArea,bestArea))){
                best=i;bestGrowth=growth;bestArea=oldArea;
            }
        }
        chosen=best;return true;
    }
    // Eligible pixel rectangles have exact integer extents and each candidate
    // union area <=2^24. Thus each binary32 subtraction/product and growth
    // subtraction in child selection is exact; ties remain earliest-slot.
    // A false result means use the reference, not that the query is invalid.
    static bool choosePixelChild(const Box* boxes,unsigned count,Box incoming,unsigned& chosen){
        if(!count||count>6)return false;
        PixelBox in;
        if(!pixels(incoming,in))return false;
        return chooseDecodedChild(boxes,count,in,chosen);
    }
private:
    static bool chooseDecodedChild(const Box* boxes,unsigned count,const PixelBox& in,unsigned& chosen){
        if(!count||count>6)return false;
        unsigned best=0,bestArea=0,bestGrowth=0;
        for(unsigned i=0;i<count;++i){
            PixelBox value;
            if(!pixels(boxes[i],value))return false;
            const PixelBox joined=pixelUnion(value,in);
            const unsigned width=unsigned(joined.right-joined.left),height=unsigned(joined.bottom-joined.top);
            if(width>65535||height>65535)return false;
            const unsigned enlarged=pixelProduct(width,height);
            if(enlarged>0x1000000u)return false;
            // The candidate is contained in its union, so its extents and
            // area satisfy the same exactness bounds without another check.
            const unsigned area=pixelArea(value),growth=enlarged-area;
            if(!i||growth<bestGrowth||(growth==bestGrowth&&area<bestArea)){
                best=i;bestArea=area;bestGrowth=growth;
            }
        }
        chosen=best;return true;
    }
public:
    static bool partitionPixels(const Box (&boxes)[7],Result& output){
        PixelBox values[7],all;unsigned areas[7];
        for(unsigned i=0;i<7;++i){
            if(!pixels(boxes[i],values[i]))return false;
            all=i?pixelUnion(all,values[i]):values[i];
        }
        const unsigned width=unsigned(all.right-all.left),height=unsigned(all.bottom-all.top);
        if(width>65535||height>65535||pixelProduct(width,height)>0x1000000u)return false;
        for(unsigned i=0;i<7;++i)areas[i]=pixelArea(values[i]);
        unsigned first=0,second=1;int best=0;bool unset=true;
        for(unsigned i=0;i<6;++i)for(unsigned j=i+1;j<7;++j){
            const int waste=int(pixelArea(pixelUnion(values[i],values[j])))-int(areas[i])-int(areas[j]);
            // The source seed is -wholeArea-1 (rounded); every candidate is
            // >=-wholeArea. Its unset equality clause always admits this first pair.
            if(unset||best<waste){first=i;second=j;best=waste;unset=false;}
        }
        Result next;for(auto& group:next.group)group=2;
        PixelBox bounds[2]={values[first],values[second]};
        unsigned groupArea[2]={areas[first],areas[second]};
        next.group[first]=0;next.group[second]=1;next.count[0]=next.count[1]=1;
        unsigned assigned=2;
        while(assigned<7&&next.count[0]<5&&next.count[1]<5){
            int largest=-1;unsigned chosen=0,group=0;
            for(unsigned i=0;i<7;++i)if(next.group[i]==2){
                const unsigned growth0=pixelArea(pixelUnion(bounds[0],values[i]))-groupArea[0];
                const unsigned growth1=pixelArea(pixelUnion(bounds[1],values[i]))-groupArea[1];
                const int difference=int(growth1)-int(growth0);
                const unsigned preferred=difference<0?1:0;
                const int magnitude=difference<0?-difference:difference;
                if(largest<magnitude||(largest==magnitude&&next.count[preferred]<next.count[group])){
                    largest=magnitude;chosen=i;group=preferred;
                }
            }
            next.group[chosen]=group;++next.count[group];++assigned;
            bounds[group]=pixelUnion(bounds[group],values[chosen]);groupArea[group]=pixelArea(bounds[group]);
        }
        const unsigned remainder=next.count[0]>=5?1:0;
        for(unsigned i=0;i<7;++i)if(next.group[i]==2){next.group[i]=remainder;++next.count[remainder];}
        output=next;return true;
    }
private:
    static bool pixel(Real value,std::int32_t& out){
        const unsigned bits=value.bits,magnitude=bits&0x7fffffffu;
        if(!magnitude){out=0;return true;}
        const unsigned exponent=(bits>>23)&255u;
        if(exponent<127||exponent>150)return false;
        const unsigned shift=150-exponent,mantissa=(bits&0x7fffffu)|0x800000u;
        if(mantissa&((1u<<shift)-1))return false;
        const auto integer=std::int32_t(mantissa>>shift);
        out=bits&0x80000000u?-integer:integer;return true;
    }
    static bool pixels(const Box& value,PixelBox& out){
        return pixel(value.left,out.left)&&pixel(value.top,out.top)&&
            pixel(value.right,out.right)&&pixel(value.bottom,out.bottom)&&
            out.left<=out.right&&out.top<=out.bottom;
    }
    static PixelBox pixelUnion(const PixelBox& a,const PixelBox& b){
        return {a.left<b.left?a.left:b.left,a.top<b.top?a.top:b.top,
            a.right>b.right?a.right:b.right,a.bottom>b.bottom?a.bottom:b.bottom};
    }
    static unsigned pixelProduct(unsigned width,unsigned height){
        return std::uint32_t(std::uint16_t(width))*std::uint32_t(std::uint16_t(height));
    }
    static unsigned pixelArea(const PixelBox& box){
        return pixelProduct(unsigned(box.right-box.left),unsigned(box.bottom-box.top));
    }
public:
    // A descent borrows one immutable incoming box until all child selections
    // finish. Candidate bounds stay live; no decoded tree state is retained.
    class ChildSelection {
        const Box& incoming_;
        PixelBox pixel_{};
        bool pixelValid_;
    public:
        explicit ChildSelection(const Box& incoming):incoming_(incoming),pixelValid_(pixels(incoming,pixel_)){}
        bool choose(const Box* boxes,unsigned count,unsigned& chosen)const{
            DW_PROFILE_SCOPE(profileSpatialChoose);
            if(pixelValid_&&chooseDecodedChild(boxes,count,pixel_,chosen))return true;
            return chooseBinary32Child(boxes,count,incoming_,chosen);
        }
    };
    // Closed bounds: touching edges survive broad-phase pruning.
    static bool overlaps(const Box& a,const Box& b){return !less(a.right,b.left)&&!less(b.right,a.left)&&
        !less(a.bottom,b.top)&&!less(b.bottom,a.top);}
    static bool valid(Real x){const unsigned e=(x.bits>>23)&255;return e!=255 && (e || !(x.bits&0x7fffffff));}
    static bool valid(const Box& b){return valid(b.left)&&valid(b.top)&&valid(b.right)&&valid(b.bottom)&&!less(b.right,b.left)&&!less(b.bottom,b.top);}
    static bool equal(Real a,Real b){return a.bits==b.bits || (!(a.bits&0x7fffffff)&&!(b.bits&0x7fffffff));}
    static bool less(Real a,Real b){
        if(equal(a,b))return false;
        if((a.bits^b.bits)&0x80000000u)return (a.bits>>31)!=0;
        return a.bits&0x80000000u?a.bits>b.bits:a.bits<b.bits;
    }
    static Real negate(Real a){a.bits^=0x80000000u;return a;}
    static bool sub(Real a,Real b,Real& out){return subtractBinary32(a,b,out);}
    static Box unite(const Box& a,const Box& b){return {less(a.left,b.left)?a.left:b.left,less(a.top,b.top)?a.top:b.top,
        less(b.right,a.right)?a.right:b.right,less(b.bottom,a.bottom)?a.bottom:b.bottom};}
    static bool area(const Box& b,Real& out){
        Real w,h;
        return sub(b.right,b.left,w)&&sub(b.bottom,b.top,h)&&
            multiplyBinary32(w,h,out);
    }
};
}
#endif
