#include "core/LevelUpgradeOffers.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
int main() {
    using downwell::GameMakerRandom;
    using downwell::LevelUpgradeOffers;
    LevelUpgradeOffers offers;
    GameMakerRandom random(2);
    require(offers.randomize(3, 23, random, [](unsigned) { return false; }), "four offers supported");
    require(offers.ids[0] == 100 && offers.ids[1] == 100 && offers.ids[2] == 100 && offers.ids[3] == 0 &&
        random.drawCount() == 4 * 61 * 2, "exhausted pool keeps source fallbacks and consumes 61 rolls per slot");
    offers.ids[3] = 17;
    require(offers.randomize(3, 23, random, [](unsigned) { return false; }) && offers.ids[3] == 17,
        "reroll preserves fourth slot on exhausted pool");
    const auto draws = random.drawCount();
    require(!offers.randomize(4, 23, random, [](unsigned) { return true; }) &&
        !offers.randomize(2, 65536, random, [](unsigned) { return true; }) && random.drawCount() == draws,
        "unsupported bounds fail without changing the RNG stream");
    GameMakerRandom singleRandom(3);
    LevelUpgradeOffers single;
    require(single.randomize(2, 0, singleRandom, [](unsigned) { return true; }) &&
        single.ids[0] == 0 && single.ids[1] == 100 && single.ids[2] == 100 &&
        singleRandom.drawCount() == 2 + 2 * 61 * 2,
        "duplicate-only pool exhausts later slots without repeating first offer");
    GameMakerRandom lastTryRandom(7), lastTryOracle(7);
    unsigned calls = 0, lastId = 0;
    for (unsigned attempt = 0; attempt < 61; ++attempt) lastId = lastTryOracle.irandom(23);
    require(single.randomize(0, 23, lastTryRandom, [&](unsigned) { return ++calls == 61; }) &&
        single.ids[0] == lastId && calls == 61 && lastTryRandom.drawCount() == 122,
        "sixty-first candidate can still be accepted before rejection limit");
    for (unsigned seed = 0; seed < 256; ++seed) {
        GameMakerRandom actual(seed), oracle(seed);
        LevelUpgradeOffers result;
        const unsigned last = seed % 3 + 1;
        // Independently reproduce the VM's while/break control flow using
        // the runner's general irandom, rather than the 68000 small-range path.
        unsigned expected[4] = {100, 100, 100, 0};
        auto eligible = [seed](unsigned id) { return (id + seed) % 4 == 0; };
        for (unsigned slot = 0; slot <= last; ++slot) {
            unsigned failures = 0;
            while (true) {
                unsigned id = oracle.irandom(23);
                unsigned matches = 0;
                if (eligible(id)) {
                    for (unsigned j = slot; j > 0; --j) matches += expected[j - 1] == id;
                    if (!matches) { expected[slot] = id; break; }
                }
                if (++failures > 60) break;
            }
        }
        require(result.randomize(last, 23, actual, eligible), "seeded roll succeeds");
        for (unsigned slot = 0; slot < 4; ++slot)
            require(result.ids[slot] == expected[slot], "offer IDs match source control flow");
        require(actual.drawCount() == oracle.drawCount() && actual.stateIndex() == oracle.stateIndex(),
            "selection preserves runner draw count and index");
        for (unsigned word = 0; word < 16; ++word)
            require(actual.stateWord(word) == oracle.stateWord(word), "full RNG state agrees after rejection sampling");
    }
    std::puts("Level upgrade offer selection and RNG tests passed");
}
