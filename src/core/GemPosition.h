#ifndef DOWNWELL_CORE_GEM_POSITION_H
#define DOWNWELL_CORE_GEM_POSITION_H
namespace downwell {
// Exact 1/12800-pixel positions, normalized on writes. Readers use pixel or
// Q8 units without dividing the full coordinate again. Velocities remain in
// their original units; this does not approximate the movement algorithm.
class GemPosition {
    long whole_=0;
    unsigned short fraction_=0,q8Fraction_=0;
    void refresh() {
        // The old Q8 conversion rounds halves away from zero, unlike the
        // ties-to-even mask position. Negative coordinates need the lower tie.
        q8Fraction_=static_cast<unsigned short>((fraction_+(whole_<0?24u:25u))/50u);
    }
public:
    GemPosition()=default;
    GemPosition(long value) {*this=value;}
    GemPosition& operator=(long value) {
        whole_=value/12800;
        long remainder=value%12800;
        if(remainder<0){--whole_;remainder+=12800;}
        fraction_=static_cast<unsigned short>(remainder);refresh();return *this;
    }
    void setPixel(long value) {whole_=value;fraction_=q8Fraction_=0;}
    long pixel()const{return whole_;}
    long rounded()const {
        return whole_+(fraction_>6400 || (fraction_==6400 && whole_%2!=0));
    }
    long q8()const{return whole_*256+q8Fraction_;}
    // Compatibility for digests and source-domain tests, never a hot reader.
    operator long()const {
        return whole_<0?(whole_+1)*12800-(12800-fraction_):whole_*12800+fraction_;
    }
    GemPosition& operator+=(long delta) {
        long remainder;
        // Live currency speeds are at most nine pixels. Keep a general exact
        // fallback for external writes, without unbounded normalization loops.
        if(delta < -16*12800L || delta > 16*12800L) {
            whole_+=delta/12800;remainder=static_cast<long>(fraction_)+delta%12800;
        } else remainder=static_cast<long>(fraction_)+delta;
        while(remainder<0){--whole_;remainder+=12800;}
        while(remainder>=12800){++whole_;remainder-=12800;}
        fraction_=static_cast<unsigned short>(remainder);refresh();return *this;
    }
    GemPosition operator+(long delta)const {GemPosition result=*this;result+=delta;return result;}
};
}
#endif
