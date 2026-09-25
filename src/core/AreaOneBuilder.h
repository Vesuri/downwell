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
    bool emitObject(std::int32_t objectId, int x, int y,
                    GameMakerRandom& random, EmitFunction emit,
                    void* context);
    bool emitPattern(const BuilderPatternPack& patterns,
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
