#ifndef DOWNWELL_AMIGA_TERRAIN_RASTER_H
#define DOWNWELL_AMIGA_TERRAIN_RASTER_H
#include "AmigaGameplayRows.h"
#include "AmigaInitialTerrain.h"
#include "core/ShopWorld.h"
class Bitmap;
// Fast-RAM raster for the left room, well and right room. Transfer only
// exposed/invalidated spans into the Chip-RAM circular backing/display rings.
class AmigaTerrainRaster {
    using PixelColumn = UWORD[8192][2][AmigaGameplayRows::terrainWords];
    PixelColumn* pixels_=nullptr;
    // Bit 0: repaint required. Bit 1: current pixels belong to the immutable
    // initial-room dictionary, not pixels_. Bit 2: verified corridor row.
    // Writes replace a complete row; mutations invalidate either borrowed form.
    unsigned char dirty_[3][8192]{};
    const AmigaInitialTerrain* initial_=nullptr;
    unsigned shopRevision_[downwell::ShopWorld::capacity]{};
public:
    static constexpr unsigned rows = 8192, words = AmigaGameplayRows::terrainWords;
    static constexpr unsigned storageBytes = 3*sizeof(PixelColumn);
    // Borrow gameplay-only ordinary memory. A live lease cannot be replaced;
    // retirement invalidates every cached/borrowed row before storage is freed.
    bool bindStorage(void* storage,unsigned bytes){
        if(!storage||bytes!=storageBytes||pixels_)return false;
        pixels_=static_cast<PixelColumn*>(storage);
        initial_=nullptr;ready=false;
        for(auto& column:dirty_)for(auto& row:column)row=0;
        for(auto& revision:shopRevision_)revision=0;
        return true;
    }
    void releaseStorage(){ready=false;initial_=nullptr;pixels_=nullptr;}
    bool hasStorage()const{return pixels_!=nullptr;}
    static constexpr unsigned x = AmigaGameplayRows::terrainX;
    static constexpr unsigned stagePaintRows = 16;
    enum StagePaint { stageFailed=0, stageInBacking=1, stageInCache=2 };
    bool ready = false;
    UWORD* writeRows(unsigned column,unsigned y,unsigned height){
#ifndef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
        for(unsigned i=y;i<y+height;++i)dirty_[column][i]&=1;
#else
        (void)height;
#endif
        return &pixels_[column][y][0][0];
    }
    UWORD* row(unsigned column, unsigned y) { return writeRows(column,y,1); }
    const UWORD* readRow(unsigned column,unsigned y,UWORD* scratch)const{
        if(dirty_[column][y]&4)return initial_->corridorRow(y,scratch);
        return (dirty_[column][y]&2)?initial_->row(column,y,scratch):&pixels_[column][y][0][0];
    }
    // Caller has verified the live index and all additional paint producers.
    // This only publishes pixels, never terrain instances or future geometry.
    bool borrowCorridor(unsigned first,unsigned height){
#ifdef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
        (void)first;(void)height;return false;
#else
        if(!initial_||!initial_->hasCorridor()||first>=rows||height>rows-first)return false;
        for(unsigned y=first;y<first+height;++y)dirty_[1][y]=4;
        return true;
#endif
    }
    bool borrowInitial(const AmigaInitialTerrain& initial){
        if(!pixels_||!initial.rows()||initial.rows()>rows)return false;
        initial_=&initial;
        for(unsigned c=0;c<3;++c)for(unsigned y=0;y<initial.rows();++y)dirty_[c][y]=(dirty_[c][y]&1)|2;
        return true;
    }
    bool dirty(unsigned column, unsigned y)const { return (dirty_[column][y]&1)!=0; }
    void setDirty(unsigned column,unsigned y,bool value){
#ifdef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
        dirty_[column][y]=value;
#else
        dirty_[column][y]=(dirty_[column][y]&6)|(value?1:0);
#endif
    }
    // First row after the homogeneous clean/dirty run. Invalid/empty requests
    // do not advance. Query before repainting: the caller may clear dirty bits.
    unsigned runEnd(unsigned column, unsigned first, unsigned end) const {
        if(column>=3 || first>=end || end>rows)return first;
        const bool state=dirty(column,first);
        unsigned next=first+1;
        while(next<end && dirty(column,next)==state)++next;
        return next;
    }
    void invalidate(int first, int end);
    void invalidateShops(const downwell::ShopWorld& shops);
    // Populate retired staging storage only. Paint must finish all DMA before
    // returning; failed paint leaves that run dirty. A producer may return
    // stageInCache to publish directly composed raster rows without reading
    // stale staging pixels back over them. Legacy bool producers retain the
    // stageInBacking contract. Never acknowledge either
    // live display's debts or use its pipelined copy target here.
    template<class Paint>
    int prepareStage(AmigaRenderer* renderer, unsigned column, unsigned worldY,
                     unsigned height, AmigaGameplayRows& debts, Paint paint) {
        if (!ready || column >= 3 || worldY >= rows || height > 48 ||
            height > rows - worldY) return -1;
        unsigned done = 0, budget = stagePaintRows;
        while (done < height) {
            const unsigned first = worldY + done;
            unsigned count = runEnd(column, first, worldY + height) - first;
            const bool repaint = dirty(column,first);
            int painted=stageInBacking;
            if (repaint) {
                if (!budget) break;
                if (count > budget) count = budget;
                painted=paint(done,count);
                if(painted!=stageInBacking&&painted!=stageInCache)return -1;
                budget -= count;
            }
            transfer(renderer, column, first, done, count, repaint&&painted==stageInBacking,
                     nullptr, nullptr, 0, debts);
            if (repaint)
                for (unsigned y = first; y < first + count; ++y) setDirty(column,y,false);
            done += count;
        }
        return int(done);
    }
    void transfer(AmigaRenderer* renderer, unsigned column, int worldY,
        int screenY, int height, bool store, Bitmap* pipelineTarget,
        const Bitmap* backing, unsigned buffer, AmigaGameplayRows& debts);
};
#endif
