#include "platform/amiga/AmigaGameplayRows.h"
#include "platform/amiga/framework/Bitmap.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

struct Copy { unsigned x,y,w,h; };
static std::vector<Copy> copies;
static unsigned drains;
static bool nasty;
unsigned amigaRowSyncVisited;
void AmigaHardware::setBlitterNasty(bool enabled) { nasty=enabled; }
void AmigaHardware::blitterDrain() { assert(nasty); ++drains; }
Bitmap::Bitmap(void* p,uint16_t w,uint16_t h,uint16_t planes,bool inter,bool own,uint16_t dma)
    : data(p),width(w),height(h),bitplanes(planes),dataWidth(dma),widthInBytes(dma/8),
      rowSizeInBytes(dma/8),bitplaneSizeInBytes(dma/8*h),interleaved(inter),owner(own),blittable(false) {}
Bitmap::~Bitmap() {}
void Bitmap::copy(const Bitmap& source,uint16_t x,uint16_t y,uint16_t sx,uint16_t sy,
    uint16_t w,uint16_t h,uint16_t mask) {
    assert(x==sx && y==sy && !(x%16) && !(w%16) && mask==0xffff);
    assert(y+h<=height && x+w<=dataWidth);
    copies.push_back({x,y,w,h});
    for(unsigned p=0;p<2;++p) for(unsigned row=y;row<y+h;++row) {
        const unsigned at=p*bitplaneSizeInBytes+row*widthInBytes+x/8;
        std::memcpy(static_cast<unsigned char*>(data)+at,
                    static_cast<const unsigned char*>(source.data)+at,w/8);
    }
}
int main() {
    // Exhaust every word shape and search start against a scalar oracle.
    // Run boundaries, not just final pixels, determine the DMA submissions.
    for(unsigned bits=0;bits<65536;++bits){
        AmigaRowDebt<16> debt;
        for(unsigned row=0;row<16;++row)if(bits&(1u<<row))debt.assign(row,row+1,true);
        for(unsigned first=0;first<=16;++first)for(bool owed:{false,true}){
            unsigned expected=first;
            while(expected<16&&bool(bits&(1u<<expected))!=owed)++expected;
            assert(debt.next(first,16,owed)==expected);
        }
    }
    std::puts("Row debt: all 65536 word shapes and every search start match scalar runs");
    constexpr unsigned rows=downwellGameRingHeight, bytes=downwellGameRingBytes;
    std::array<unsigned char,bytes> backing{}, pixels[2]{}, expected[2]{};
    bool owed[2][rows]{};
    Bitmap source(backing.data(),380,rows,2,false,false,384);
    Bitmap targets[2]={{pixels[0].data(),380,rows,2,false,false,384},
                       {pixels[1].data(),380,rows,2,false,false,384}};
    AmigaGameplayRows tracker;
    for(unsigned b=0;b<2;++b) tracker.sync(targets[b],source,b);
    assert(copies.empty() && drains==0);
    auto initialize=[&] {
        tracker.reset();
        tracker.damage(0,0,rows);
        for(unsigned b=0;b<2;++b) {
            copies.clear(); tracker.sync(targets[b],source,b);
            assert(copies.size()==1 && copies[0].x==0 && copies[0].w==384 && copies[0].h==rows);
            expected[b]=backing;
            for(unsigned y=0;y<rows;++y) { owed[b][y]=false; assert(!tracker.dirty(y,b)); }
        }
    };
    initialize();
    tracker.damage(rows-8,0,16);
    for(unsigned b=0;b<2;++b){
        copies.clear();amigaRowSyncVisited=0;
        tracker.sync(targets[b],source,b);
        assert(copies.size()==2&&copies[0].y==0&&copies[0].h==8&&
               copies[1].y==rows-8&&copies[1].h==8);
#if defined(DOWNWELL_ROW_SYNC_AUDIT) && defined(DOWNWELL_FRAME_WORK_CANDIDATE)
        assert(amigaRowSyncVisited<=48); // The clean middle is not visited.
#endif
    }
    initialize();
    unsigned random=17;
    auto next=[&] { random=random*1664525u+1013904223u; return random; };
    for(unsigned tick=0;tick<10000;++tick) {
        if(tick && tick%1000==0) initialize();
        const unsigned origin=next()%rows, y=next()%284, height=1+next()%rows;
        const unsigned buffers=1+next()%3;
        tracker.damage(origin,y,height,buffers);
        for(unsigned n=0;n<height;++n) {
            const unsigned row=(origin+y+n)%rows;
            for(unsigned b=0;b<2;++b) if(buffers&(1u<<b)) owed[b][row]=true;
            // Backing changes only in the 176-pixel terrain strip.
            for(unsigned p=0;p<2;++p) for(unsigned x=12;x<34;++x)
                backing[p*rows*48+row*48+x]=static_cast<unsigned char>(next());
        }
        const unsigned b=next()%2;
        if(tick%3==0) {
            const unsigned first=next()%rows, count=1+next()%(rows-first);
            // A completed pipeline copy has already installed these rows.
            targets[b].copy(source,96,first,96,first,176,count);
            for(unsigned r=first;r<first+count;++r) {
                owed[b][r]=false;
                for(unsigned p=0;p<2;++p) std::memcpy(expected[b].data()+p*rows*48+r*48+12,
                    backing.data()+p*rows*48+r*48+12,22);
            }
            tracker.copied(first,count,b);
        }
        for(unsigned r=0;r<rows;++r) {
            assert(tracker.dirty(r,b)==owed[b][r]);
            if(owed[b][r]) for(unsigned p=0;p<2;++p)
                std::memcpy(expected[b].data()+p*rows*48+r*48+12,backing.data()+p*rows*48+r*48+12,22);
        }
        copies.clear(); tracker.sync(targets[b],source,b);
        unsigned copied=0, required=0;
        for(const auto& c:copies) {
            assert(c.x==96 && c.w==176);
            for(unsigned r=c.y;r<c.y+c.h;++r) { assert(owed[b][r]); ++copied; }
        }
        for(unsigned r=0;r<rows;++r) { required+=owed[b][r]; owed[b][r]=false; assert(!tracker.dirty(r,b)); }
        assert(copied==required && pixels[b]==expected[b] && !nasty);
        copies.clear(); const auto before=drains;
        tracker.sync(targets[b],source,b);
        assert(copies.empty() && drains==before); // unchanged ring costs no DMA
    }
    std::puts("Gameplay rows: 10000 wrap/damage/pipeline cases, exact copied rows and untouched margins");
}
