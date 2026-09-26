#include "core/CorpseVerification.h"
#include <cassert>
#include <cstdio>
int main(){
    const auto result=downwell::verifyCorpses();
    assert(result.checks==2464&&result.failures==0&&result.digest==0xfc743d8du);
    const auto repeated=downwell::verifyCorpses();
    assert(repeated.checks==result.checks&&repeated.failures==0&&repeated.digest==result.digest);
    std::puts("Corpse verification: 2464 checks, deterministic digest fc743d8d");
}
