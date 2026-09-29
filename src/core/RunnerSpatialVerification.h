#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_VERIFICATION_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_VERIFICATION_H
#include "RunnerSpatialInstances.h"
#ifndef DOWNWELL_PLATFORM_AMIGA
#include <new>
#endif
namespace downwell {
// CPU-independent mutation tape. Expected flags/queue membership are checked
// separately from the digest; equal host/target failures must never pass.
// Does not claim live scheduler integration or original-runner execution.
struct RunnerSpatialVerification {
    unsigned checks=0,failures=0;
    std::uint32_t digest=2166136261u;
    void check(bool value){++checks;if(!value)++failures;}
    void word(std::uint32_t value){digest=(digest^value)*16777619u;}
};
// Keep this fixture's sizeable owner scratch off the small target task stack.
// Placement construction also avoids unsupported guarded-static runtime calls.
[[gnu::noinline]] inline RunnerSpatialVerification verifyRunnerSpatialMutations(){
    RunnerSpatialVerification result;
    {
        const std::uint32_t bits[]={0u,0x80000000u,1u,0x80000001u,0x00800000u,0x80800000u,
            0x3f800000u,0xbf800000u,0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,0x7fc00000u};
        for(auto a:bits)for(auto b:bits){
            const bool equal=a==b||(!(a&0x7fffffffu)&&!(b&0x7fffffffu));
            const bool expected=!equal&&((a^b)&0x80000000u?bool(a>>31):a&0x80000000u?a>b:a<b);
            result.check(RunnerSpatialSplit::less({a},{b})==expected);
        }
    }
    using Owner=RunnerSpatialInstances<2,8>;
    alignas(Owner) static std::uint8_t storage[sizeof(Owner)];
    const InstanceHandle h{0,1};
    const std::uint32_t values[]={0u,0x80000000u,0x3f800000u,0x40000000u,0xbf800000u};
    for(unsigned axis=0;axis<3;++axis)for(auto bits:values)
    for(unsigned tinted=0;tinted<2;++tinted)for(unsigned faded=0;faded<2;++faded){
        auto& owner=*new (storage) Owner;Owner::State clean;clean.flags=0;
        result.check(owner.attach(h,1,clean));
        Owner::Sprite sprite;
        auto& target=axis==0?sprite.scaleX:axis==1?sprite.scaleY:sprite.angle;
        target={0x40000000u};
        const auto field=axis==0?Owner::TransformField::scaleX:
            axis==1?Owner::TransformField::scaleY:Owner::TransformField::angle;
        const std::uint32_t alpha=faded?0x3f000000u:0x3f800000u;
        result.check(owner.changeTransform(h,sprite,field,{bits},{alpha},tinted?0xff0000u:0xffffffu,0x38));
        const bool changed=bits!=0x40000000u;
        const bool fast=sprite.scaleX.bits==0x3f800000u&&sprite.scaleY.bits==0x3f800000u&&
            !(sprite.angle.bits&0x7fffffffu)&&!tinted&&!faded;
        const auto* state=owner.resolve(h);
        result.check(state&&target.bits==bits&&state->flags==(changed?(8u|(fast?0x4000u:0u)):0u));
        result.check(owner.pendingSpatial()==unsigned(changed)&&owner.pendingCollision()==unsigned(changed));
        result.word(target.bits);result.word(state?state->flags:0xffffffffu);
        result.word(owner.pendingSpatial());result.word(owner.pendingCollision());
    }
    for(unsigned objectFlags=0;objectFlags<256;++objectFlags)for(unsigned prepared=0;prepared<2;++prepared){
        auto& owner=*new (storage) Owner;Owner::State state;state.flags=0x200070u;
        result.check(owner.attach(h,1,state));
        const Owner::Box box{};
        result.check(owner.refresh(h,0x38,false,&box,false,true));
        result.check(owner.move(h,{0x3f800000u},{},0x38));
        unsigned expected=owner.resolve(h)->flags;
        const bool remove=!(objectFlags&1u)&&!prepared&&!(objectFlags&0x28u);
        if(objectFlags&1u)expected|=0x28u;else expected&=~0x20u;
        if(remove)expected&=~0x8000u;
        expected=(expected&~0x50u)|((objectFlags&2u)?0x10u:0u)|((objectFlags&4u)?0x40u:0u);
        unsigned callbacks=0;
        result.check(owner.applyBindingProperties(h,objectFlags,prepared,[&](InstanceHandle){
            ++callbacks;
            result.check(owner.resolve(h)->flags==expected);
            result.check(owner.pendingSpatial()==(remove?0u:1u));
            return true;
        }));
        expected=(expected&~0x200000u)|((objectFlags&0x80u)?0x200000u:0u)|8u;
        result.check(callbacks==1&&owner.resolve(h)->flags==expected);
        result.check(owner.pendingCollision()==(remove?0u:1u));
        result.word(owner.resolve(h)->flags);result.word(owner.pendingSpatial());result.word(owner.pendingCollision());
        result.check(owner.retire(h)&&!owner.resolve(h)&&!owner.pendingSpatial()&&!owner.pendingCollision());
        result.check(owner.attach({0,2},1,{})&&!owner.resolve(h));
    }
    // Exact pixel-path decisions are compared with the retained binary32
    // calculation on BOTH CPUs, independently of the final replay digest.
    std::uint32_t seed=0x19537u;
    for(unsigned trial=0;trial<64;++trial){
        RunnerSpatialSplit::Box boxes[7];
        for(auto& box:boxes){
            seed=seed*1664525u+1013904223u;
            const int x=int((seed>>8)&2047u)-1024,y=int((seed>>19)&4095u);
            result.check(box.left.assign(MotionBinary64::fromInteger(x))&&
                box.top.assign(MotionBinary64::fromInteger(y))&&
                box.right.assign(MotionBinary64::fromInteger(x+int(seed&127u)))&&
                box.bottom.assign(MotionBinary64::fromInteger(y+int((seed>>7)&31u))));
        }
        const unsigned count=1+trial%6;unsigned expected=99,fast=99,actual=99;
        result.check(RunnerSpatialSplit::chooseBinary32Child(boxes,count,boxes[6],expected));
        result.check(RunnerSpatialSplit::choosePixelChild(boxes,count,boxes[6],fast)&&fast==expected);
        result.check(RunnerSpatialSplit::chooseChild(boxes,count,boxes[6],actual)&&actual==expected);
        const RunnerSpatialSplit::ChildSelection selection(boxes[6]);
        actual=99;result.check(selection.choose(boxes,count,actual)&&actual==expected);
        struct Strided {unsigned prefix;RunnerSpatialSplit::Box box;unsigned suffix;};
        Strided entries[6]{};
        for(unsigned i=0;i<count;++i)entries[i]={i,boxes[i],~i};
        actual=99;result.check(selection.chooseFrom([&](unsigned i)->const RunnerSpatialSplit::Box&{
            return entries[i].box;
        },count,actual)&&actual==expected);
        result.word(actual);
        if(trial<16){
            RunnerSpatialSplit::Result reference,partition,pixelPartition;
            result.check(RunnerSpatialSplit::partitionBinary32(boxes,reference));
            result.check(RunnerSpatialSplit::partitionPixels(boxes,pixelPartition));
            result.check(RunnerSpatialSplit::partition(boxes,partition));
            for(unsigned i=0;i<7;++i){
                result.check(partition.group[i]==reference.group[i]&&pixelPartition.group[i]==reference.group[i]);
                result.word(partition.group[i]);
            }
            for(unsigned i=0;i<2;++i)result.check(partition.count[i]==reference.count[i]&&pixelPartition.count[i]==reference.count[i]);
        }
        boxes[0].left.bits=0x3f000000u; // Fractional coordinate must use fallback.
        fast=99;
        result.check(!RunnerSpatialSplit::choosePixelChild(boxes,count,boxes[6],fast)&&fast==99);
        expected=actual=99;
        const bool fallback=RunnerSpatialSplit::chooseBinary32Child(boxes,count,boxes[6],expected);
        result.check(selection.choose(boxes,count,actual)==fallback&&actual==expected);
        entries[0].box=boxes[0];actual=99;
        result.check(selection.chooseFrom([&](unsigned i)->const RunnerSpatialSplit::Box&{
            return entries[i].box;
        },count,actual)==fallback&&actual==expected);
        auto fractional=boxes[6];fractional.left.bits=0x3f000000u;
        const RunnerSpatialSplit::ChildSelection fractionalSelection(fractional);
        expected=actual=99;
        const bool fractionalFallback=RunnerSpatialSplit::chooseBinary32Child(boxes,count,fractional,expected);
        result.check(fractionalSelection.choose(boxes,count,actual)==fractionalFallback&&actual==expected);
    }
    {
        RunnerSpatialSplit::Box boxes[2],incoming;
        auto assign=[](auto& box,int left,int top,int right,int bottom){
            return box.left.assign(MotionBinary64::fromInteger(left))&&
                box.top.assign(MotionBinary64::fromInteger(top))&&
                box.right.assign(MotionBinary64::fromInteger(right))&&
                box.bottom.assign(MotionBinary64::fromInteger(bottom));
        };
        result.check(assign(boxes[0],-4000,0,0,4000)&&assign(boxes[1],0,0,4000,4000)&&assign(incoming,0,0,0,4000));
        unsigned expected=99,actual=99;
        result.check(RunnerSpatialSplit::chooseBinary32Child(boxes,2,incoming,expected));
        result.check(RunnerSpatialSplit::choosePixelChild(boxes,2,incoming,actual)&&actual==expected);
        result.word(actual);
        result.check(assign(boxes[1],0,0,4097,4096));actual=99;
        result.check(!RunnerSpatialSplit::choosePixelChild(boxes,2,incoming,actual)&&actual==99);
    }
    {
        using Math=RunnerSpatialSplit;
        const Math::Box a{{0xbf800000u},{0xc0000000u},{0x40000000u},{0x40400000u}};
        const Math::Box b{{0},{0xc0400000u},{0x40800000u},{0x40000000u}};
        auto same=[](const auto& x,const auto& y){return x.left.bits==y.left.bits&&
            x.top.bits==y.top.bits&&x.right.bits==y.right.bits&&x.bottom.bits==y.bottom.bits;};
        const Math::Box expected{{0xbf800000u},{0xc0400000u},{0x40800000u},{0x40400000u}};
        auto first=a,second=b;
        first=Math::unite(first,b);second=Math::unite(a,second);
        result.check(same(first,expected)&&same(second,expected));
        result.check(Math::overlaps(a,b)&&Math::valid(a)&&Math::valid(b));
        for(unsigned edge=0;edge<4;++edge){
            auto box=a;
            auto& out=edge==0?box.left:edge==1?box.top:edge==2?box.right:box.bottom;
            result.check(Math::area(box,out)&&out.bits==0x41700000u); // 3 * 5 = 15.
        }
        auto invalid=a;invalid.left.bits=0x7fc00000u;
        result.check(!Math::area(invalid,invalid.left)&&invalid.left.bits==0x7fc00000u);
    }
    // Compare the integer-backed owner to forced original arithmetic across
    // populated-tree representation changes and reuse of released nodes.
    {
        using Fast=RunnerSpatialIndex<64>;
        using Reference=RunnerSpatialIndex<64,false>;
        alignas(Fast) static std::uint8_t fastStorage[sizeof(Fast)];
        alignas(Reference) static std::uint8_t referenceStorage[sizeof(Reference)];
        const RunnerSpatialSplit::Box triggers[]={
            {{0x3f000000u},{0xbf000000u},{0x43008000u},{0x43008000u}},
            {{0x80000000u},{0},{0},{0}},
            {{0x4b800000u},{0},{0x4b800000u},{0x41800000u}}
        };
        for(const auto& trigger:triggers){
            auto& fast=*new(fastStorage) Fast;
            auto& reference=*new(referenceStorage) Reference;
            RunnerSpatialSplit::Box boxes[32];
            auto compare=[&](RunnerSpatialSplit::Box query){
                unsigned a[40],b[40],na=0,nb=0;
                result.check(fast.query(query,[&](unsigned id){if(na<40)a[na++]=id;return na<40;}));
                result.check(reference.query(query,[&](unsigned id){if(nb<40)b[nb++]=id;return nb<40;}));
                result.check(na==nb&&fast.height()==reference.height()&&fast.nodeCount()==reference.nodeCount());
                for(unsigned i=0;i<na&&i<nb;++i){result.check(a[i]==b[i]);result.word(a[i]);}
            };
            for(unsigned id=0;id<32;++id){
                const int x=int(id%8)*16,y=int(id/8)*16;
                boxes[id]=RunnerSpatialSplit::encodePixels({x,y,x+16,y+16});
                result.check(fast.insert(boxes[id],id)&&reference.insert(boxes[id],id));
            }
            // Fractional reads do not mutate integer storage.
            compare(triggers[0]);
            for(unsigned id=0;id<16;++id){
                bool a=false,b=false;
                result.check(fast.remove(boxes[id],id,false,a)&&reference.remove(boxes[id],id,false,b)&&a&&b);
            }
            result.check(fast.insert(trigger,32)&&reference.insert(trigger,32));
            for(unsigned id=0;id<16;++id)
                result.check(fast.insert(boxes[id],id)&&reference.insert(boxes[id],id));
            compare(RunnerSpatialSplit::encodePixels({-1,-1,256,256}));compare(trigger);
            for(const auto& box:boxes)compare(box);
            fast.clear();reference.clear();
            result.check(fast.insert(boxes[0],0)&&reference.insert(boxes[0],0));compare(boxes[0]);
        }
    }
    {
        // Family selection must not expand unrelated terrain runs. Compare
        // both CPUs against scalar registration, including fragmentation,
        // re-registration, modulo identities and callback stop prefixes.
        using Runs=RunnerRunMembership<64,0,64>;
        using Scalar=RunnerObjectMembership<64>;
        alignas(Runs) static std::uint8_t runStorage[sizeof(Runs)];
        alignas(Scalar) static std::uint8_t scalarStorage[sizeof(Scalar)];
        auto& runs=*new (runStorage) Runs;
        auto& scalar=*new (scalarStorage) Scalar;
        struct Hit {unsigned slot;std::uint32_t identity;};
        static Hit expected[64];
        const int selectors[]={-1,0,1,60,93,99};
        auto isA=[](int child,int parent){return parent==0&&(child==60||child==93);};
        for(unsigned phase=0;phase<4;++phase){
            if(!phase){
                for(unsigned s=0;s<64;++s)
                    result.check(runs.bind(s,s<32?60:93,0xffffffe0u+s)&&
                        scalar.bind(s,s<32?60:93,0xffffffe0u+s));
            }else if(phase==1){
                for(unsigned s=0;s<64;s+=3)result.check(runs.unbind(s)&&scalar.unbind(s));
            }else if(phase==2){
                for(unsigned s=0;s<64;s+=3)
                    result.check(runs.bind(s,s&1?1:60,100+s)&&scalar.bind(s,s&1?1:60,100+s));
            }else{
                runs.clear();scalar.clear();
                for(unsigned s=64;s>0;--s)
                    result.check(runs.bind(s-1,93,200+s)&&scalar.bind(s-1,93,200+s));
            }
            for(int selector:selectors){
                const unsigned count=scalar.count(selector,isA);
                result.check(runs.count(selector,isA)==count&&runs.countReference(selector,isA)==count);
                result.word(count);
                for(unsigned stop=1;stop<=65;stop+=8){
                    unsigned referenceCount=0,actualCount=0;
                    scalar.visit(selector,isA,[&](unsigned slot,std::uint32_t identity){
                        expected[referenceCount++]={slot,identity};return referenceCount<stop;
                    });
                    runs.visit(selector,isA,[&](unsigned slot,std::uint32_t identity){
                        if(actualCount>=referenceCount){result.check(false);return false;}
                        const auto& hit=expected[actualCount++];
                        result.check(slot==hit.slot&&identity==hit.identity);
                        result.word(slot);result.word(identity);return actualCount<stop;
                    });
                    result.check(actualCount==referenceCount);
                }
            }
        }
    }
    return result;
}
}
#endif
