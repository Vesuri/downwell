#include "AmigaTerrainPainter.h"
#include "AmigaWallBackground.h"
#include "AmigaTerrainRunSurface.h"
#include "core/GameplayWorld.h"
#include "core/Profile.h"
#include "core/ShopFixtures.h"
#include "core/WallPresentation.h"
#include "core/WallFootprint.h"
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
#include "framework/AmigaHardware.h"
#include "AmigaBufferCompare.h"
#include <cstring>
extern "C" {
volatile ULONG gDownwellTerrainBatchChecks=0,gDownwellTerrainBatchFailures=0;
volatile ULONG gDownwellTerrainDirectChecks=0;
__attribute__((noinline)) void downwellTerrainBatchMismatch(){
    ++gDownwellTerrainBatchFailures;
    __asm__ volatile("" ::: "memory");
}
}
#endif

namespace {
AmigaTerrainBatch terrainBatch{};
const UBYTE* preparedTerrain=nullptr;
AmigaTerrainRunSurface* directSurface=nullptr;
bool directFailed=false;
#ifdef DOWNWELL_TERRAIN_BATCH
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
bool useTerrainBatch=true;
#else
constexpr bool useTerrainBatch=true;
#endif
static ULONG wallRowBits(const AmigaTerrainIndex& index,int row,int column,unsigned count){
    if(row<0 || row>=AmigaTerrainIndex::rows)return 0;
    ULONG bits=0,bit=1;
    for(unsigned i=0;i<count;++i,++column,bit<<=1)
        if(column>=0 && column<AmigaTerrainIndex::columns &&
            (index.cells[row*AmigaTerrainIndex::columns+column]&AmigaTerrainIndex::wallAppearanceFlags))bits|=bit;
    return bits;
}
static void paintIndexedBatch(const AmigaTerrainIndex& index,int viewX,int viewY){
    if(terrainBatch.left>=terrainBatch.right || terrainBatch.top>=terrainBatch.bottom)return;
    const int firstX=(viewX+terrainBatch.left-downwellPlayfieldX+8)&~15;
    const int lastX=(viewX+terrainBatch.right-downwellPlayfieldX+7)&~15;
    const int firstY=(viewY+terrainBatch.top+8)&~15;
    const int lastY=(viewY+terrainBatch.bottom+7)&~15;
    const unsigned columns=unsigned(lastX-firstX)/16+1;
    const int column=(firstX-AmigaTerrainIndex::minX)/16-1;
    int row=firstY/16;
    ULONG above=wallRowBits(index,row-1,column,columns+2);
    ULONG middle=wallRowBits(index,row,column,columns+2);
    for(int y=firstY;y<=lastY;y+=16,++row){
        const ULONG below=wallRowBits(index,row+1,column,columns+2);
        if(directSurface){
            directSurface->wallRow(downwellPlayfieldX+firstX-viewX,y-viewY,
                above,middle,below,columns);
        }else amigaRendererBatchTerrainWallRow(&terrainBatch,
            downwellPlayfieldX+firstX-viewX,y-viewY,above,middle,below,columns);
        above=middle;middle=below;
    }
}
#endif
static bool roomHasMainWall(const downwell::RoomPack& rooms,
                            const downwell::PackedRoom& room,
                            int x, int y) {
    unsigned index;
    for (index = 0; index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        if (rooms.roomInstance(room, index, instance) &&
            instance.objectId == 60 && instance.x == x && instance.y == y)
            return true;
    }
    return false;
}

static UWORD mainWallFrame(const AmigaTerrainIndex* terrainIndex,
                           const downwell::RoomPack& rooms,
                           const downwell::PackedRoom& room,
                           int x, int y) {
    DW_PROFILE_SCOPE(profileWallFrame);
    if (terrainIndex) {
        const auto& index = *terrainIndex;
        return (amigaIndexedWall(index, x, y - 16) ? 1 : 0) |
               (amigaIndexedWall(index, x + 16, y) ? 2 : 0) |
               (amigaIndexedWall(index, x, y + 16) ? 4 : 0) |
               (amigaIndexedWall(index, x - 16, y) ? 8 : 0);
    }
    UWORD frame = 0;
    if (roomHasMainWall(rooms, room, x, y - 16)) frame |= 1;
    if (roomHasMainWall(rooms, room, x + 16, y)) frame |= 2;
    if (roomHasMainWall(rooms, room, x, y + 16)) frame |= 4;
    if (roomHasMainWall(rooms, room, x - 16, y)) frame |= 8;
    return frame;
}
}

bool AmigaTerrainPainter::bindBatch(const UBYTE* pack,const UBYTE* end,UBYTE* scratch,unsigned bytes) {
    return amigaRendererBindTerrainBatch(&terrainBatch,pack,end,scratch,bytes)!=0;
}
bool AmigaTerrainPainter::bindPrepared(const UBYTE* data,unsigned bytes){
    preparedTerrain=bytes==4992?data:nullptr;
    return preparedTerrain!=nullptr;
}

