#ifndef DOWNWELL_CORE_STABLE_INSTANCE_SLOTS_H
#define DOWNWELL_CORE_STABLE_INSTANCE_SLOTS_H
namespace downwell {
// Preserve live creation order before appending births to fixed-capacity
// storage. Reusing an earlier hole would reorder subsequent Step traversal.
template<class Record>
unsigned compactInstanceSlots(Record* records, unsigned capacity) {
    unsigned write = 0;
    for (unsigned read = 0; read < capacity; ++read) {
        if (!records[read].active) continue;
        if (write != read) records[write] = records[read];
        ++write;
    }
    for (unsigned i = write; i < capacity; ++i) records[i].active = false;
    return write;
}
}
#endif
