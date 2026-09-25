#include "AmigaGameplayGeneration.h"
#include "AmigaPortRuntime.h"
#include "AmigaProfile.h"

void AmigaGameplayGeneration::bind(AmigaPortRuntime& runtime) {
    runtime_ = &runtime;
    dirtyFirst = worldRows; dirtyEnd = 0;
    builder_.setCreateObserver(create, this);
}

bool AmigaGameplayGeneration::create(void* opaque,
    const downwell::DeferredCreateInstance& command) {
    DW_PROFILE_SCOPE(profileGeneratedRegistration);
    auto& self = *static_cast<AmigaGameplayGeneration*>(opaque);
    return self.runtime_->createGeneratedInstance(command, self.objects_,
        [&self](const downwell::DeferredCreateInstance& command) {
            {
                DW_PROFILE_SCOPE(profileGeneratedIndex);
                if (!amigaAddTerrainObject(self.index_, command.objectId, command.x, command.y) ||
                    !self.tiles_.append(command)) return false;
            }
            if (command.objectId == 60 || command.objectId == 92 || command.objectId == 93 ||
                command.objectId == 289 || command.objectId == 293 || command.objectId == 296 ||
                command.objectId == 109) {
                if (command.y - 24 < self.dirtyFirst) self.dirtyFirst = command.y - 24;
                if (command.y + 24 > self.dirtyEnd) self.dirtyEnd = command.y + 24;
            }
            return true;
        });
}

bool AmigaGameplayGeneration::step(downwell::GameMakerRandom& random, int viewBottom) {
    dirtyFirst = worldRows; dirtyEnd = 0;
    if (!runtime_) return false;
    downwell::AreaOneBuilderStep result;
    const bool ok = builder_.stepVisible(patterns_, random, viewBottom, result);
    if (dirtyFirst < 0) dirtyFirst = 0;
    if (dirtyEnd > worldRows) dirtyEnd = worldRows;
    return ok;
}
