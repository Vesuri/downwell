#ifndef DOWNWELL_AMIGA_TRANSIENT_BANK_H
#define DOWNWELL_AMIGA_TRANSIENT_BANK_H

#include <cstdint>

// A scene owns this lease, not the renderer. Retire its blits/DMA before
// release/replacement. Failed I/O leaves no readable lease (not the old art).
// This covers transient presentation art, not gameplay's resource catalogue.
class AmigaTransientBank {
public:
    enum class Owner { none, shop, results, upgrade };
    struct Contract {
        Owner owner;
        const char* file;
        unsigned bytes, budget;
    };
    struct View { const std::uint8_t* data; unsigned bytes; };

    template<class Read, class Validate>
    bool load(Owner owner, const Contract& contract, std::uint8_t* target,
              unsigned capacity, Read read, Validate validate) {
        // Closed admission: extending Owner does not implicitly enable loading.
        if ((owner != Owner::shop && owner != Owner::results && owner != Owner::upgrade) ||
            owner != contract.owner || !contract.file ||
            !target || !contract.bytes || contract.bytes > contract.budget ||
            contract.bytes > capacity) return false;
        // Contracts must be immutable and outlive the lease.
        if (owner_ == owner && data_ == target && contract_ == &contract)
            return true;
        release();
        if (!read(contract.file, target, contract.bytes) ||
            !validate(target, contract.bytes)) return false;
        data_ = target; bytes_ = contract.bytes; owner_ = owner; contract_ = &contract;
        return true;
    }
    View view(Owner owner) const {
        return owner != Owner::none && owner == owner_ ? View{data_, bytes_} : View{nullptr, 0};
    }
    void release() { owner_ = Owner::none; data_ = nullptr; bytes_ = 0; contract_ = nullptr; }
private:
    Owner owner_ = Owner::none;
    const std::uint8_t* data_ = nullptr;
    unsigned bytes_ = 0;
    const Contract* contract_ = nullptr;
};
#endif
