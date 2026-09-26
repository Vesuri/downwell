#ifndef DOWNWELL_CORE_RUNNER_MASK_PAIR_H
#define DOWNWELL_CORE_RUNNER_MASK_PAIR_H
#include "RunnerMaskDispatch.h"
#include "RunnerMaskTransform.h"
namespace downwell {
// Ordinary stored-mask pair kernel, 0x4b8eb0..0x4b96a6. The caller decides
// whether precise testing is required, resolves resources/normalized mask frames
// and refreshes cached bounds. This kernel does not replace those outer gates.
struct RunnerMaskPair {
    using Real=RunnerImageValue;
    using Math=RunnerSpatialSplit;
    using Path=RunnerMaskDispatch::Path;
    struct Shape {
        Real x{},y{};
        RunnerMaskDispatch::Shape transform{{0x3f800000},{0x3f800000},{}};
        Math::Box bounds{};
        int originX=0,originY=0;
        unsigned frameCount=0;
        SpriteMaskPack::Mask mask{};
    };
    // The runner returns only hit. First pixel is retained here for testing the
    // source's X-outer/Y-inner scan and diagnosing divergent mask coordinates.
    struct Result {bool hit=false;int x=0,y=0;};
    static bool storedFrame(Real imageIndex,unsigned count,unsigned& result){
        if(!Math::valid(imageIndex)||!count||count>0x7fffffffu)return false;
        int frame;
        if(!truncate(imageIndex,frame))return false;
        const int remainder=frame%int(count);
        result=unsigned(remainder<0?remainder+int(count):remainder);return true;
    }
    // inverseTrig(degrees,c,s) must supply source-narrowed inverse-rotation
    // cosine/sine. round implements the source's integer position rounding.
    // Unsupported arithmetic/assets leave result unchanged, never a false hit.
    template<class Trig,class Round>
    static bool test(const Shape& a,const Shape& b,const RunnerMaskDispatch& dispatch,
        Trig inverseTrig,Round round,Result& result){
        if(!a.frameCount||!b.frameCount){result={};return true;}
        const auto path=dispatch.select(a.transform,b.transform);
        if(path==Path::unsupported)return false;
        if(path==Path::empty){result={};return true;}
        const Real edges[]={a.bounds.left,a.bounds.top,a.bounds.right,a.bounds.bottom,
                            b.bounds.left,b.bounds.top,b.bounds.right,b.bounds.bottom};
        for(auto edge:edges)if(!Math::valid(edge))return false;
        int left,right,top,bottom;
        if(!truncate(max(a.bounds.left,b.bounds.left),left)||
           !truncate(min(a.bounds.right,b.bounds.right),right)||
           !truncate(max(a.bounds.top,b.bounds.top),top)||
           !truncate(min(a.bounds.bottom,b.bounds.bottom),bottom))return false;
        Sampler first,second;
        if(!first.prepare(a,path,inverseTrig,round)||!second.prepare(b,path,inverseTrig,round))return false;
        if(left<=right&&top<=bottom){
            int x=left;
            for(;;){
                int y=top;
                for(;;){
                    bool hitA=false,hitB=false;
                    if(!first.contains(a.mask,x,y,hitA))return false;
                    if(hitA){
                        if(!second.contains(b.mask,x,y,hitB))return false;
                        if(hitB){result={true,x,y};return true;}
                    }
                    if(y==bottom)break;
                    ++y;
                }
                if(x==right)break;
                ++x;
            }
        }
        result={};return true;
    }
private:
    static Real min(Real a,Real b){return Math::less(a,b)?a:b;}
    static Real max(Real a,Real b){return Math::less(a,b)?b:a;}
    static bool truncate(Real value,int& result){
        if(value.bits==0xcf000000u){
            result=-2147483647-1;return true;
        }
        const int exponent=int((value.bits>>23)&255)-127;
        if(exponent>=31)return false;
        if(exponent<0){result=0;return true;}
        const unsigned significand=(value.bits&0x7fffffu)|0x800000u;
        const int n=int(exponent<23?significand>>unsigned(23-exponent):significand<<unsigned(exponent-23));
        result=value.bits&0x80000000u?-n:n;return true;
    }
    struct Sampler {
        RunnerMaskTransform transform;
        bool rounded=false;
        std::uint32_t offsetX=0,offsetY=0;
        template<class Trig,class Round>
        bool prepare(const Shape& shape,Path path,Trig trig,Round round){
            if(!shape.mask.bytes||!shape.mask.width||!shape.mask.height||
               shape.mask.width>0x7fffffffu||shape.mask.height>0x7fffffffu||
               !Math::valid(shape.x)||!Math::valid(shape.y))return false;
            rounded=path==Path::roundedPosition;
            if(rounded){
                std::int32_t x,y;
                if(!round(shape.x,x)||!round(shape.y,y))return false;
                offsetX=std::uint32_t(shape.originX)-std::uint32_t(x);
                offsetY=std::uint32_t(shape.originY)-std::uint32_t(y);return true;
            }
            if(path==Path::scaled)return transform.prepareAxisAligned(shape.x,shape.y,
                shape.transform.scaleX,shape.transform.scaleY,shape.originX,shape.originY);
            Real c,s;
            return trig(shape.transform.angle,c,s)&&transform.prepare(shape.x,shape.y,
                shape.transform.scaleX,shape.transform.scaleY,c,s,shape.originX,shape.originY);
        }
        bool contains(const SpriteMaskPack::Mask& mask,int x,int y,bool& hit)const{
            if(!rounded)return transform.contains(mask,x,y,hit);
            const auto px=offsetX+std::uint32_t(x),py=offsetY+std::uint32_t(y);
            hit=px<mask.width&&py<mask.height&&mask.contains(int(px),int(py));return true;
        }
    };
};
}
#endif
