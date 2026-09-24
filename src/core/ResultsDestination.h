#ifndef DOWNWELL_CORE_RESULTS_DESTINATION_H
#define DOWNWELL_CORE_RESULTS_DESTINATION_H
#include "DeathMenuSequence.h"
namespace downwell {
// DeathMenu Begin Step, ResetPlayer, scrNextLevel(1), BackToSurface.
// Numeric room IDs belong to the recovered resource table, not a backend.
enum class ResultsDestination : int { none = -1, styleSelect = 8, gameplay = 11, surface = 20 };
inline ResultsDestination resultsDestination(DeathMenuSequence::Action action) {
    switch (action) {
    case DeathMenuSequence::Action::Retry: return ResultsDestination::gameplay;
    case DeathMenuSequence::Action::StyleSelect: return ResultsDestination::styleSelect;
    case DeathMenuSequence::Action::Surface: return ResultsDestination::surface;
    default: return ResultsDestination::none;
    }
}
} // namespace downwell
#endif
