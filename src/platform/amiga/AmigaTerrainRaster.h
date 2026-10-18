#ifndef DOWNWELL_AMIGA_TERRAIN_RASTER_H
#define DOWNWELL_AMIGA_TERRAIN_RASTER_H
#include "AmigaGameplayRows.h"
#include "AmigaInitialTerrain.h"
#include "AmigaPatternRow.h"
#include "core/ShopWorld.h"
#include <cstring>
class Bitmap;
// Fast-RAM raster for the left room, well and right room. Transfer only
// exposed/invalidated spans into the Chip-RAM circular backing/display rings.
class AmigaTerrainRaster {
    using PixelColumn = UWORD[8192][2][AmigaGameplayRows::terrainWords];
    PixelColumn* pixels_=nullptr;
    AmigaPatternRow* bands_=nullptr;
    // Bit 0: repaint required. Bit 1: current pixels belong to the immutable
    // initial-room dictionary, not pixels_. Bit 2: verified corridor row.
    // Bit 3: immutable prepared-pattern row (pointer in the pixel storage).
    // Bit 4: wall/tile strip references, merged directly at transfer.
    // Bit 5: one 16-line descriptor in the gameplay lease's band suffix.
    // Writes replace a complete row; mutations invalidate either borrowed form.
    unsigned char dirty_[3][8192]{};
    const AmigaInitialTerrain* initial_=nullptr;
    bool patternBorrowed_=false;
    unsigned prepareFirst_=8192,prepareEnd_=0;
    unsigned revision_=0;
    unsigned fallbackFirst_=rows,fallbackEnd_=0;
    unsigned shopRevision_[downwell::ShopWorld::capacity]{};
    bool shopRevisionsPopulated_=false;
public:
    static constexpr unsigned rows = 8192, words = AmigaGameplayRows::terrainWords;
#if defined(DOWNWELL_PLATFORM_AMIGA) && !defined(DOWNWELL_PREPARED_PATTERN_ROWS)
    static constexpr unsigned bandStorageBytes=0;
#else
    static constexpr unsigned bandStorageBytes=512*sizeof(AmigaPatternRow);
#endif
    static constexpr unsigned storageBytes = 3*sizeof(PixelColumn)+bandStorageBytes;
    // Borrow gameplay-only ordinary memory. A live lease cannot be replaced;
    // retirement invalidates every cached/borrowed row before storage is freed.
    bool bindStorage(void* storage,unsigned bytes){
        if(!storage||bytes!=storageBytes||pixels_)return false;
        pixels_=static_cast<PixelColumn*>(storage);
        bands_=bandStorageBytes?reinterpret_cast<AmigaPatternRow*>(pixels_+3):nullptr;
        initial_=nullptr;patternBorrowed_=false;ready=false;
        prepareFirst_=rows;prepareEnd_=0;
        fallbackFirst_=rows;fallbackEnd_=0;
        ++revision_;
        for(auto& column:dirty_)for(auto& row:column)row=0;
        for(auto& revision:shopRevision_)revision=0;
        shopRevisionsPopulated_=false;
        return true;
    }
    void releaseStorage(){ready=false;initial_=nullptr;pixels_=nullptr;bands_=nullptr;patternBorrowed_=false;
        prepareFirst_=rows;prepareEnd_=0;fallbackFirst_=rows;fallbackEnd_=0;}
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
        if(dirty_[column][y]&32){
            const auto source=bands_[(y+8)/16].offset((y+8)&15);
            if(source.tiles[0])source.write(scratch,scratch+words);
            else for(unsigned w=0;w<words*2;++w)scratch[w]=AmigaPatternRow::word(source.walls+w*2);
            return scratch;
        }
        if(dirty_[column][y]&16){
            AmigaPatternRow row;
            std::memcpy(&row,&pixels_[column][y][0][0],sizeof row);
            row.write(scratch,scratch+words);
            return scratch;
        }
        if(dirty_[column][y]&8){
            const unsigned char* source;
            std::memcpy(&source,&pixels_[column][y][0][0],sizeof source);
#ifdef DOWNWELL_PLATFORM_AMIGA
            return reinterpret_cast<const UWORD*>(source);
#else
            for(unsigned w=0;w<words*2;++w)scratch[w]=UWORD(unsigned(source[w*2])*256+source[w*2+1]);
            return scratch;
#endif
        }
        if(dirty_[column][y]&4)return initial_->corridorRow(y,scratch);
        return (dirty_[column][y]&2)?initial_->row(column,y,scratch):&pixels_[column][y][0][0];
    }
    // The immutable bank shares this gameplay lease. Reuse the row's existing
    // storage for its pointer; no parallel per-pixel ownership array is needed.
    void borrowPatternRow(unsigned column,unsigned y,const unsigned char* source){
        std::memcpy(&pixels_[column][y][0][0],&source,sizeof source);
        dirty_[column][y]=(dirty_[column][y]&1)|8;patternBorrowed_=true;
    }
    void borrowPatternRow(unsigned column,unsigned y,const AmigaPatternRow& source){
        std::memcpy(&pixels_[column][y][0][0],&source,sizeof source);
        dirty_[column][y]=(dirty_[column][y]&1)|16;patternBorrowed_=true;
    }
    // One immutable-art descriptor owns an entire admitted 16-line band.
    // Individual materialized replacements clear only their own flag; they
    // cannot overwrite the descriptor still used by neighbouring lines.
    bool borrowPatternBand(unsigned first,const AmigaPatternRow& source,bool clean=false){
        if(!bands_||first<8||first>rows-24||((first+8)&15)||!source.walls||
           bool(source.tiles[0])!=bool(source.tiles[1])||
           bool(source.tiles[0])!=bool(source.tiles[2]))return false;
        bands_[(first+8)/16]=source;
        for(unsigned y=first;y<first+16;++y)dirty_[1][y]=32|(clean?0:(dirty_[1][y]&1));
        patternBorrowed_=true;return true;
    }
    unsigned borrowedRunEnd(unsigned column,unsigned first,unsigned end)const{
        if(first<end&&(dirty_[column][first]&32)){
            const unsigned band=(first+8)/16;
            if(bands_[band].tiles[0])return first;
            unsigned next=first+1;
            while(next<end&&(next+8)/16==band&&(dirty_[column][next]&32))++next;
            return next;
        }
        if(first>=end||!(dirty_[column][first]&8))return first;
        const unsigned char* source;
        std::memcpy(&source,&pixels_[column][first][0][0],sizeof source);
        unsigned next=first+1;
        for(;next<end&&(dirty_[column][next]&8);++next){
            const unsigned char* row;
            std::memcpy(&row,&pixels_[column][next][0][0],sizeof row);
            if(row!=source+(next-first)*words*2*sizeof(UWORD))break;
        }
        return next;
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
    // Scene-entry preparation in the already allocated raster. Only dictionary
    // loans are materialized: live replacements and other borrowed forms keep
    // their owner and every row keeps its pending repaint bit.
    bool materializeInitialColumn(unsigned column){
        if(!pixels_||!initial_||column>=3)return false;
        UWORD scratch[AmigaInitialTerrain::words];
        for(unsigned y=0;y<initial_->rows();++y){
            if(!(dirty_[column][y]&2))continue;
            const auto* source=initial_->row(column,y,scratch);
            if(!source)return false;
            std::memcpy(&pixels_[column][y][0][0],source,AmigaInitialTerrain::rowBytes);
            dirty_[column][y]&=static_cast<unsigned char>(~2u);
        }
        return true;
    }
    bool dirty(unsigned column, unsigned y)const { return (dirty_[column][y]&1)!=0; }
    bool needsPatternMaterialization(unsigned y)const{
        return (dirty_[1][y]&33)==32&&bands_[(y+8)/16].tiles[0];
    }
    // Prepare already-admitted immutable pixels in ordinary memory. The
    // upload path still composes untouched loans if preparation runs out of
    // time. A dirty (mutated) row must instead be admitted again by the painter.
    unsigned materializePatternRows(unsigned first,unsigned budget);
    unsigned revision()const{return revision_;}
    // A generation/mutation band owns speculative work. Do not scan the
    // camera's entire look-ahead window again when nothing invalidated it.
    unsigned nextPreparation(unsigned first,unsigned limit){
        if(prepareFirst_<first)prepareFirst_=first;
        const unsigned end=prepareEnd_<limit?prepareEnd_:limit;
        while(prepareFirst_<end&&!dirty(1,prepareFirst_)&&
              !needsPatternMaterialization(prepareFirst_))++prepareFirst_;
        return prepareFirst_<end?prepareFirst_:rows;
    }
    void skipPreparation(unsigned end){if(prepareFirst_<end)prepareFirst_=end;}
    // A dictionary miss is still paintable by the portable compositor. Keep
    // its admission separate from painting, just like a prepared-pattern row.
    // This is one presentation task, not another terrain/resource owner.
    void deferPreparationFallback(unsigned y){
        if(y<8||y>=rows-8){fallbackFirst_=rows;fallbackEnd_=0;return;}
        fallbackFirst_=((y+8)&~15u)-8;
        fallbackEnd_=fallbackFirst_+16;
    }
    bool preparationFallback(unsigned y)const{
        // No geometry or art is cached in this task: Paint reads current live
        // producers. Invalidation keeps the task and dirty pixels, rather than
        // cancelling it at every unrelated generation-row publication.
        return y>=fallbackFirst_&&y<fallbackEnd_;
    }
    template<class Paint> int prepareFallback(unsigned first,unsigned limit,Paint paint){
        if(!ready||!pixels_||!preparationFallback(first)||first>=limit||limit>rows)return 0;
        unsigned end=fallbackEnd_<limit?fallbackEnd_:limit;
        if(end>first+8)end=first+8;
        end=runEnd(1,first,end);
        if(!dirty(1,first)||end==first)return 0;
        if(!paint(writeRows(1,first,end-first),first,end-first))return -1;
        for(unsigned y=first;y<end;++y)setDirty(1,y,false);
        return int(end-first);
    }
    void setDirty(unsigned column,unsigned y,bool value){
#ifdef DOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
        dirty_[column][y]=value;
#else
        dirty_[column][y]=(dirty_[column][y]&62)|(value?1:0);
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
