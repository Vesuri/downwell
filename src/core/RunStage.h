#ifndef DOWNWELL_CORE_RUN_STAGE_H
#define DOWNWELL_CORE_RUN_STAGE_H
namespace downwell {
// ResetPlayer/scrPlayerGlobalStat and scrNextLevel VM blocks 1--6.
// Room loading and levelTile selection remain the caller's responsibility.
struct RunStage {
    int area = 0, level = 0;
    void advance(int requestedArea = 0) {
        ++level;
        if (area == 0) area = 1;
        if (level > 3) { ++area; level = 1; }
        if (requestedArea > 0) { area = requestedArea; level = 1; }
    }
};
} // namespace downwell
#endif