bool AmigaTerrainPainter::corridorRows(const AmigaTerrainIndex& index,
    const AmigaTerrainTiles& tiles,int first,int end){
    if(first<24||first>=end||end>AmigaTerrainIndex::rows*16-24)return false;
    const int firstRow=(first+8)>>4,lastRow=(end+7)>>4;
    // Only the vertical neighbours of the two visible wall columns select
    // their frames. Offscreen diagonals at the authored entrance do not.
    const int guards[]={firstRow-1,lastRow+1};
    for(int row:guards)for(unsigned column=42;column<=52;column+=10)
        if(!(index.cells[row*AmigaTerrainIndex::columns+column]&AmigaTerrainIndex::wallAppearanceFlags))return false;
    // Every rendered row must still contain exactly the two walls, with
    // empty horizontal neighbours. Contact multiplicity is not artwork.
    for(int row=firstRow;row<=lastRow;++row){
        if(tiles.first(row*16))return false;
        for(unsigned column=41;column<=53;++column){
            const bool wall=(index.cells[row*AmigaTerrainIndex::columns+column]&
                             AmigaTerrainIndex::wallAppearanceFlags)!=0;
            if(wall!=(column==42||column==52))return false;
        }
    }
    return true;
}

bool AmigaTerrainPainter::paintCached(UWORD* rows,AmigaRenderer* renderer,
    const downwell::RoomPack& rooms,const downwell::PackedRoom& room,
    const AmigaTerrainIndex* index,const AmigaTerrainTiles& tiles,
    const AmigaTerrainDecoration* decorations,unsigned decorationCount,
    const GameplayWorld* runtime,const UBYTE* pack,const UBYTE* end,
    int viewX,int viewY,int clipY,int clipHeight){
#ifndef DOWNWELL_TERRAIN_BATCH
    return false;
#else
    if(!rows||!terrainBatch.scratch||clipY<0||clipHeight<=0||
        clipY+clipHeight>downwellDisplayHeight||directSurface)return false;
    AmigaTerrainRunSurface surface(rows,clipY,clipHeight,downwellPlayfieldX,
        downwellPlayfieldWidth,terrainBatch.tiles,preparedTerrain);
    AmigaRenderer local=*renderer;
    directSurface=&surface;directFailed=false;
    surface.clear();
    const bool ok=paint(&local,rooms,room,index,tiles,decorations,decorationCount,
        runtime,pack,end,viewX,viewY,downwellPlayfieldX,clipY,downwellPlayfieldWidth,clipHeight);
    directSurface=nullptr;
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
    if(ok&&!directFailed){
        alignas(UWORD) static UBYTE expected[downwellGameRingBytes];
        std::memset(expected,0,sizeof expected);
        AmigaRenderer oracle=*renderer;
        oracle.game=oracle.target=expected;oracle.blitterEnabled=0;
        useTerrainBatch=false;
        const bool referenceOK=paint(&oracle,rooms,room,index,tiles,decorations,decorationCount,
            runtime,pack,end,viewX,viewY,downwellPlayfieldX,clipY,downwellPlayfieldWidth,clipHeight);
        useTerrainBatch=true;
        bool equal=referenceOK;
        for(int y=0;y<clipHeight;++y)for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w){
            const unsigned physical=(renderer->gameOrigin+clipY+y)%downwellGameRingHeight;
            const UWORD* source=reinterpret_cast<const UWORD*>(expected+p*downwellGameRingPlaneBytes+
                physical*downwellBytesPerPlaneLine+12);
            if(rows[y*22+p*11+w]!=source[w])equal=false;
        }
        ++gDownwellTerrainDirectChecks;
        if(!equal){downwellTerrainBatchMismatch();return false;}
    }
#endif
    return ok&&!directFailed;
#endif
}

