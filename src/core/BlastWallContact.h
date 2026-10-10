#ifndef DOWNWELL_CORE_BLAST_WALL_CONTACT_H
#define DOWNWELL_CORE_BLAST_WALL_CONTACT_H
#include "BlastActor.h"
namespace downwell {
// ID is queried only after the source's direct-parent guard. Unknown IDs may
// never be replaced by native handles, even for an already exhausted box.
template<unsigned Capacity,class Read,class Write>
bool blastWallContact(BlastActor<Capacity>& blast,int parent,unsigned id,Read read,Write write,bool& good){
    struct Events {
        int parent;unsigned id;Read& read;Write& write;
        bool directParent(int& out){out=parent;return true;}
        bool targetId(std::uint32_t& out){out=id;return id&&id<=0x7fffffffu;}
        bool wallHealth(MotionBinary64& out){return read(out);}
        bool setWallHealth(MotionBinary64 value){return write(value);}
    } events{parent,id,read,write};
    return blast.wallContact(events,good);
}
}
#endif
