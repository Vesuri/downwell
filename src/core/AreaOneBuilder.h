#ifndef DOWNWELL_CORE_AREA_ONE_BUILDER_H
#define DOWNWELL_CORE_AREA_ONE_BUILDER_H

#include "BuilderPatternPack.h"
#include "DeferredInstanceQueue.h"
#include "GameMakerRandom.h"
#include <cstdint>

namespace downwell {

struct AreaOneBuilderStep {
    bool patternGenerated = false;
    BuilderPatternFamily family = BuilderPatternFamily::areaOneMain;
    std::uint16_t patternIndex = 0;
    int patternY = 0;
    bool reversed = false;
    bool sideRoom = false;
    bool appendPillars = false;
    std::uint32_t advancedRows = 0;
};

// Allocation-free recovered scrLevelPat1/scrBuilder implementation.  Object
// Create-event state is emitted synchronously because those events consume the
// same GameMaker random stream as pattern selection.
class AreaOneBuilder {
public:
    using EmitFunction = bool (*)(void*, const DeferredCreateInstance&);
    // A borrowed, deterministic same-row run. Setup may inspect its geometry,
    // but execute() alone advances the live random stream, one cell immediately
    // before its synchronous Create callback. Never retain this view.
    class TerrainRun {
        friend class AreaOneBuilder;
        GameMakerRandom& random_;
        DeferredCreateInstance& command_;
        mutable bool started_=false,completed_=false;
        TerrainRun(GameMakerRandom& random,DeferredCreateInstance& command,
            int first,int row,int stride,unsigned length):random_(random),command_(command),
            x(first),y(row),stepX(stride),count(length),objectId(command.objectId){}
    public:
        const int x,y,stepX;
        const unsigned count;
        const int objectId;
        bool completed()const{return completed_;}
        bool execute(EmitFunction emit,void* context)const{
            return executeMode<true>(emit,context);
        }
        template<class Emit> bool executeEach(Emit emit)const{
            if(started_)return false;
            started_=true;
            int position=x;
            for(unsigned cell=0;cell<count;++cell){
                random_.nextU32();
                command_.objectId=objectId;command_.x=position;command_.y=y;
                if(!emit(command_))return false;
                if(cell+1<count)position+=stepX;
            }
            completed_=true;return true;
        }
        // For backends whose run Creates only publish occupancy that nothing
        // reads before the run returns: draw every cell's value, then publish
        // the whole span once. Publish must be admitted beforehand; a failure
        // here leaves more draws than the per-cell prefix would.
        template<class Publish> bool executeSpan(Publish publish)const{
            if(started_)return false;
            started_=true;
            random_.advance(count);
            const int span=int(count-1)*16;
            command_.objectId=objectId;command_.x=stepX<0?x-span:x+span;command_.y=y;
            if(!publish())return false;
            completed_=true;return true;
        }
        bool executeReference(EmitFunction emit,void* context)const{
            return executeMode<false>(emit,context);
        }
    private:
        template<bool Incremental> bool executeMode(EmitFunction emit,void* context)const{
            if(started_)return false;
            started_=true;
            int position=x;
            for(unsigned cell=0;cell<count;++cell){
                // Normal-mode builderCavern draws even for non-enemy symbols.
                random_.nextU32();
                command_.objectId=objectId;
                command_.x=Incremental?position:x+int(cell)*stepX;command_.y=y;
                if(!emit||!emit(context,command_))return false;
                // Advance only after synchronous Create, never from its borrowed
                // command storage, and never beyond the admitted run endpoint.
                if(Incremental&&cell+1<count)position+=stepX;
            }
            completed_=true;return true;
        }
    };
    using TerrainRunFunction=bool (*)(void*,const TerrainRun&,EmitFunction);
    void setTerrainRunEmitter(TerrainRunFunction emit){terrainRunEmitter_=emit;}
    // Opt-in until a backend uses the run to amortize owner setup. Reset clears
    // this setting and the optional run emitter. Scalar remains the oracle.
    void enableTerrainRuns(bool enabled){terrainRunsEnabled_=enabled;}

