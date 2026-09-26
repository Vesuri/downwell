#include "core/MainRoomActivation.h"
#include <cstdlib>
#include <iostream>

static void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}

int main() {
    using downwell::ActivationInstance;
    ActivationInstance records[] = {
        {111, {236,244,12,20}, false, true},
        {112, {232,248,16,28}, false, true},
        {113, {234,246,0,8}, true, true},
        {116, {234,246,0,8}, false, false},
        {60, {234,246,0,8}, false, true},
        {117, {-7,7,0,8}, false, true},
        {119, {479,489,0,8}, false, true}
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
                read, activate) == 4 && frontier == 16,
            "strict view crossing advances exactly one row and activates eligible enemies");
    require(writes == 4 && order[0] == 0 && order[1] == 1 && order[2] == 5 && order[3] == 6,
            "activation preserves storage order and excludes active/retired/non-enemy records");
    require(!records[0].pending && records[1].active && !records[1].pending,
            "original runner includes a mask whose top equals the activation frontier");
    require(downwell::advanceMainRoomActivation(frontier, 300000L * 256, 7,
                read, activate) == 0 && frontier == 32 && writes == 4,
            "distant camera does not catch up multiple rows in one Begin Step");
    require(downwell::advanceMainRoomActivation(frontier, 300000L * 256, 7,
                read, activate) == 0 && frontier == 48 && writes == 4,
            "already activated instances are not activated twice");
    ActivationInstance edges[] = {
        {111, {-8,0,0,8}, false,true}, {111,{480,488,0,8},false,true},
        {111, {0,8,-8,0}, false,true}, {111,{0,8,0,0},false,true},
        {111, {0,8,15,23},false,true}, {111,{0,8,16,24},false,true},
        {111, {481,489,0,8},false,true}, {111,{0,8,17,25},false,true},
        {111, {-8,1,0,8},false,true}, {111,{0,8,-8,1},false,true}
    };
    frontier = 0;
    unsigned edgeMask = 0;
    const unsigned touchingOrInside = (1u<<1) | (1u<<4) | (1u<<5) | (1u<<8) | (1u<<9);
    require(downwell::advanceMainRoomActivation(frontier, 318L*256, 10,
        [&](unsigned i) { return edges[i]; }, [&](unsigned i) { edgeMask |= 1u << i; }) == 5 && edgeMask == touchingOrInside,
        "activation includes all four touching pixel edges but excludes the adjacent outside pixels");
    frontier = 299984;
    require(downwell::advanceMainRoomActivation(frontier, 300000L * 256, 0,
                read, activate) == 0 && frontier == 300000,
            "empty stores still advance and serialized room coordinates fit 32 bits");
    std::cout << "shared main room activation tests passed\n";
}
