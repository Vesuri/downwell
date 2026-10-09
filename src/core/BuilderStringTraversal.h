#ifndef DOWNWELL_CORE_BUILDER_STRING_TRAVERSAL_H
#define DOWNWELL_CORE_BUILDER_STRING_TRAVERSAL_H
#include <cstdint>
namespace downwell {
// scrBuilder VM blocks 72--209, with scrSetBuildx. Execute the original
// immutable VM string, not a normalized rectangle. Selection and the common
// symbol switch belong to the caller. Callbacks are synchronous and checked:
// area(symbol,x,y), common(symbol,x,y), pillar(x,y), publishY(nextY).
// The three-callback overload is for unregistered geometry/reference callers.
// A pillar callback includes
// its Create and subsequent isPillar publication. No area callback is made
// for the extra pillars after the string.
struct BuilderStringTraversal {
    int y=0, buildx=0, t=0, p=0;
    unsigned i=0;
    bool reversed=false, wrap=false, leftRoom=false, setPillar=true;
    bool succeeded=true;
    template<class Area,class Common,class Pillar>
    bool run(const std::uint8_t* text,unsigned length,unsigned extraRows,
             Area area,Common common,Pillar pillar){
        return run(text,length,extraRows,area,common,pillar,[](int){return true;});
    }
    template<class Area,class Common,class Pillar,class PublishY>
    bool run(const std::uint8_t* text,unsigned length,unsigned extraRows,
             Area area,Common common,Pillar pillar,PublishY publishY){
        if(!succeeded)return false;
        if(executing_ || !text || !length || length>65535 || extraRows>65535)
            return succeeded=false;
        executing_=true;
        auto fail=[&](){executing_=false;return succeeded=false;};
        auto position=[&](){
            const std::int64_t x=leftRoom?std::int64_t(t)*16:
                (wrap?(reversed?336:144):(reversed?320:160))+
                std::int64_t(t)*(reversed?-16:16);
            if(x<-2147483647-1LL || x>2147483647)return false;
            buildx=int(x);return true;
        };
        auto advance=[&](){
            if(y>2147483647-16)return false;
            const int next=y+16;
            if(!publishY(next)||!succeeded)return false;
            y=next;return true;
        };
        t=0;
        for(i=1;i<=length;++i){
            const char symbol=char(text[i-1]);
            // VM calls string_delete, then discards the returned string. It
            // does not erase these characters or renumber subsequent indices.
            if(symbol=='\t' || (i<3 && (symbol=='\r'||symbol=='\n')))continue;
            if(t==2147483647)return fail();
            ++t;
            if(!position() || !area(symbol,buildx,y) || !succeeded)return fail();
            if(symbol=='\r'||symbol=='\n'){
                if(i>1){
                    ++i; // Source consumes the following character, even if not LF.
                    if(!position() || (setPillar && !pillar(buildx,y)) || !succeeded)return fail();
                    t=0;
                    if(!position() || (setPillar && !pillar(buildx,y)) || !succeeded)return fail();
                    if(!advance())return fail();
                }else t=0;
            }else if(!common(symbol,buildx,y) || !succeeded)return fail();
            p=0;
        }
        if(extraRows){
            leftRoom=false; // sideRoomGeneration = 0 before extraColumn+1.
            for(unsigned row=0;row<extraRows;++row){
                t=0;
                if(!position() || (setPillar && !pillar(buildx,y)) || !succeeded)return fail();
                t=10;
                if(!position() || (setPillar && !pillar(buildx,y)) || !succeeded)return fail();
                if(!advance())return fail();
            }
        }
        executing_=false;return true;
    }
private:
    bool executing_=false;
};
}
#endif
