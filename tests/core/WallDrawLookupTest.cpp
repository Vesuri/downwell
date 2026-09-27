#include "core/WallDrawLookup.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>

struct Record { int objectId, x, y; bool active; };
static void require(bool ok) { if (!ok) std::abort(); }
int main() {
    std::array<Record, 2048> records{};
    downwell::WallDrawLookup<2048> lookup;
    for (unsigned n : {128u, 512u, 2048u}) {
        for (unsigned i = 0; i < n; ++i)
            records[i] = {i % 7 == 0 ? 92 : 60, int(i % 16)*16-128,
                          int(i / 16)*16-64, i % 11 != 0};
        // Include duplicate positions, inactive records and non-wall objects.
        records[n-1] = records[1];
        auto at = [&](std::size_t i) -> const Record& { return records[i]; };
        auto old = [&](int x, int y) {
            for (unsigned i=0; i<n; ++i)
                if(records[i].active && records[i].objectId==60 &&
                   records[i].x==x && records[i].y==y) return true;
            return false;
        };
        lookup.rebuild(n, at);
        for (unsigned i=0;i<n;++i)
            for (auto delta : {std::make_pair(0,-16),{16,0},{0,16},{-16,0}})
                require(old(records[i].x+delta.first, records[i].y+delta.second)==
                    lookup.contains(records[i].x+delta.first, records[i].y+delta.second));
        auto bench = [&](bool indexed) {
            unsigned checksum=0;
            const auto start=std::chrono::steady_clock::now();
            for(unsigned repeat=0;repeat<100;++repeat) {
                if(indexed) lookup.rebuild(n,at);
                for(unsigned i=0;i<n;++i) {
                    if(!records[i].active || records[i].objectId!=60) continue;
                    for(auto delta : {std::make_pair(0,-16),{16,0},{0,16},{-16,0}}) {
                        const int x=records[i].x+delta.first,y=records[i].y+delta.second;
                        checksum += indexed ? lookup.contains(x,y) : old(x,y);
                    }
                }
            }
            const double ms=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-start).count()/100;
            std::printf("%u records %s: %.4f ms/frame checksum=%u\n",n,
                        indexed?"indexed":"linear",ms,checksum);
            return checksum;
        };
        require(bench(false)==bench(true));
        records[1].active=false; records[n-1].active=false;
        lookup.rebuild(n,at);
        require(!lookup.contains(records[1].x,records[1].y));
        records[1] = {60, -1001, 99999, true};
        lookup.rebuild(n,at);
        require(lookup.contains(-1001,99999));
        records[1].objectId = 92;
        lookup.rebuild(n,at);
        require(!lookup.contains(-1001,99999));
    }
    lookup.rebuild(0, [&](std::size_t i)->const Record& { return records[i]; });
    require(!lookup.contains(0,0));
}
