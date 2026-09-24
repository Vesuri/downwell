#include "core/PlayerDeathVerification.h"
#include <cstdio>
int main() {
    const auto first = downwell::verifyPlayerDeath();
    const auto second = downwell::verifyPlayerDeath();
    if (first.coverage != downwell::playerDeathVerificationCoverage ||
        first.digest != second.digest || first.coverage != second.coverage) {
        std::fprintf(stderr, "death CPU fixture coverage/repeatability failed: %x\n", first.coverage);
        return 1;
    }
    std::puts("death CPU fixture covers publication, motion, alarms, skip and poses");
}
