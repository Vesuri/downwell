#ifndef DOWNWELL_AMIGA_TABLES_H
#define DOWNWELL_AMIGA_TABLES_H
// WI.3: the tables derived from the player's files, installed as
// Downwell.tables (tools/pack-amiga-tables.py) instead of being linked into
// the executable. Read whole once at startup into public memory, then bound
// by tag; the views stay valid until the owner is destroyed.
#include "framework/AmigaPlatform.h"
#include "AmigaDataFiles.h"
#include "../../../generated/amiga-tables.h"
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/exec.h>

class AmigaTables {
public:
    struct View { const UBYTE* data; ULONG bytes; };

    AmigaTables() = default;
    ~AmigaTables() { if (file_) FreeMem(file_, DOWNWELL_TABLES_BYTES); }

    // Read and checked whole; noMemory and wrongFile leave nothing held. A
    // missing or short file is wrongFile too (the start checks sizes first).
    enum Load { loaded, noMemory, wrongFile };
    Load load(AmigaPlatform& platform) {
        if (file_) return loaded;
        file_ = static_cast<UBYTE*>(AllocMem(DOWNWELL_TABLES_BYTES, MEMF_PUBLIC));
        if (!file_) return noMemory;
        if (platform.loadData(AmigaDataFile::tables, file_, DOWNWELL_TABLES_BYTES) != LONG(DOWNWELL_TABLES_BYTES) ||
            word(0) != 0x4457u || word(2) != 0x5442u || word(4) != DOWNWELL_TABLES_VERSION ||
            word(6) != DOWNWELL_TABLES_COUNT) {
            FreeMem(file_, DOWNWELL_TABLES_BYTES);
            file_ = nullptr;
            return wrongFile;
        }
        return loaded;
    }

    // The table with this tag, or {nullptr, 0}.
    View find(ULONG tag) const {
        if (!file_) return {nullptr, 0};
        for (ULONG i = 0; i < DOWNWELL_TABLES_COUNT; ++i) {
            const ULONG entry = 8 + 12 * i;
            if (longword(entry) != tag) continue;
            const ULONG offset = longword(entry + 4), bytes = longword(entry + 8);
            if (offset > DOWNWELL_TABLES_BYTES || bytes > DOWNWELL_TABLES_BYTES - offset) break;
            return {file_ + offset, bytes};
        }
        return {nullptr, 0};
    }

    // The same table, writable: only the startup bind relocates in place.
    UBYTE* writable(ULONG tag, ULONG& bytes) {
        const View view = find(tag);
        bytes = view.bytes;
        return const_cast<UBYTE*>(view.data);
    }

private:
    ULONG word(ULONG at) const { return ULONG(file_[at]) << 8 | file_[at + 1]; }
    ULONG longword(ULONG at) const { return word(at) << 16 | word(at + 2); }
    AmigaTables(const AmigaTables&);
    AmigaTables& operator=(const AmigaTables&);

    UBYTE* file_ = nullptr;
};

#endif