    void reset(int x, int y, GameMakerRandom& random);
    struct Setup {
        unsigned level = 1, style = 0;
        bool member = false, firstBoot = true;
    };
    void reset(int x, int y, GameMakerRandom& random, const Setup& setup);
    void restoreInitialCheckpoint(int x, int y, int generationCount,
                                  int firstRightRoom, int secondRightRoom);
    bool step(const BuilderPatternPack& patterns, GameMakerRandom& random,
              EmitFunction emit, void* context, AreaOneBuilderStep& result);
    // One objBuilder Step event, never a catch-up loop. Pass ceil(viewBottom)
    // so the strict integer threshold preserves fractional camera positions
    // without requiring floating point on the 68000.
    bool stepVisible(const BuilderPatternPack& patterns, GameMakerRandom& random,
                     int viewBottomCeiling, EmitFunction emit, void* context,
                     AreaOneBuilderStep& result);

    int y() const { return y_; }
    void setPlayerHealth(int health) { playerHealth_ = health; }
    int generationCount() const { return generationCount_; }
    std::uint32_t rowsGenerated() const { return rowsGenerated_; }
    int rightRoom(unsigned index) const {
        return index < rightRoomCount_ ? rightRooms_[index] : -1;
    }
    unsigned rightRoomCount() const { return rightRoomCount_; }
    int startPattern() const { return startPattern_; }
    bool startReversed() const { return startReversed_; }
    std::uint32_t startRowsGenerated() const { return startRowsGenerated_; }
    int lastMainPattern() const { return lastMainPattern_; }
    bool lastPatternReversed() const { return lastPatternReversed_; }
    std::uint32_t mainPatternsGenerated() const {
        return mainPatternsGenerated_;
    }
    bool spawnerMade() const { return spawnerMade_; }
    int lastRightPattern() const { return lastRightPattern_; }
    std::uint32_t sideRoomsGenerated() const { return sideRoomsGenerated_; }
    int lastShopPattern() const { return lastShopPattern_; }
    std::uint32_t shopsGenerated() const { return shopsGenerated_; }
    std::uint32_t rightRoomsMade() const { return rightRoomsMade_; }
    bool shopMade() const { return shopMade_; }
    bool endReached() const { return endReached_; }
    int endPattern() const { return endPattern_; }
    int chunkOrCrate() const { return chunkOrCrate_; }
    std::uint32_t specialObjectsGenerated() const {
        return specialObjectsGenerated_;
    }

private:
    TerrainRunFunction terrainRunEmitter_=nullptr;
    bool terrainRunsEnabled_=false;
    bool emitObject(std::int32_t objectId, int x, int y,
                    GameMakerRandom& random, EmitFunction emit,
                    void* context);
    bool emitPattern(const BuilderPatternPack& patterns,
                     const PackedBuilderPattern& pattern, bool reversed,
                     bool appendPillars, GameMakerRandom& random,
                     EmitFunction emit, void* context,
                     std::uint32_t& advancedRows);
    template<bool Runs> bool emitPatternMode(const BuilderPatternPack& patterns,
                     const PackedBuilderPattern& pattern, bool reversed,
                     bool appendPillars, GameMakerRandom& random,
                     EmitFunction emit, void* context,
                     std::uint32_t& advancedRows);

    int x_ = 0;
    int y_ = 704;
    int generationCount_ = -30;
    std::uint32_t rowsGenerated_ = 0;
    int rightRooms_[4] = {-1, -1, -1, -1};
    int sideRoomLog_[4] = {-1, -1, -1, -1};
    unsigned rightRoomCount_ = 2, levelLength_ = 16, style_ = 0;
    bool member_ = false;
    int startPattern_ = -1;
    bool startReversed_ = false;
    std::uint32_t startRowsGenerated_ = 0;
    int lastMainPattern_ = -1;
    bool lastPatternReversed_ = false;
    std::uint32_t mainPatternsGenerated_ = 0;
    bool spawnerMade_ = false;
    int lastRightPattern_ = -1;
    std::uint32_t sideRoomsGenerated_ = 0;
    int lastShopPattern_ = -1;
    std::uint32_t shopsGenerated_ = 0;
    std::uint32_t rightRoomsMade_ = 0;
    bool shopMade_ = true;
    bool endReached_ = false;
    int endPattern_ = -1;
    int chunkOrCrate_ = 0;
    int playerHealth_ = 4;
    std::uint32_t specialObjectsGenerated_ = 0;
};

} // namespace downwell

#endif
