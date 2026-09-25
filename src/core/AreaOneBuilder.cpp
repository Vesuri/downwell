#include "AreaOneBuilder.h"

namespace downwell {

void AreaOneBuilder::reset(int x, int y, GameMakerRandom& random) {
    reset(x, y, random, Setup{});
}

void AreaOneBuilder::reset(int x, int y, GameMakerRandom& random, const Setup& setup) {
    *this = AreaOneBuilder{};
    x_ = x;
    y_ = y;
    generationCount_ = setup.level == 1 && setup.firstBoot ? -30 : -10;
    rowsGenerated_ = 0;
    rightRooms_[0] = rightRooms_[1] = -1;
    startPattern_ = -1;
    startReversed_ = false;
    startRowsGenerated_ = 0;
    lastMainPattern_ = -1;
    lastPatternReversed_ = false;
    mainPatternsGenerated_ = 0;
    spawnerMade_ = false;
    lastRightPattern_ = -1;
    sideRoomsGenerated_ = 0;
    lastShopPattern_ = -1;
    shopsGenerated_ = 0;
    rightRoomsMade_ = 0;
    shopMade_ = setup.level == 1 && setup.style != 4;
    levelLength_ = 15 + setup.level;
    style_ = setup.style;
    member_ = setup.member;
    endReached_ = false;
    endPattern_ = -1;
    chunkOrCrate_ = 0;
    specialObjectsGenerated_ = 0;

    rightRoomCount_ = random.irandomSmall(2) == 2 ? 3 : 2;
    if (shopMade_) rightRoomCount_ = 2;
    if (member_) ++rightRoomCount_;
    for (unsigned i = 0; i < rightRoomCount_; ++i) {
        unsigned rejected = 0;
        for (;;) {
            rightRooms_[i] = 3 + static_cast<int>(random.irandomSmall(levelLength_ - 4));
            if (!i) { if (member_) rightRooms_[i] = 0; break; }
            bool conflict = false;
            for (unsigned j = 0; j < i; ++j)
                if (rightRooms_[i] - rightRooms_[j] <= 2 && rightRooms_[j] - rightRooms_[i] <= 2)
                    conflict = true;
            if (!conflict) break;
            if (++rejected > 100) { rightRooms_[i] = 9999; break; }
        }
    }
}

void AreaOneBuilder::restoreInitialCheckpoint(
    int x, int y, int generationCount,
    int firstRightRoom, int secondRightRoom) {
    *this = AreaOneBuilder{};
    x_ = x;
    y_ = y;
    generationCount_ = generationCount;
    rightRooms_[0] = firstRightRoom;
    rightRooms_[1] = secondRightRoom;
}

bool AreaOneBuilder::emitObject(std::int32_t objectId, int x, int y,
                                GameMakerRandom& random,
                                EmitFunction emit, void* context) {
    DeferredCreateInstance command{objectId, x, y};
    switch (objectId) {
    case 92: {
        random.irandomSmall(3);
        command.imageSpeed = 0.0F;
        command.integerImageIndex = 16 + random.irandomSmall(1);
#ifndef DOWNWELL_FREESTANDING
        command.imageIndex = static_cast<float>(command.integerImageIndex);
#endif
        if (random.irandomSmall(50) < 1) {
            random.irandomSmall(1);
            command.integerImageIndex = 22;
            command.imageIndex = 22.0F;
        }
    }
    break;
    case 111: {
        command.imageSpeed = 0.25F;
        command.velocityX = 0.5F;
        command.integerVelocityX = 128;
        command.motionInitialized = true;
        command.grounded = true;
    }
    break;
    case 112: {
        command.imageSpeed = 0.15F;
        command.velocityX = 0.25F;
        command.integerVelocityX = 64;
        command.auxiliaryState = random.irandomSmall(1) == 0 ? 1 : -1;
        command.motionInitialized = true;
        command.grounded = true;
        command.alarms.set(1, 60 + static_cast<int>(random.irandomSmall(60)));
    }
    break;
    case 113: {
        command.imageSpeed = 0.0F;
        command.motionInitialized = true;
        command.alarms.set(1,
            60 + static_cast<int>(random.randomIntegerSmall(60)));
    }
    break;
    case 116: {
        command.imageSpeed = 0.2F;
        random.nextU32();
        command.integerDirection = random.irandomSmall(359);
#ifndef DOWNWELL_FREESTANDING
        command.direction = static_cast<float>(command.integerDirection);
#endif
        command.moveSpeed = 1.25F;
        command.integerMoveSpeed = 320;
        command.motionInitialized = true;
        command.active = false;
    }
    break;
    case 117: {
        command.imageSpeed = 0.1F;
        command.integerVelocityY = random.irandomSmall(1) == 0 ? 26 : -26;
        command.velocityY = command.integerVelocityY > 0 ? 0.1F : -0.1F;
        command.motionInitialized = true;
        command.alarms.set(1, 60);
    }
    break;
    case 119: {
        command.imageSpeed = 0.1F;
        command.motionState = random.irandomSmall(1) == 0 ? 1 : -1;
        command.moveSpeed = 1.0F;
        command.integerMoveSpeed = 256;
        command.motionInitialized = true;
        command.alarms.set(0, 2);
    }
    break;
    case 289: {
        command.imageSpeed = 0.0F;
        command.integerScaleX = random.irandomSmall(1) == 0 ? 1 : -1;
        command.scaleX = command.integerScaleX < 0 ? -1.0F : 1.0F;
        command.integerImageIndex = random.irandomSmall(15);
#ifndef DOWNWELL_FREESTANDING
        command.imageIndex = static_cast<float>(command.integerImageIndex);
#endif
        if (random.randomIntegerSmall(300) == 0) {
            command.integerImageIndex = 16;
            command.imageIndex = 16.0F;
        }
    }
    break;
    case 82:
        random.nextU32();
        break;
    case 293: {
        command.imageSpeed = 0.0F;
        command.alarms.set(1, 30 + static_cast<int>(random.irandomSmall(30)));
        command.alarms.set(2, 180 + static_cast<int>(random.irandomSmall(180)));
    }
    break;
    case 109: {
        command.imageSpeed = 0.0F;
        command.alarms.set(0, 10);
        command.alarms.set(3, 10);
        random.irandomSmall(1);
        std::uint32_t module = 2;
        while (module == 2)
            module = random.irandomSmall(6);
    }
    break;
    default: break;
    }
    return emit != nullptr && emit(context, command);
}

bool AreaOneBuilder::emitPattern(
    const BuilderPatternPack& patterns, const PackedBuilderPattern& pattern,
    bool reversed, bool appendPillars, GameMakerRandom& random,
    EmitFunction emit, void* context, std::uint32_t& advancedRows) {
    for (std::uint16_t row = 0; row < pattern.rowCount; ++row) {
        const int rowY = y_ + row * 16;
        std::uint32_t width = 0;
        const std::uint8_t* cells = nullptr;
        if (!patterns.rowData(pattern, row, cells, width))
            return false;
        for (std::uint16_t column = 0; column < width; ++column) {
            const char symbol = static_cast<char>(cells[column]);
            const int ordinal = column + 1;
            const int buildX = reversed ? 320 - ordinal * 16
                                        : 160 + ordinal * 16;
            if ((symbol == 'I' || symbol == 'J') &&
                !emitObject(296, buildX, rowY, random, emit, context))
                return false;
            if (symbol == '^' && random.irandomSmall(4) != 0 &&
                !emitObject(289, buildX, rowY, random, emit, context))
                return false;
            if (random.nextU32() <= 0x55555555U) {
                std::int32_t enemyId = -1;
                if (symbol == 'a') enemyId = 111;
                if (symbol == 'b') enemyId = 116;
                if (symbol == 'd') enemyId = 113;
                if (symbol == 'e') enemyId = 117;
                if (symbol == 't') enemyId = 112;
                if (symbol == '%') {
                    const std::int32_t enemies[] = {113, 111, 119, 112};
                    enemyId = enemies[random.irandomSmall(3)];
                }
                if (enemyId >= 0 &&
                    !emitObject(enemyId, buildX, rowY, random, emit, context))
                    return false;
            }
            std::int32_t objectId = -1;
            if (symbol == '1') objectId = 60;
            if (symbol == '2' && random.irandomSmall(1) != 0) objectId = 60;
            if (symbol == '-') objectId = 93;
            if (symbol == 'B') objectId = 92;
            if (symbol == '3' && random.irandomSmall(1) != 0) objectId = 92;
            if (symbol == '|') objectId = 60;
            if (symbol == 'v') objectId = 376;
            if (symbol == 'O') objectId = 82;
            if (symbol == '_') objectId = 62;
            if (symbol == '$') objectId = 277;
            if (symbol == '\\') objectId = 285;
            if (symbol == '=') objectId = 306;
            if (symbol == '@') {
                if (chunkOrCrate_ == 0) {
                    const std::int32_t choices[] = {293, 109};
                    chunkOrCrate_ = choices[random.irandomSmall(1)];
                } else {
                    chunkOrCrate_ = chunkOrCrate_ == 109 ? 293 : 109;
                }
                objectId = chunkOrCrate_;
            }
            if (symbol == '{' || symbol == '}') {
                if (!emitObject(60, reversed ? 16 : 464, rowY,
                                random, emit, context))
                    return false;
                ++specialObjectsGenerated_;
                objectId = -1;
            }
            if (objectId >= 0 &&
                !emitObject(objectId, buildX, rowY, random, emit, context))
                return false;
            if (symbol == 'I' || symbol == 'J' || symbol == 'v' ||
                symbol == 'O' || symbol == '_' || symbol == '$' ||
                symbol == '\\' || symbol == '@')
                ++specialObjectsGenerated_;
        }
        if (row + 1 < pattern.rowCount) {
            random.nextU32();
            const bool sideRoom =
                pattern.family == BuilderPatternFamily::areaOneRight ||
                pattern.family == BuilderPatternFamily::areaOneShop;
            const int firstX = sideRoom ? (reversed ? 0 : 480)
                                        : (reversed ? 160 : 320);
            const int secondX = reversed ? 320 : 160;
            if (!emitObject(60, firstX, rowY, random, emit, context) ||
                !emitObject(60, secondX, rowY, random, emit, context))
                return false;
        }
    }
    advancedRows = pattern.rowCount - 1U;
    y_ += static_cast<int>(advancedRows * 16U);
    if (appendPillars) {
        const int firstX = reversed ? 160 : 320;
        const int secondX = reversed ? 320 : 160;
        for (int padding = 0; padding < 3; ++padding) {
            if (!emitObject(60, firstX, y_, random, emit, context) ||
                !emitObject(60, secondX, y_, random, emit, context))
                return false;
            y_ += 16;
            ++advancedRows;
        }
    }
    return true;
}

bool AreaOneBuilder::stepVisible(const BuilderPatternPack& patterns,
                                 GameMakerRandom& random, int viewBottomCeiling,
                                 EmitFunction emit, void* context,
                                 AreaOneBuilderStep& result) {
    result = AreaOneBuilderStep{};
    if (endReached_ || viewBottomCeiling <= y_ - 128)
        return true;
    return step(patterns, random, emit, context, result);
}

bool AreaOneBuilder::step(const BuilderPatternPack& patterns,
                          GameMakerRandom& random, EmitFunction emit,
                          void* context, AreaOneBuilderStep& result) {
    result = AreaOneBuilderStep{};
    if (endReached_)
        return true;
    if (generationCount_ < -1) {
        random.irandomSmall(1);
        for (int cell = 0; cell < 19; ++cell)
            random.nextU32();
        if (!emitObject(60, 160, y_, random, emit, context) ||
            !emitObject(60, 320, y_, random, emit, context))
            return false;
        y_ += 16;
        ++generationCount_;
        ++rowsGenerated_;
        result.advancedRows = 1;
        return true;
    }
    if (!patterns.valid() || patterns.patternCount() == 0)
        return false;

    PackedBuilderPattern pattern;
    std::uint16_t selected = 0;
    bool reversed = false;
    bool appendPillars = false;
    bool sideRoom = false;
    BuilderPatternFamily family = BuilderPatternFamily::areaOneMain;
    const int patternY = y_;
    if (generationCount_ == -1) {
        selected = static_cast<std::uint16_t>(random.irandomSmall(4));
        family = BuilderPatternFamily::areaOneStart;
        reversed = random.irandomSmall(1) != 0;
        startPattern_ = selected;
        startReversed_ = reversed;
    } else if (generationCount_ > static_cast<int>(levelLength_)) {
        selected = static_cast<std::uint16_t>(random.irandomSmall(3));
        family = BuilderPatternFamily::areaOneEnd;
        reversed = random.irandomSmall(1) != 0;
        appendPillars = true;
    } else {
        if (!spawnerMade_) {
            if (!emitObject(78, x_ - 100, y_, random, emit, context))
                return false;
            spawnerMade_ = true;
        }
        unsigned sideIndex = 0;
        for (unsigned i = 0; i < rightRoomCount_; ++i)
            if (generationCount_ == rightRooms_[i]) { sideRoom = true; sideIndex = i; }
        bool makeShop = false;
        if (sideRoom) {
            ++rightRoomsMade_;
            if (!shopMade_) {
                if (style_ == 1) makeShop = random.irandomSmall(6) == 6;
                else if (style_ == 4) makeShop = rightRoomsMade_ >= rightRoomCount_;
                else makeShop = random.irandomSmall(1) != 0;
                if (member_) makeShop = true;
            }
            if (makeShop) {
                family = BuilderPatternFamily::areaOneShop;
                selected = static_cast<std::uint16_t>(random.irandomSmall(1));
            } else {
                family = BuilderPatternFamily::areaOneRight;
                unsigned attempts = 0;
                bool repeat;
                do {
                    ++attempts;
                    selected = static_cast<std::uint16_t>(random.irandomSmall(9));
                    repeat = false;
                    for (unsigned i = 0; i < rightRoomCount_; ++i)
                        if (sideRoomLog_[i] == selected) repeat = true;
                } while (repeat && attempts <= 60);
                sideRoomLog_[sideIndex] = selected;
            }
        } else {
            family = BuilderPatternFamily::areaOneMain;
            do {
                selected = static_cast<std::uint16_t>(random.irandomSmall(40));
            } while (selected == lastMainPattern_);
        }
        reversed = makeShop ? false : random.irandomSmall(1) != 0;
        appendPillars = true;
    }
    if (!patterns.findPattern(family, selected, pattern))
        return false;
    std::uint32_t advancedRows = 0;
    if (!emitPattern(patterns, pattern, reversed, appendPillars, random,
                     emit, context, advancedRows))
        return false;
    lastPatternReversed_ = reversed;
    if (generationCount_ == -1) {
        generationCount_ = 0;
        startRowsGenerated_ = advancedRows;
    } else if (family == BuilderPatternFamily::areaOneEnd) {
        endPattern_ = selected;
        endReached_ = true;
    } else {
        if (family == BuilderPatternFamily::areaOneShop) {
            lastShopPattern_ = selected;
            ++shopsGenerated_;
            shopMade_ = true;
        } else if (family == BuilderPatternFamily::areaOneRight) {
            lastRightPattern_ = selected;
            ++sideRoomsGenerated_;
        } else {
            lastMainPattern_ = selected;
            ++mainPatternsGenerated_;
        }
        ++generationCount_;
    }
    rowsGenerated_ += advancedRows;
    result.patternGenerated = true;
    result.family = family;
    result.patternIndex = selected;
    result.patternY = patternY;
    result.reversed = reversed;
    result.sideRoom = sideRoom;
    result.appendPillars = appendPillars;
    result.advancedRows = advancedRows;
    return true;
}

} // namespace downwell
