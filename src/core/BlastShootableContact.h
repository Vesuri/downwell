#ifndef DOWNWELL_CORE_BLAST_SHOOTABLE_CONTACT_H
#define DOWNWELL_CORE_BLAST_SHOOTABLE_CONTACT_H
#include "BlastActor.h"
namespace downwell {
template<unsigned Capacity,class Read,class Write,class Flash>
bool blastShootableContact(BlastActor<Capacity>& actor,std::uint32_t id,MotionBinary64 x,MotionBinary64 y,
                          Read read,Write write,Flash flash,bool& good){
    struct Events {
        BlastActor<Capacity>& actor;std::uint32_t id;MotionBinary64 x,y;
        Read& read;Write& write;Flash& emit;
        bool targetId(std::uint32_t& out){out=id;return id&&id<=0x7fffffffu;}
        bool objectHealth(MotionBinary64& out){return read(out);}
        bool setObjectHealth(MotionBinary64 value){return write(value);}
        MotionBinary64 callerX()const{return x;}
        MotionBinary64 callerY()const{return y;}
        bool flash(MotionBinary64 px,MotionBinary64 py,int count,int variation,int radius){
            return count==1&&!variation&&!radius&&emit(actor.flashCaller,px,py);
        }
    } events{actor,id,x,y,read,write,flash};
    return actor.shootableContact(events,good);
}
}
#endif
