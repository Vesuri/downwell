#include "platform/amiga/AmigaTransientBank.h"
#include <cassert>
#include <cstring>

int main() {
    using Bank = AmigaTransientBank;
    using Owner = Bank::Owner;
    Bank bank;
    unsigned char storage[32]{};
    const Bank::Contract shop{Owner::shop, "shop", 16, 24};
    const Bank::Contract results{Owner::results, "results", 24, 24};
    unsigned reads = 0, validations = 0;
    bool readOk = true, valid = true;
    auto read = [&](const char*, unsigned char* p, unsigned n) {
        ++reads; std::memset(p, 0x5a, n); return readOk;
    };
    auto validate = [&](const unsigned char*, unsigned) { ++validations; return valid; };
    auto load = [&](Owner owner, const Bank::Contract& c, unsigned capacity=32) {
        return bank.load(owner, c, storage, capacity, read, validate);
    };
    assert(!bank.view(Owner::shop).data);
    assert(!load(Owner::results, shop) && reads == 0);
    assert(!load(Owner::none, shop) && reads == 0);
    assert(!load(static_cast<Owner>(99), {static_cast<Owner>(99),"future",16,24}) && reads == 0);
    assert(!load(Owner::shop, shop, 15) && reads == 0);
    assert(!load(Owner::shop, {Owner::shop,"shop",25,24}) && reads == 0);
    assert(load(Owner::shop, shop) && reads == 1 && validations == 1);
    assert(bank.view(Owner::shop).data == storage && bank.view(Owner::shop).bytes == 16);
    assert(!bank.view(Owner::results).data);
    // A held scene incurs no asset work, regardless of how many frames pass.
    for (unsigned i=0; i<1800; ++i) assert(load(Owner::shop, shop));
    assert(reads == 1 && validations == 1);
    // Declaring an unrelated future bank must not change this scene's work.
    const Bank::Contract upgrade{Owner::upgrade, "upgrade", 32, 32};
    (void)upgrade;
    assert(load(Owner::shop, shop) && reads == 1);
    assert(load(Owner::results, results) && reads == 2);
    assert(!bank.view(Owner::shop).data);
    bank.release();
    assert(!bank.view(Owner::results).data);
    readOk = false;
    assert(!load(Owner::shop, shop) && !bank.view(Owner::shop).data);
    readOk = true; valid = false;
    assert(!load(Owner::results, results) && !bank.view(Owner::results).data);
    valid = true;
    assert(load(Owner::shop, shop));
    readOk = false;
    assert(!load(Owner::results, results));
    assert(!bank.view(Owner::shop).data && !bank.view(Owner::results).data);
}
