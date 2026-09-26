#include "core/CorpseVerification.h"
#include <cassert>
#include <cstdio>
int main(){
    const auto result=downwell::verifyCorpses();
    assert(result.checks==1184&&result.failures==0&&result.digest==0x9a034aa1u);
    const auto repeated=downwell::verifyCorpses();
    assert(repeated.checks==result.checks&&repeated.failures==0&&repeated.digest==result.digest);
    std::puts("Corpse verification: 1184 checks, deterministic digest 9a034aa1");
}
