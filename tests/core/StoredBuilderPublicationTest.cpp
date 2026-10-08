#include "core/StoredBuilderPublication.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
struct Owner {
    unsigned masks=0,images=0,activations=0;bool rejectMask=false,rejectImage=false,rejectActivation=false;
    template<class... A> bool birth(A...){return true;}
    template<class... A> bool retire(A...){return true;}
    template<class... A> bool membership(A...){return true;}
    template<class... A> bool move(A...){return true;}
    template<class... A> bool sprite(A...){return true;}
    template<class... A> bool transform(A...){return true;}
    bool mask(InstanceHandle,const InstanceRecord& r,int next){
        assert(r.maskId!=next);++masks;return !rejectMask;
    }
    bool imageIndex(InstanceHandle,const InstanceRecord& r,RunnerImageValue next){
        assert(r.imageIndex==0&&next.bits==0x41700000u);++images;return !rejectImage;
    }
    bool activation(InstanceHandle,const InstanceRecord& r,bool active){
        assert(r.active&&!active);++activations;return !rejectActivation;
    }
};
int main(){
    for(unsigned rejected=0;rejected<3;++rejected){
        InstanceStore<2> records;Owner owner;bool good=true;
        assert(records.create(100,60,11,false,0)==CreateInstanceResult::success);
        const auto h=records.handleAt(0);auto& r=*records.resolve(h);
        StoredBuilderPublication<2> publication(records,InstanceLifecycleBinding::bindMasks(owner),good);
        assert(publication.write(h,BuilderChildField::deactivate,1)&&r.wall.deactivate&&r.active&&owner.activations==0);
        owner.rejectMask=rejected==1;owner.rejectImage=rejected==2;
        const bool ok=publication.write(h,BuilderChildField::maskIndex,594)&&
            publication.write(h,BuilderChildField::indexVariable,15)&&
            publication.write(h,BuilderChildField::imageIndex,15)&&
            publication.write(h,BuilderChildField::checked,1);
        assert(ok==(rejected==0)&&good==ok&&owner.masks==1);
        if(rejected==1)assert(r.maskId==-1&&r.wall.imageIndex==0&&owner.images==0);
        if(rejected==2)assert(r.maskId==594&&r.wall.imageIndex==15&&r.imageIndex==0&&r.wall.checked==0&&owner.images==1);
        if(!rejected){
            assert(r.maskId==594&&r.wall.imageIndex==15&&r.imageIndex==15&&r.wall.checked==1);
            bool right=true;r.x=240;r.preciseX=240.0F;
            assert(publication.rightOfMiddle(h,480,right)&&!right);
            r.preciseX=240.5F;assert(publication.rightOfMiddle(h,480,right)&&right);
            assert(publication.rightOfMiddle(h,481,right)&&!right);
            assert(publication.write(h,BuilderChildField::isPillar,-1)&&r.wall.pillar==-1);
            assert(records.erase(100));
            assert(records.create(101,60,11,false,0)==CreateInstanceResult::success);
            assert(!publication.write(h,BuilderChildField::checked,1)&&!good&&records.at(0).wall.checked==0);
        }else{
            const auto images=owner.images;
            assert(!publication.write(h,BuilderChildField::imageIndex,15)&&owner.images==images);
        }
    }
    for(bool reject:{false,true}){
        InstanceStore<1> records;Owner owner;bool good=true;owner.rejectActivation=reject;
        assert(records.create(200,92,11,false,0)==CreateInstanceResult::success);
        StoredBuilderPublication<1> publication(records,InstanceLifecycleBinding::bindMasks(owner),good);
        assert(publication.deactivate(records.handleAt(0))==!reject&&good==!reject);
        assert(records.at(0).active==reject&&records.at(0).activationPending==!reject&&owner.activations==1);
    }
    {
        InstanceStore<1> records;bool good=true;
        assert(records.create(300,60,11,false,0)==CreateInstanceResult::success);
        records.at(0).x=-16; // Legacy integer position with uninitialized preciseX.
        StoredBuilderPublication<1> publication(records,{},good);
        bool right=true;
        assert(!publication.rightOfMiddle(records.handleAt(0),480,right)&&!good&&right);
    }
    std::puts("Stored builder publication and failure-prefix checks passed");
}
