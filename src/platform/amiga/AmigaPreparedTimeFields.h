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
        const UBYTE* terrain_=nullptr;
        unsigned origin_=0;
        struct Rect { unsigned x,y,sx,sy,width,height; } rect_{};
        struct Entry { Rect rect;const UBYTE* image;unsigned circle;bool large; } entries_[66];
        UWORD largeRows_[downwellGameRingHeight/16]{};
        unsigned entryCount_=0;
        bool multiple_=false;
        downwell::TimeFieldOutlines shape_;
        bool fast_=false,valid_=false,pending_=false,staticUnderlay_=false;
        bool composite_=false;
        downwell::TimeFieldOutlines::Circle compositeOld_{};
        // Two transfers at most: the physical ring can wrap within the image.
        bool transfer(bool capture,const UBYTE* source) {
            unsigned physical=origin_+rect_.y;
            if(physical>=downwellGameRingHeight)physical-=downwellGameRingHeight;
            unsigned first=downwellGameRingHeight-physical;
            if(first>rect_.height)first=rect_.height;
            auto part=[&](unsigned row,unsigned sy,unsigned height){
                if(!capture&&source==backing_&&staticUnderlay_)
                    return Ops::restoreTerrain(terrain_,image_+2*planeBytes,target_,
                        rect_.x,row,rect_.sx,sy,rect_.width,height);
                if(capture)return Ops::capture(backing_,target_,image_+2*planeBytes,
                    rect_.x,row,rect_.sx,sy,rect_.width,height);
                return Ops::masked(source,image_+2*planeBytes,target_,rect_.x,row,
                    rect_.sx,sy,rect_.width,height);
            };
            return part(physical,rect_.sy,first) && (first==rect_.height ||
                part(0,rect_.sy+first,rect_.height-first));
        }
        const UBYTE* describe(const downwell::TimeFieldOutlines::Circle& circle,Rect& rect) const {
            const int left=circle.x-32,top=circle.y-28;
            const int x=left<downwellPlayfieldX?downwellPlayfieldX:left;
            const int y=top<0?0:top;
            const int right=left+64>downwellPlayfieldX+downwellPlayfieldWidth?
                downwellPlayfieldX+downwellPlayfieldWidth:left+64;
            const int bottom=top+56>downwellDisplayHeight?downwellDisplayHeight:top+56;
            if(x>=right||y>=bottom)return nullptr;
            rect={unsigned(x),unsigned(y),unsigned(x-left),unsigned(y-top),unsigned(right-x),unsigned(bottom-y)};
            return pack_+headerBytes+word(pack_+16+(circle.radius-24*256)*2)*imageBytes;
        }
        void select(unsigned i){rect_=entries_[i].rect;image_=entries_[i].image;}
        bool transferEntry(unsigned i,bool restore){
            if(entries_[i].large)return Ops::columns(terrain_,target_,shape_.circles[entries_[i].circle],
                origin_,restore,restore?nullptr:largeRows_);
            select(i);return transfer(false,restore?backing_:image_);
        }
        void validate(){
            if constexpr(Ops::verify){
                unsigned row=origin_+rect_.y;
                if(row>=downwellGameRingHeight)row-=downwellGameRingHeight;
                unsigned first=downwellGameRingHeight-row;
                if(first>rect_.height)first=rect_.height;
                Ops::validateTerrain(terrain_,image_+2*planeBytes,target_,rect_.x,row,
                    rect_.sx,rect_.sy,rect_.width,first);
                if(first<rect_.height)Ops::validateTerrain(terrain_,image_+2*planeBytes,target_,rect_.x,0,
                    rect_.sx,rect_.sy+first,rect_.width,rect_.height-first);
            }
        }
    public:
        explicit Overlay(Reference& reference):reference_(reference) {}
        void bind(const UBYTE* pack,UBYTE* backing,const UBYTE* terrain) {
            discard();pack_=pack;backing_=backing;terrain_=terrain;
        }
        void discard() { fast_=valid_=pending_=multiple_=composite_=false;entryCount_=0;target_=nullptr;reference_.discard(); }
        bool present() const { return fast_ ? valid_||pending_ : reference_.present(); }
        bool matches(UBYTE* target,unsigned origin,const downwell::TimeFieldOutlines& shape) const {
            if(!fast_)return reference_.matches(target,origin,shape);
            return valid_ && target==target_ && amigaFieldSameCoverage(shape,origin,shape_,origin_,
                downwellGameRingHeight,downwellDisplayHeight);
        }
        template<class DirtyRow> bool touchesDirtyRows(DirtyRow dirty) const {
            if(!fast_)return reference_.touchesDirtyRows(dirty);
            if(multiple_)for(unsigned chunk=0;chunk<downwellGameRingHeight/16;++chunk){
                unsigned bits=largeRows_[chunk];
                for(unsigned row=chunk*16;bits;++row,bits>>=1)if((bits&1)&&dirty(row))return true;
            }
            for(unsigned e=0;e<(multiple_?entryCount_:1);++e){
                if(multiple_&&entries_[e].large)continue;
                const auto& rect=multiple_?entries_[e].rect:rect_;
                unsigned row=origin_+rect.y;
                if(row>=downwellGameRingHeight)row-=downwellGameRingHeight;
                for(unsigned i=0;i<rect.height;++i){
                    if(dirty(row))return true;
                    if(++row==downwellGameRingHeight)row=0;
                }
            }
            return false;
        }
        template<class DirtyRow> bool prepare(UBYTE* target,unsigned origin,
            const downwell::TimeFieldOutlines& shape,bool underlyingChanged,DirtyRow dirty,
            bool staticRectangleOwned=false) {
            if(origin>=downwellGameRingHeight)return false;
            if(target!=target_){discard();target_=target;}
            if(composite_&&pending_&&!draw())return false;
            if(matches(target,origin,shape)&&!underlyingChanged)return true;
            if(staticRectangleOwned&&fast_&&valid_&&multiple_&&entryCount_==1&&entries_[0].large&&
               shape_.count==1&&shape.visible&&shape.count==1&&origin==origin_&&Ops::compositeEnabled()&&
               Ops::retireOutside(terrain_,target_,shape_.circles[0],shape.circles[0],origin)){
                compositeOld_=shape_.circles[0];shape_.circles[0]=shape.circles[0];
                composite_=pending_=true;valid_=false;return true;
            }
            // The sparse reference owns kept bits after prepare. Materialize a
            // pending fallback before replacing it so its underlay is captured.
            if(!fast_)reference_.draw();
            Rect next{};
            const UBYTE* image=nullptr;
            bool many=pack_&&terrain_&&shape.visible&&shape.count>1&&shape.count<=66&&Ops::staticEnabled();
            if(many)for(unsigned i=0;i<shape.count;++i)
                if(shape.circles[i].radius<24*256||shape.circles[i].radius>=26*256){many=false;break;}
            if(pack_&&shape.visible&&shape.count==1){
                const auto& circle=shape.circles[0];
                const int phase=circle.radius-24*256;
                if(phase>=0&&phase<512&&Ops::enabled()){
                    image=describe(circle,next);
                }
            }
            if(!image&&!many&&pack_&&terrain_&&shape.visible&&shape.count&&shape.count<=66&&Ops::columnsEnabled()){
                many=true;
                for(unsigned i=0;i<shape.count;++i){
                    const int radius=shape.circles[i].radius;
                    if(!((radius>=24*256&&radius<26*256)||(radius>=128*256&&radius<130*256))){many=false;break;}
                }
            }
            if(fast_){
                if(valid_){
                    if(multiple_){
                        for(unsigned i=0;i<entryCount_;++i)if(!transferEntry(i,true))return false;
                    }else if(!transfer(false,backing_))return false;
                }
                fast_=valid_=pending_=false;
            }
            multiple_=false;entryCount_=0;
            if(!image&&!many)return reference_.prepare(target,origin,shape,underlyingChanged,dirty);
            if(reference_.present()){
                downwell::TimeFieldOutlines empty;
                if(!reference_.prepare(target,origin,empty,true,dirty))return false;
                reference_.discard();
            }
            image_=image;rect_=next;origin_=origin;
            staticUnderlay_=terrain_&&Ops::staticEnabled();
            multiple_=many;
            if(many)for(unsigned i=0;i<shape.count;++i){
                auto& entry=entries_[entryCount_];
                entry.circle=i;entry.large=shape.circles[i].radius>=128*256;
                if(entry.large){entry.image=nullptr;++entryCount_;continue;}
                entry.image=describe(shape.circles[i],entry.rect);
                if(entry.image)++entryCount_;
            }
            // Preserve source order; do not copy unused slots or capture a
            // different saved underlay for every overlapping circle.
            shape_.visible=true;shape_.count=shape.count;
            for(unsigned i=0;i<shape.count;++i)shape_.circles[i]=shape.circles[i];
            fast_=pending_=true;
            return true;
        }
        bool draw() {
            if(!fast_){reference_.draw();return true;}
            if(!pending_)return true;
            if(composite_){
                for(auto& rows:largeRows_)rows=0;
                if(!Ops::compose(terrain_,target_,compositeOld_,shape_.circles[0],origin_,largeRows_))return false;
                composite_=pending_=false;valid_=true;return true;
            }
            if(multiple_){
                // Validate all footprints before any earlier circle paints
                // over a later one's (legitimate) overlapping underlay.
                if constexpr(Ops::verify)for(unsigned i=0;i<entryCount_;++i){
                    if(entries_[i].large)Ops::validateColumns(terrain_,target_,shape_.circles[entries_[i].circle],origin_);
                    else{select(i);validate();}
                }
                for(auto& rows:largeRows_)rows=0;
                for(unsigned i=0;i<entryCount_;++i)if(!transferEntry(i,false))return false;
            }else{
                if(staticUnderlay_)validate();
                if((!staticUnderlay_&&!transfer(true,nullptr))||!transfer(false,image_))return false;
            }
            pending_=false;valid_=true;return true;
        }
        bool preparedImage() const { return fast_; }
    };
