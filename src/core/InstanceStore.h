#ifndef DOWNWELL_CORE_INSTANCE_STORE_H
#define DOWNWELL_CORE_INSTANCE_STORE_H

#include "AlarmBank.h"
#include "FrogHorizontalMotion.h"
#include "FrogVerticalMotion.h"
#include "FrogHitState.h"
#include "CrawlerMotion.h"
#include "SnailMotion.h"
#include "SnailHitState.h"
#include "WalkingEnemyMotion.h"
#include "BatHitState.h"
#include "BatBehavior.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

struct InstanceRecord {
    std::uint32_t instanceId = 0;
    std::int32_t objectId = -1;
    std::int32_t roomId = -1;
    std::int32_t x = 0;
    std::int32_t y = 0;
    float scaleX = 1.0F;
    float scaleY = 1.0F;
    float rotation = 0.0F;
    float imageSpeed = 1.0F;
    float imageIndex = 0.0F;
    std::int32_t spriteId = -1;
    std::int32_t creationCodeId = -1;
    std::int32_t preCreateCodeId = -1;
    std::uint32_t layerId = 0;
    std::int32_t depth = 0;
    bool persistent = false;
    bool active = true;
    bool activationPending = false;
    std::int32_t health = 0;
    std::int32_t reward = 0;
    float preciseX = 0.0F;
    float preciseY = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    float direction = 0.0F;
    float moveSpeed = 0.0F;
    std::int32_t motionState = 0;
    std::int32_t auxiliaryState = 0;
    bool motionInitialized = false;
    bool grounded = false;
    bool frogLanded = true; // enmFrog Create; cleared by an airborne Step
    FrogHorizontalMotion frogHorizontal;
    FrogVerticalMotion frogVertical;
    FrogHitState frogHit;
    CrawlerMotion crawler;
    SnailMotion snail;
    SnailHitState snailHit;
    BatHitState batHit;
    BatActivity batActivity;
    WalkingEnemyMotion walker;
    bool frogMotionInitialized = false;
    AlarmBank alarms;
};

enum class CreateInstanceResult {
    success,
    duplicateId,
    full
};

// Stable-order, allocation-free storage suitable for both the reference build
// and an Amiga capacity selected later from measured room/runtime requirements.
template <std::size_t Capacity>
class InstanceStore {
public:
    static constexpr std::size_t capacity = Capacity;

    void clear() { size_ = 0; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    const InstanceRecord& at(std::size_t index) const { return records_[index]; }
    InstanceRecord& at(std::size_t index) { return records_[index]; }

    const InstanceRecord* find(std::uint32_t instanceId) const {
        for (std::size_t index = 0; index < size_; ++index) {
            if (records_[index].instanceId == instanceId)
                return &records_[index];
        }
        return nullptr;
    }

    InstanceRecord* find(std::uint32_t instanceId) {
        for (std::size_t index = 0; index < size_; ++index) {
            if (records_[index].instanceId == instanceId)
                return &records_[index];
        }
        return nullptr;
    }

    CreateInstanceResult create(std::uint32_t instanceId,
                                std::int32_t objectId,
                                std::int32_t roomId,
                                bool persistent) {
        if (find(instanceId) != nullptr)
            return CreateInstanceResult::duplicateId;
        if (size_ == Capacity)
            return CreateInstanceResult::full;

        InstanceRecord& instance = records_[size_++];
        instance = InstanceRecord{};
        instance.instanceId = instanceId;
        instance.objectId = objectId;
        instance.roomId = roomId;
        instance.persistent = persistent;
        return CreateInstanceResult::success;
    }

    bool erase(std::uint32_t instanceId) {
        for (std::size_t index = 0; index < size_; ++index) {
            if (records_[index].instanceId != instanceId)
                continue;
            for (std::size_t next = index + 1; next < size_; ++next)
                records_[next - 1] = records_[next];
            --size_;
            return true;
        }
        return false;
    }

    void enterRoom(std::int32_t roomId) {
        std::size_t destination = 0;
        for (std::size_t source = 0; source < size_; ++source) {
            if (!records_[source].persistent)
                continue;
            if (destination != source)
                records_[destination] = records_[source];
            records_[destination].roomId = roomId;
            ++destination;
        }
        size_ = destination;
    }

private:
    std::array<InstanceRecord, Capacity> records_{};
    std::size_t size_ = 0;
};

} // namespace downwell

#endif
