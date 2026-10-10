#include "core/OrdinaryTimeField.h"
#include "core/OrdinaryTimeFields.h"
#include "core/GoalTimeField.h"
#include "core/DeferredInstanceQueue.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using namespace downwell;
static double value(HudReal n){return std::ldexp(double(n.mantissa),n.exponent-52);}
int main(){
    struct Audit {
        OrdinaryTimeFields<2>* fields=nullptr;GameMakerRandom* random=nullptr;
        unsigned births=0,retires=0,failBirth=0,failRetire=0;
        std::uint32_t drawsAtBirth=0;
        std::uint32_t sourceAtBirth=0;
        bool birth(InstanceHandle h,const OrdinaryTimeField& r){
            assert(fields->find(h.generation)==&r&&fields->identityAt(h.slot)==h.generation&&r.radius.mantissa==0);
            sourceAtBirth=r.sourceId;drawsAtBirth=random->drawCount();return ++births!=failBirth;
        }
        bool retire(InstanceHandle h,const OrdinaryTimeField& r){
            assert(fields->find(h.generation)==&r);return ++retires!=failRetire;
        }
        bool activation(InstanceHandle,const OrdinaryTimeField&,bool){return true;}
        bool membership(InstanceHandle,const OrdinaryTimeField&,bool){return true;}
    };
    for(bool reject:{false,true}){
        OrdinaryTimeFields<2> fields;GameMakerRandom random(42);Audit audit{&fields,&random};
        audit.failBirth=reject?1:0;
        assert(fields.bindLifecycle(OrdinaryTimeFields<2>::Binding::bind(audit)));
        assert(fields.createWithSourceId(210001,240,512,random)==!reject);
        assert(audit.sourceAtBirth==210001&&audit.drawsAtBirth==0&&fields.at(0)->sourceId==210001&&
            random.drawCount()==unsigned(!reject));
        const auto draws=random.drawCount();
        assert(!fields.createWithSourceId(210001,241,512,random)&&fields.size()==1&&audit.births==1&&
            random.drawCount()==draws&&!fields.valid());
    }
    for(bool reject:{false,true}){
        OrdinaryTimeFields<2> fields;GameMakerRandom random(42);Audit audit{&fields,&random};
        audit.failBirth=reject?1:0;
        assert(fields.bindLifecycle(OrdinaryTimeFields<2>::Binding::bind(audit)));
        const auto word=random.nextU32();
        assert(fields.adoptCreate(240,512,word,210007)==!reject);
        assert(audit.sourceAtBirth==210007&&audit.drawsAtBirth==1&&random.drawCount()==1);
        assert(fields.at(0)->sourceId==210007);
        if(!reject)assert(fields.at(0)->radius.equal(OrdinaryTimeField::radiusFromWord(word)));
        assert(!fields.adoptCreate(256,600,word,210007)&&fields.size()==1&&!fields.valid());
    }
    for(auto invalid:{0u,0x80000000u}){
        OrdinaryTimeFields<2> fields;GameMakerRandom random(42);
        assert(!fields.createWithSourceId(invalid,240,512,random)&&!fields.size()&&!random.drawCount()&&!fields.valid());
    }
    {
        OrdinaryTimeFields<2> fields;GameMakerRandom random(42);Audit audit{&fields,&random};
        assert(fields.bindLifecycle(OrdinaryTimeFields<2>::Binding::bind(audit)));
        const auto draws=random.drawCount();
        assert(fields.create(240,512,random)&&audit.births==1&&audit.drawsAtBirth==draws&&random.drawCount()==draws+1);
        assert(fields.adoptCreate(256,600,123)&&audit.births==2&&random.drawCount()==draws+1);
        const auto first=fields.identityAt(0),second=fields.identityAt(1);
        audit.failRetire=2;
        assert(!fields.clear()&&!fields.valid()&&!fields.find(first)&&fields.find(second)&&audit.retires==2);
        assert(!fields.clear()&&!fields.create(1,2,random)&&random.drawCount()==draws+1);
    }
    {
        OrdinaryTimeFields<2> fields;GameMakerRandom random(42);Audit audit{&fields,&random};audit.failBirth=1;
        assert(fields.bindLifecycle(OrdinaryTimeFields<2>::Binding::bind(audit)));
        const auto draws=random.drawCount();
        assert(!fields.create(240,512,random)&&!fields.valid()&&fields.find(1)&&random.drawCount()==draws);
        assert(!fields.create(256,600,random)&&!fields.restoreEmptyBirthSequence({1})&&audit.births==1);
    }
    for(unsigned seed=0;seed<256;++seed){
        OrdinaryTimeFields<2> fields;
        GameMakerRandom actual(seed),expected(seed);
        assert(fields.create(-16,512,actual));
        assert(fields.at(0)->x==-16 && fields.at(0)->y==512);
        assert(fields.at(0)->radius.equal(OrdinaryTimeField::radiusFromWord(expected.nextU32())));
        const auto first=fields.identityAt(0);
        assert(fields.adoptCreate(240,600,17)); // Adoption must not sample again.
        assert(fields.identityAt(1)==first+1);
        assert(!fields.create(0,0,actual) && fields.size()==2);
        fields.clear();
        assert(!fields.find(first));
        assert(fields.restoreEmptyBirthSequence({0xffffffffu}));
        assert(fields.create(240,700,actual) && fields.identityAt(0)==0xffffffffu);
        assert(fields.at(0)->radius.equal(OrdinaryTimeField::radiusFromWord(expected.nextU32())));
        // A free storage slot cannot revive an exhausted identity sequence.
        assert(!fields.create(0,0,actual) && fields.size()==1);
        fields.clear();assert(!fields.create(0,0,actual));
        assert(actual.drawCount()==2 && actual.stateIndex()==expected.stateIndex());
        for(unsigned i=0;i<16;++i)assert(actual.stateWord(i)==expected.stateWord(i));
    }
    {
        OrdinaryTimeFields<2> fields;
        std::uint32_t previous=0;
        for(unsigned room=0;room<1024;++room){
            assert(fields.adoptCreate(240,512,room));
            auto key=fields.identityAt(0);
            assert(key>previous && fields.find(key)==fields.at(0));
            assert(!fields.find(previous) && !fields.find(0) && !fields.identityAt(2));
            assert(fields.adoptCreate(240,600,room+1));
            assert(fields.identityAt(1)==key+1 && !fields.adoptCreate(0,0,0));
            assert(!fields.restoreEmptyBirthSequence({1}));
            auto copy=fields;
            assert(copy.find(key)==copy.at(0) && copy.find(key)!=fields.find(key));
            previous=fields.identityAt(1);
            const auto sequence=fields.birthSequence();
            fields.clear();
            assert(!fields.find(key) && !fields.find(previous));
            assert(!fields.restoreEmptyBirthSequence({1}));
            // Same cursor must work for room retirement and fresh-owner Retry.
            if(room&1){fields=OrdinaryTimeFields<2>{};assert(fields.restoreEmptyBirthSequence(sequence));}
        }
        fields.clear();
        assert(fields.restoreEmptyBirthSequence({0xffffffffu}));
        assert(fields.adoptCreate(240,512,0) && fields.identityAt(0)==0xffffffffu);
        assert(fields.find(0xffffffffu)==fields.at(0) && !fields.adoptCreate(0,0,0));
        auto exhausted=fields.birthSequence();fields.clear();
        assert(!fields.restoreEmptyBirthSequence({1}));
        assert(!fields.find(0xffffffffu) && !fields.adoptCreate(0,0,0));
        assert(fields.restoreEmptyBirthSequence(exhausted) && !fields.adoptCreate(0,0,0));
    }
    for(unsigned seed=0;seed<64;++seed)for(unsigned boundary=0;boundary<=5;++boundary){
        OrdinaryTimeFields<5> fields;GoalTimeField goal;
        GameMakerRandom actual(seed),expected(seed);
        for(unsigned i=0;i<5;++i)assert(fields.adoptCreate(240,int(i),i));
        unsigned calls=0;
        assert(fields.advanceDraw(false,actual,true,boundary,[&]{++calls;goal.advanceDraw(false,actual);}));
        for(unsigned i=5;i>0;--i){
            if(i==boundary)assert(value(goal.radius)==128+double(expected.nextU32())/2147483648.0);
            assert(value(fields.at(i-1)->radius)==24+double(expected.nextU32())/2147483648.0);
        }
        if(boundary==0)assert(value(goal.radius)==128+double(expected.nextU32())/2147483648.0);
        assert(calls==1 && actual.drawCount()==expected.drawCount() && actual.stateIndex()==expected.stateIndex());
        for(unsigned i=0;i<16;++i)assert(actual.stateWord(i)==expected.stateWord(i));
        const auto goalRadius=goal.radius;
        assert(fields.advanceDraw(true,actual,true,boundary,[&]{goal.advanceDraw(true,actual);}));
        assert(actual.drawCount()==6 && goal.radius.equal(goalRadius));
        assert(!fields.advanceDraw(false,actual,true,6,[&]{assert(false);}));
        assert(actual.drawCount()==6);
        assert(fields.advanceDraw(false,actual,false,999,[&]{assert(false);}));
        assert(actual.drawCount()==11);
        fields.clear();assert(fields.advanceDraw(false,actual,true,0,[&]{goal.advanceDraw(false,actual);}));
        assert(actual.drawCount()==12);
    }
    GameMakerRandom rng(42),reference(42);
    for(unsigned i=0;i<65536;++i){
        const auto n=OrdinaryTimeField::nextRadius(rng);
        assert(value(n)==24+double(reference.nextU32())/2147483648.0);
    }
    assert(rng.drawCount()==65536);
    for(std::uint32_t word:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){
        DeferredCreateInstance command(82,240,512);
        command.setTimeFieldRadiusWord(word);
        assert(command.timeFieldRadiusWord()==word);
        OrdinaryTimeField adopted;
        adopted.adoptCreate(command.x,command.y,command.timeFieldRadiusWord());
        assert(value(adopted.radius)==24+double(word)/2147483648.0);
        assert(rng.drawCount()==65536);
    }
    OrdinaryTimeField field;field.create(240,512,rng);
    const auto radius=field.radius;
    const auto draws=rng.drawCount();
    field.advanceDraw(true,rng);
    assert(rng.drawCount()==draws && value(field.radius)==value(radius));
    unsigned calls=0;
    auto circle=[&](int x,int y,HudReal r,bool red){
        assert(x==240 && y==512);
        assert(value(r)==value(field.radius)+(calls==0?0:calls==1?-1:1));
        assert(red==(calls==2));++calls;
    };
    field.drawSnapshot(160,480,circle);field.drawSnapshot(320,480,circle);
    assert(calls==0 && rng.drawCount()==draws);
    field.advanceDraw(false,rng); // Offscreen Draw still consumes its random sample.
    assert(rng.drawCount()==draws+1);
    field.drawSnapshot(240,480,circle);
    assert(calls==3 && rng.drawCount()==draws+1);
    std::ifstream file("generated/downwell-time-field-mask.mask",std::ios::binary);
    const std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});
    assert(raw.size()==288);
    unsigned checked=0,hits=0;
    for(int y=-40;y<=40;++y)for(int x=-40;x<=40;++x){
        bool expected=false;
        for(int py=y-4;py<y+8;++py)for(int px=x-4;px<x+4;++px){
            if(px < -24 || px>=24 || py < -24 || py>=24)continue;
            const int sx=px+24,sy=py+24;
            expected|=(raw[sy*6+sx/8]&(128>>(sx%8)))!=0;
        }
        const bool actual=OrdinaryTimeField::meetsRelativeBounds({x-4,x+4,y-4,y+8});
        assert(actual==expected);++checked;hits+=actual;
    }
    assert(hits && hits<checked);
    assert(OrdinaryTimeField::meetsRelativeBounds({-2147483647,2147483647,-2147483647,2147483647}));
    assert(!OrdinaryTimeField::meetsRelativeBounds({0,0,0,1}));
    assert(!OrdinaryTimeField::meetsRelativeBounds({-2147483647,-2147483646,0,1}));
    std::printf("Ordinary time field: 384 merged Draw orders, 65536 exact radius draws, frozen/offscreen Draw rules, %u mask comparisons\n",checked);
}
