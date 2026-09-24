#ifndef DOWNWELL_AMIGA_GAMEPLAY_SCENE_STORAGE_H
#define DOWNWELL_AMIGA_GAMEPLAY_SCENE_STORAGE_H
// Ownership of the retired garden allocation during gameplay. The staging
// ring can rotate into either display slot; its entire region stays reserved.
// HUD art is immutable at the high end; the middle is available to results art.
struct GameplaySceneStorage {
    unsigned resultsOffset = 0;
    unsigned resultsCapacity = 0;
    unsigned hudOffset = 0;
    bool valid = false;
    static GameplaySceneStorage partition(unsigned total, unsigned staging,
                                           unsigned hudBytes) {
        GameplaySceneStorage result;
        if (staging > total || hudBytes > total - staging) return result;
        const unsigned hudOffset = (total - hudBytes) & ~3u;
        if (hudOffset < staging) return result;
        result.resultsOffset = staging;
        result.resultsCapacity = hudOffset - staging;
        result.hudOffset = hudOffset;
        result.valid = true;
        return result;
    }
};
#endif