bool AmigaTerrainPainter::paint(AmigaRenderer* renderer,
    const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
    const AmigaTerrainIndex* terrainIndex, const AmigaTerrainTiles& terrainTiles,
    const AmigaTerrainDecoration* decorations, unsigned decorationCount,
    const GameplayWorld* runtime, const UBYTE* pack, const UBYTE* end,
    int viewX, int viewY, int clipX, int clipY, int clipWidth, int clipHeight) {
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
    alignas(UWORD) static UBYTE expected[downwellGameRingBytes];
    AmigaRenderer oracle=*renderer;
    if(useTerrainBatch&&!directSurface){
        AmigaHardware::blitterDrain();
        std::memcpy(expected,renderer->game,sizeof expected);
        oracle.game=oracle.target=expected;
        oracle.blitterEnabled=0;
    }
#endif
    unsigned index;
    const int firstWorldY = viewY + clipY;
    const int finalWorldY = firstWorldY + clipHeight;
    const auto drawGameAsset = [pack, end](AmigaRenderer* target, UWORD sprite,
        UWORD frame, int x, int y, int flip) {
        DW_PROFILE_SCOPE(profileTerrainAsset);
        if(directSurface){
            if(sprite==51&&!flip&&directSurface->tile(frame,x,y))return;
            // The normal renderer does nothing for missing or wholly clipped
            // art. Such a command must not abort a completed cache span.
            int left,top,width,height;
            if(amigaRendererAssetBounds(pack,end,sprite,frame,x,y,flip,&left,&top,&width,&height)&&
                left<target->clipX+target->clipWidth&&left+width>target->clipX&&
                top<target->clipY+target->clipHeight&&top+height>target->clipY)
                directFailed=true;
            return;
        }
#ifdef DOWNWELL_TERRAIN_BATCH
        if(useTerrainBatch){
            if(sprite==51 && !flip && amigaRendererBatchTerrainTile(&terrainBatch,frame,x,y))return;
            amigaRendererFlushTerrainBatch(&terrainBatch);
        }
#endif
        amigaRendererDrawAsset(target, pack, end, sprite, frame, x, y, flip);
    };
    auto drawIndexedWalls = [&](AmigaRenderer* renderer,
        const downwell::RoomPack& rooms, const downwell::PackedRoom& room,
        int viewX, int viewY, int clipX, int clipY, int width, int height) {
#ifdef DOWNWELL_TERRAIN_BATCH
        if(useTerrainBatch&&terrainBatch.scratch){paintIndexedBatch(*terrainIndex,viewX,viewY);return;}
#endif
        const int firstX = viewX + clipX - downwellPlayfieldX;
        const int firstY = viewY + clipY;
        for (int y = (firstY - 8) & ~15; y < firstY + height + 8; y += 16)
            for (int x = (firstX - 8) & ~15; x < firstX + width + 8; x += 16)
                if (amigaIndexedWall(*terrainIndex, x, y))
                    drawGameAsset(renderer, 51, mainWallFrame(terrainIndex, rooms, room, x, y),
                                  downwellPlayfieldX + x - viewX, y - viewY, 0);
    };
    amigaRendererSetClipRect(renderer, clipX, clipY, clipWidth, clipHeight);
#ifdef DOWNWELL_TERRAIN_BATCH
    if(useTerrainBatch){
        if(directSurface){
            terrainBatch.left=clipX;terrainBatch.right=clipX+clipWidth;
            terrainBatch.top=clipY;terrainBatch.bottom=clipY+clipHeight;
        }else amigaRendererBeginTerrainBatch(&terrainBatch,renderer,viewX,viewY);
    }
#endif
    // A fresh direct-cache span is already black. All wall-background
    // rectangles are black and precede every wall/tile/decoration.
    if(!directSurface&&runtime&&!amigaPaintWallBackground(renderer,runtime->wallBackgroundTiles(),
        viewX,viewY,clipX,clipY,clipWidth,clipHeight)){
        amigaRendererClearClipRect(renderer);return false;
    }
    {
    DW_PROFILE_SCOPE(profileTerrainWalls);
    const auto* records=runtime?runtime->terrainInstances():nullptr;
    const auto* native=runtime?runtime->nativeTerrainInstances():nullptr;
    const bool ownedNative=native&&native->hasWallPresentationState();
    if(records){
        DW_PROFILE_COUNT(counterTerrainWallPasses);
        unsigned examined=0;
#ifdef DOWNWELL_SCENE_VERIFY
        // Independent full-order selection oracle; never benchmark this scan.
        unsigned referenceIndex=0;
        auto nextReference=[&]()->decltype(records->find(downwell::InstanceHandle{})){
            while(referenceIndex<records->size()){
                const auto r=records->find(records->handle(referenceIndex++));
                const int sprite=r->wall.checked?r->spriteId:51;
                if(downwell::wallIntersectsRows(sprite,r->y,firstWorldY,finalWorldY))return r;
            }
            return nullptr;
        };
#endif
        const bool painted=records->visitWallRows(firstWorldY,finalWorldY,[&](const auto& r){
            ++examined;
            const int sprite=r.wall.checked?r.spriteId:51;
            if(!downwell::wallIntersectsRows(sprite,r.y,firstWorldY,finalWorldY))return true;
#ifdef DOWNWELL_SCENE_VERIFY
            const auto reference=nextReference();
            if(!reference||reference->lifetimeId!=r.lifetimeId||reference->objectId!=r.objectId||
                reference->x!=r.x||reference->y!=r.y||reference->active!=r.active||
                reference->destroyRequested!=r.destroyRequested||reference->spriteId!=r.spriteId||
                reference->imageIndex!=r.imageIndex||reference->wall.checked!=r.wall.checked)return false;
#endif
            DW_PROFILE_COUNT(counterTerrainWallRowCandidates);
            if(!downwell::drawWallPresentation(r,[&]{return mainWallFrame(terrainIndex,rooms,room,r.x,r.y);},
                [&](int sprite,unsigned frame){drawGameAsset(renderer,UWORD(sprite),UWORD(frame),
                    downwellPlayfieldX+r.x-viewX,r.y-viewY,0);}))return false;
            return true;
        });
        DW_PROFILE_ADD(counterTerrainWallRecords,examined);
        if(!painted){amigaRendererClearClipRect(renderer);return false;}
#ifdef DOWNWELL_SCENE_VERIFY
        if(nextReference()){amigaRendererClearClipRect(renderer);return false;}
#endif
    }
    if(!records&&ownedNative){
        // Keep run birth order, including coincident walls. Event-selected
        // frames must not be recomputed from the occupancy grid after a
        // neighbor dies. Untouched scenes retain the indexed batch path.
        const bool painted=native->visitWallRows(firstWorldY,finalWorldY,[&](const auto& r){
            return downwell::drawWallPresentation(r,[&]{return mainWallFrame(terrainIndex,rooms,room,r.x,r.y);},
                [&](int sprite,unsigned frame){drawGameAsset(renderer,UWORD(sprite),UWORD(frame),
                    downwellPlayfieldX+r.x-viewX,r.y-viewY,0);});
        });
        if(!painted){amigaRendererClearClipRect(renderer);return false;}
    }
    if (!records&&!ownedNative&&terrainIndex)
        drawIndexedWalls(renderer, rooms, room, viewX, viewY,
                             clipX, clipY, clipWidth, clipHeight);
    for (index = 0; !records && !ownedNative && !terrainIndex && index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        if (!rooms.roomInstance(room, index, instance) ||
            instance.objectId != 60 || instance.y + 8 < firstWorldY ||
            instance.y - 8 >= finalWorldY)
            continue;
        drawGameAsset(renderer, 51,
                      mainWallFrame(terrainIndex, rooms, room, instance.x, instance.y),
                      downwellPlayfieldX + instance.x - viewX,
                      instance.y - viewY, 0);
    }
    }
    {
    DW_PROFILE_SCOPE(profileTerrainTiles);
    for (int rowY = (firstWorldY - 8) & ~15; rowY < finalWorldY + 8; rowY += 16) {
        for (unsigned id = terrainTiles.first(rowY); id; ) {
            const auto& tile = terrainTiles.at(id);
            id = tile.next;
            if (tile.objectId == 92 && runtime &&
                !runtime->breakableActiveAt(tile.x - 240, rowY - 176)) continue;
            drawGameAsset(renderer, 51, tile.frame,
                          downwellPlayfieldX + tile.x - viewX, rowY - viewY, 0);
        }
    }
    }
    {
    DW_PROFILE_SCOPE(profileTerrainDecorations);
    for (index = 0; index < decorationCount; ++index) {
        const AmigaTerrainDecoration& decoration = decorations[index];
        if (decoration.y + 24 < firstWorldY ||
            decoration.y - 24 >= finalWorldY)
            continue;
        drawGameAsset(renderer, (UWORD)decoration.spriteId,
                      (UWORD)decoration.frame,
                      downwellPlayfieldX + decoration.x - viewX,
                      decoration.y - viewY,
                      decoration.flipX ? 1 : 0);
    }
    }
    if (runtime) downwell::drawShopFixtures(runtime->shops(), [&](int sprite, int frame, int x, int y) {
        if (y + 24 >= firstWorldY && y - 24 < finalWorldY)
            drawGameAsset(renderer, sprite, frame, downwellPlayfieldX + x - viewX, y - viewY, 0);
    });
#ifdef DOWNWELL_TERRAIN_BATCH
    if(useTerrainBatch&&!directSurface)amigaRendererFlushTerrainBatch(&terrainBatch);
#endif
    amigaRendererClearClipRect(renderer);
#ifdef DOWNWELL_TERRAIN_BATCH_VERIFY
    if(useTerrainBatch&&!directSurface){
        AmigaHardware::blitterDrain();
        useTerrainBatch=false;
        const bool ok=paint(&oracle,rooms,room,terrainIndex,terrainTiles,decorations,
            decorationCount,runtime,pack,end,viewX,viewY,clipX,clipY,clipWidth,clipHeight);
        useTerrainBatch=true;
        ++gDownwellTerrainBatchChecks;
        if(!ok || amigaBufferDifferences(expected,renderer->game,sizeof expected)){
            downwellTerrainBatchMismatch();return false;
        }
    }
#endif
    return true;
}
