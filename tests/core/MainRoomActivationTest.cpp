#include "core/MainRoomActivation.h"
#include <cstdlib>
#include <iostream>

static void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}

int main() {
    using downwell::ActivationInstance;
    ActivationInstance records[] = {
        {111, 16 * 256, false, true},
        {112, 16 * 256 + 1, false, true},
        {113, 0, true, true},
        {116, 0, false, false},
        {60, 0, false, true},
        {117, 0, false, true},
        {119, 0, false, true}
    };
    unsigned order[7] = {};
    unsigned writes = 0, reads = 0;
    auto read = [&](unsigned i) { ++reads; return records[i]; };
    auto activate = [&](unsigned i) {
        order[writes++] = i;
        records[i].active = true;
        records[i].pending = false;
    };
    int frontier = 0;
    require(downwell::advanceMainRoomActivation(frontier, -112L * 256, 7,
                read, activate) == 0 && frontier == 0 && reads == 0,
            "view equality does not advance or visit instances");
    require(downwell::advanceMainRoomActivation(frontier, -112L * 256 + 1, 7,
                read, activate) == 3 && frontier == 16,
            "strict view crossing advances exactly one row and activates eligible enemies");
    require(writes == 3 && order[0] == 0 && order[1] == 5 && order[2] == 6,
            "activation preserves storage order and excludes active/retired/non-enemy records");
    require(!records[0].pending && !records[1].active,
            "activation clears pending; fractional centre beyond frontier stays dormant");
    require(downwell::advanceMainRoomActivation(frontier, 300000L * 256, 7,
                read, activate) == 1 && frontier == 32 && order[3] == 1,
            "distant camera does not catch up multiple rows in one Begin Step");
    require(downwell::advanceMainRoomActivation(frontier, 300000L * 256, 7,
                read, activate) == 0 && frontier == 48 && writes == 4,
            "already activated instances are not activated twice");
    frontier = 299984;
    require(downwell::advanceMainRoomActivation(frontier, 300000L * 256, 0,
                read, activate) == 0 && frontier == 300000,
            "empty stores still advance and serialized room coordinates fit 32 bits");
    std::cout << "shared main room activation tests passed\n";
}
