#ifndef DOWNWELL_CORE_MASK_BOUNDS_MEMO_H
#define DOWNWELL_CORE_MASK_BOUNDS_MEMO_H
#include "CurrencyCollisionBounds.h"
#include <cstdint>
namespace downwell {
// CurrencyCollisionBounds::calculateMask is a pure function of (mask, x, y),
// so its successes can be remembered under the whole key: a resting time
// shard, debris piece or corpse asks for the same few positions every
// update. Direct-mapped; a colliding key only replaces the entry. Failures
// are never stored and reach the caller unchanged.
class MaskBoundsMemo {
public:
    static constexpr unsigned entries=64;
    bool calculate(int mask,MotionBinary64 x,MotionBinary64 y,ContactBounds& out){
        Entry& e=entry_[index(mask,x,y)];
        if(e.mask==mask&&e.x.high==x.high&&e.x.low==x.low&&e.y.high==y.high&&e.y.low==y.low){
            out=e.bounds;return true;
        }
        if(!CurrencyCollisionBounds::calculateMask(mask,x,y,out))return false;
        e.mask=mask;e.x=x;e.y=y;e.bounds=out;return true;
    }
    void clear(){for(auto& e:entry_)e.mask=-1;}
    static unsigned index(int mask,MotionBinary64 x,MotionBinary64 y){
        std::uint32_t h=x.high^x.low^(y.high*3u)^y.low^std::uint32_t(mask);
        h^=h>>16;h^=h>>8;
        return h&(entries-1);
    }
private:
    struct Entry {int mask=-1;MotionBinary64 x{},y{};ContactBounds bounds{};};
    Entry entry_[entries];
};
}
#endif