private:
    AmigaTimeFieldPair reference_;
    Overlay overlays_[2]={Overlay(reference_[0]),Overlay(reference_[1])};
    UBYTE* residentRegion_=nullptr;
    const UBYTE* residentPack_=nullptr;
    unsigned residentBytes_=0;
public:
    static unsigned storageBytes(unsigned packBytes) { return ((packBytes+3)&~3u)+2*backingBytes; }
    // The caller owns the shared Chip region. Invalidate before any other
    // scene writes it, even when that scene's load subsequently fails.
    void invalidateBank(){
        residentRegion_=nullptr;residentPack_=nullptr;residentBytes_=0;
        for(auto& overlay:overlays_)overlay.discard();
    }
    bool resident(const UBYTE* region,const UBYTE* pack,unsigned packBytes)const{
        return residentRegion_&&residentRegion_==region&&residentPack_==pack&&residentBytes_==packBytes;
    }
    bool bind(UBYTE* region,unsigned bytes,const UBYTE* pack,unsigned packBytes,const UBYTE* terrain=nullptr) {
        // Source bytes are immutable. Rebinding still discards both overlays:
        // no underlay or pending draw may survive the level boundary.
        if(resident(region,pack,packBytes)){
            if(bytes<storageBytes(packBytes))return false;
            UBYTE* backing=region+((packBytes+3)&~3u);
            overlays_[0].bind(region,backing,terrain);overlays_[1].bind(region,backing+backingBytes,terrain);
            return true;
        }
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
        overlays_[0].bind(region,backing,terrain);overlays_[1].bind(region,backing+backingBytes,terrain);
        residentRegion_=region;residentPack_=pack;residentBytes_=packBytes;
        return true;
    }
    Overlay& operator[](unsigned i) { return overlays_[i]; }
    Overlay* begin() { return overlays_; }
    Overlay* end() { return overlays_+2; }
};
#endif
