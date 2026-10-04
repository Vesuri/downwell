#ifndef DOWNWELL_AMIGA_PREPARED_TIME_FIELDS_H
#define DOWNWELL_AMIGA_PREPARED_TIME_FIELDS_H
#include "AmigaGoalField.h"

// Ops captures into a 64x56 two-plane backing and mask-blits from that layout
// into a 384x320 game ring. Native Ops uses the shared Bitmap implementation;
// the test backend independently copies pixels. Images never change at runtime.
template<class Ops> class AmigaPreparedTimeFieldPair {
    using Reference = AmigaFieldOverlay<downwell::TimeFieldOutlines,true>;
    static unsigned word(const UBYTE* p) { return (unsigned(p[0])<<8)|p[1]; }
public:
    static constexpr unsigned planeBytes=448, backingBytes=896, imageBytes=1344, headerBytes=1040;
    class Overlay {
        Reference& reference_;
        const UBYTE* pack_=nullptr;
        const UBYTE* image_=nullptr;
        UBYTE* backing_=nullptr;
        UBYTE* target_=nullptr;
        unsigned origin_=0;
        struct Rect { unsigned x,y,sx,sy,width,height; } rect_{};
        downwell::TimeFieldOutlines shape_;
        bool fast_=false,valid_=false,pending_=false;
        // Two transfers at most: the physical ring can wrap within the image.
        bool transfer(bool capture,const UBYTE* source) {
            unsigned physical=origin_+rect_.y;
            if(physical>=downwellGameRingHeight)physical-=downwellGameRingHeight;
            unsigned first=downwellGameRingHeight-physical;
            if(first>rect_.height)first=rect_.height;
            auto part=[&](unsigned row,unsigned sy,unsigned height){
                if(capture)return Ops::capture(backing_,target_,image_+2*planeBytes,
                    rect_.x,row,rect_.sx,sy,rect_.width,height);
                return Ops::masked(source,image_+2*planeBytes,target_,rect_.x,row,
                    rect_.sx,sy,rect_.width,height);
            };
            return part(physical,rect_.sy,first) && (first==rect_.height ||
                part(0,rect_.sy+first,rect_.height-first));
        }
    public:
        explicit Overlay(Reference& reference):reference_(reference) {}
        void bind(const UBYTE* pack,UBYTE* backing) { discard();pack_=pack;backing_=backing; }
        void discard() { fast_=valid_=pending_=false;target_=nullptr;reference_.discard(); }
        bool present() const { return fast_ ? valid_||pending_ : reference_.present(); }
        bool matches(UBYTE* target,unsigned origin,const downwell::TimeFieldOutlines& shape) const {
            if(!fast_)return reference_.matches(target,origin,shape);
            return valid_ && target==target_ && amigaFieldSameCoverage(shape,origin,shape_,origin_,
                downwellGameRingHeight,downwellDisplayHeight);
        }
        template<class DirtyRow> bool touchesDirtyRows(DirtyRow dirty) const {
            if(!fast_)return reference_.touchesDirtyRows(dirty);
            unsigned row=origin_+rect_.y;
            if(row>=downwellGameRingHeight)row-=downwellGameRingHeight;
            for(unsigned i=0;i<rect_.height;++i){
                if(dirty(row))return true;
                if(++row==downwellGameRingHeight)row=0;
            }
            return false;
        }
        template<class DirtyRow> bool prepare(UBYTE* target,unsigned origin,
            const downwell::TimeFieldOutlines& shape,bool underlyingChanged,DirtyRow dirty) {
            if(origin>=downwellGameRingHeight)return false;
            if(target!=target_){discard();target_=target;}
            if(matches(target,origin,shape)&&!underlyingChanged)return true;
            // The sparse reference owns kept bits after prepare. Materialize a
            // pending fallback before replacing it so its underlay is captured.
            if(!fast_)reference_.draw();
            Rect next{};
            const UBYTE* image=nullptr;
            if(pack_&&shape.visible&&shape.count==1){
                const auto& circle=shape.circles[0];
                const int phase=circle.radius-24*256;
                if(phase>=0&&phase<512&&Ops::enabled()){
                    const int left=circle.x-32,top=circle.y-28;
                    const int x=left<downwellPlayfieldX?downwellPlayfieldX:left;
                    const int y=top<0?0:top;
                    const int right=left+64>downwellPlayfieldX+downwellPlayfieldWidth?
                        downwellPlayfieldX+downwellPlayfieldWidth:left+64;
                    const int bottom=top+56>downwellDisplayHeight?downwellDisplayHeight:top+56;
                    if(x<right&&y<bottom){
                        next={unsigned(x),unsigned(y),unsigned(x-left),unsigned(y-top),
                              unsigned(right-x),unsigned(bottom-y)};
                        image=pack_+headerBytes+word(pack_+16+phase*2)*imageBytes;
                    }
                }
            }
            if(fast_){
                if(valid_&&!transfer(false,backing_))return false;
                fast_=valid_=pending_=false;
            }
            if(!image)return reference_.prepare(target,origin,shape,underlyingChanged,dirty);
            if(reference_.present()){
                downwell::TimeFieldOutlines empty;
                if(!reference_.prepare(target,origin,empty,true,dirty))return false;
                reference_.discard();
            }
            image_=image;rect_=next;origin_=origin;
            // Only the admitted circle is needed; do not copy 65 unused slots.
            shape_.visible=true;shape_.count=1;shape_.circles[0]=shape.circles[0];
            fast_=pending_=true;
            return true;
        }
        bool draw() {
            if(!fast_){reference_.draw();return true;}
            if(!pending_)return true;
            if(!transfer(true,nullptr)||!transfer(false,image_))return false;
            pending_=false;valid_=true;return true;
        }
        bool preparedImage() const { return fast_; }
    };
private:
    AmigaTimeFieldPair reference_;
    Overlay overlays_[2]={Overlay(reference_[0]),Overlay(reference_[1])};
public:
    static unsigned storageBytes(unsigned packBytes) { return ((packBytes+3)&~3u)+2*backingBytes; }
    bool bind(UBYTE* region,unsigned bytes,const UBYTE* pack,unsigned packBytes) {
        if(!region||!pack||packBytes<headerBytes||packBytes>headerBytes+65535u*imageBytes||
            bytes<storageBytes(packBytes)||
            pack[0]!='D'||pack[1]!='W'||pack[2]!='F'||pack[3]!='I'||word(pack+4)!=1||
            word(pack+8)!=64||word(pack+10)!=56||word(pack+12)!=32||word(pack+14)!=28)return false;
        const unsigned count=word(pack+6);
        if(!count||packBytes!=headerBytes+count*imageBytes)return false;
        for(unsigned i=0;i<512;++i)if(word(pack+16+i*2)>=count)return false;
        for(auto& overlay:overlays_)overlay.discard();
        for(unsigned i=0;i<packBytes;++i)region[i]=pack[i];
        UBYTE* backing=region+((packBytes+3)&~3u);
        overlays_[0].bind(region,backing);overlays_[1].bind(region,backing+backingBytes);
        return true;
    }
    Overlay& operator[](unsigned i) { return overlays_[i]; }
    Overlay* begin() { return overlays_; }
    Overlay* end() { return overlays_+2; }
};
#endif
