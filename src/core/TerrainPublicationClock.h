#ifndef DOWNWELL_CORE_TERRAIN_PUBLICATION_CLOCK_H
#define DOWNWELL_CORE_TERRAIN_PUBLICATION_CLOCK_H
#include "RunnerRegistrationOrder.h"
namespace downwell {
// The source clock is the sum of independent publication counts. Terrain
// records keep the non-terrain basis at birth, plus their terrain ordinal.
// Publishing terrain therefore need not update this consumer's clock or
// create a RunnerRetainedLayerState. Reading a record recovers the EXACT old
// serial, not a different ordering key. The terrain ordinal is cumulative:
// destruction, reuse and room clear must not decrement or reset it while a
// consumer remains bound.
class TerrainPublicationClock {
    RunnerRegistrationOrder other_{};
public:
    static RunnerRegistrationOrder serial(RunnerRegistrationOrder basis,unsigned terrainOrdinal){
        const auto before=basis.low;
        basis.low+=terrainOrdinal;
        if(basis.low<before)++basis.high;
        return basis;
    }
    RunnerRegistrationOrder basis()const{return other_;}
    RunnerRegistrationOrder current(unsigned terrainPublished)const{return serial(other_,terrainPublished);}
    RunnerRegistrationOrder advance(unsigned terrainPublished){
        other_.advance();return current(terrainPublished);
    }
    // Existing callback-based terrain constructors still count as ordinary
    // publications until that birth actually belongs to the source-record
    // owner. Do not count the same birth through both routes.
    void advanceOther(unsigned count){other_=serial(other_,count);}
};
template<bool Enabled> struct TerrainPublicationClockStorage {};
template<> struct TerrainPublicationClockStorage<true> {TerrainPublicationClock sourceClock_;};
}
#endif
