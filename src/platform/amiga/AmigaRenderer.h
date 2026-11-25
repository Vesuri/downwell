#ifndef DOWNWELL_PLATFORM_AMIGA_RENDERER_H
#define DOWNWELL_PLATFORM_AMIGA_RENDERER_H

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    downwellVisibleWidth = 380,
    downwellDmaWidth = 384,
    downwellDisplayHeight = 284,
    downwellGameRingHeight = 320,
    downwellBytesPerPlaneLine = downwellDmaWidth / 8,
    downwellPlaneBytes = downwellBytesPerPlaneLine * downwellDisplayHeight,
    downwellLayerBytes = downwellPlaneBytes * 2,
    downwellGameRingPlaneBytes =
        downwellBytesPerPlaneLine * downwellGameRingHeight,
    downwellGameRingBytes = downwellGameRingPlaneBytes * 2,
    /* Beam-space origin for logical x=0 of the gameplay window. Its first
       fetched word (logical x=16) appears at $81, the earliest start that
       keeps all eight sprite DMA channels (DDFSTRT $38). */
    downwellDisplayHStart = 0x71,
    /* Menu scenes keep their own fetch; their sprites use this origin. */
    downwellMenuSpriteHStart = 0x61,
    /* HRM table 3-13: PAL lines $1D-$137 are displayable. */
    downwellDisplayVStart = 0x1d,
    downwellPlayfieldX = 110,
    downwellPlayfieldWidth = 160
};

typedef struct AmigaRenderer {
    UBYTE* game;
    UBYTE* hud;
    UBYTE* target;
    unsigned gameOrigin;
    int clipX;
    int clipY;
    int clipWidth;
    int clipHeight;
    int blitterEnabled;
} AmigaRenderer;

void amigaRendererInit(AmigaRenderer* renderer, UBYTE* game, UBYTE* hud);
void amigaRendererSetGameOrigin(AmigaRenderer* renderer, unsigned origin);
void amigaRendererEnableBlitter(AmigaRenderer* renderer);
void amigaRendererClear(AmigaRenderer* renderer, unsigned pen);
void amigaRendererClearGameRows(AmigaRenderer* renderer,
                                int firstY, int height, unsigned pen);
void amigaRendererSetClipRect(AmigaRenderer* renderer,
                              int x, int y, int width, int height);
void amigaRendererClearClipRect(AmigaRenderer* renderer);
void amigaRendererFillRect(AmigaRenderer* renderer,
                           int x, int y, int width, int height,
                           unsigned pen);
/* CPU-only binary coverage, MSB first. Preserves unmasked pixels and padding. */
void amigaRendererDrawCoverageWord(AmigaRenderer*,int x,int y,UWORD bits,unsigned pen);
/* Final planar colours under alpha, MSB first; one placement for both planes. */
void amigaRendererDrawPlanarWord(AmigaRenderer*,int x,int y,UWORD alpha,UWORD low,UWORD high);
/* CPU-only prepared black/white rectangle. Native-endian source words;
   clip and circular destination placement are resolved once per rectangle. */
void amigaRendererDrawTextWords(AmigaRenderer*,int x,int y,
    const UWORD* mask,const UWORD* white,unsigned words,unsigned height,unsigned stride);
/* As above with the ink in pen (0..3) instead of pen 2. */
void amigaRendererDrawTextWordsPen(AmigaRenderer*,int x,int y,
    const UWORD* mask,const UWORD* ink,unsigned words,unsigned height,unsigned stride,unsigned pen);
/* Coverage copied from the existing offline numeric glyph planes at startup. */
typedef struct AmigaDamageGlyphs { UBYTE rows[11][10]; } AmigaDamageGlyphs;
int amigaRendererPrepareDamageGlyphs(AmigaDamageGlyphs*, const UBYTE*, const UBYTE*);
/* One 8x10, origin (4,0) offline glyph's coverage rows; 0 if the record differs. */
int amigaRendererPrepareGlyphRows(UBYTE rows[10], const UBYTE*, const UBYTE*, UWORD sprite);
/* The glyph's four pen-0 neighbours, then the glyph in pen, as one CPU write. */
void amigaRendererDrawBorderGlyph(AmigaRenderer*, const UBYTE rows[10], int x, int y, unsigned pen);
void amigaRendererDrawAsset(AmigaRenderer* renderer,
                            const UBYTE* pack, const UBYTE* packEnd,
                            UWORD sprite, UWORD frame,
                            int anchorX, int anchorY, int flipX);
/* Gameplay-only scratch in the retired scene bank; original planar tile art. */
enum { downwellTerrainBatchStride = 52,
       downwellTerrainBatchPlane = downwellTerrainBatchStride * downwellDisplayHeight,
       downwellTerrainBatchBytes = downwellTerrainBatchPlane * 3 };
typedef struct AmigaTerrainBatch {
    const UBYTE* tiles[26];
    UBYTE* scratch;
    AmigaRenderer* renderer;
    ULONG occupied[20];
    UBYTE lastFrame[20][26];
    struct { UBYTE* destination; unsigned sourceOffset, count; } rows[20];
    int firstColumn,lastColumn;
    int phaseX, phaseY, left, right, top, bottom, active;
} AmigaTerrainBatch;
int amigaRendererBindTerrainBatch(AmigaTerrainBatch*, const UBYTE*, const UBYTE*, UBYTE*, unsigned);
void amigaRendererBeginTerrainBatch(AmigaTerrainBatch*, AmigaRenderer*, int viewX, int viewY);
int amigaRendererBatchTerrainTile(AmigaTerrainBatch*, unsigned frame, int x, int y);
void amigaRendererBatchTerrainWallRow(AmigaTerrainBatch*,int x,int y,
    ULONG above,ULONG middle,ULONG below,unsigned columns);
void amigaRendererFlushTerrainBatch(AmigaTerrainBatch*);
/* Native words copied from an offline planar asset, not pixel conversion. */
typedef struct AmigaWordStamp {
    UWORD low[8], high[8], mask[8];
    UWORD runLow[64], runHigh[64], runMask[64];
    unsigned first, last;
} AmigaWordStamp;
int amigaRendererPrepareWordStamp(AmigaWordStamp*, const UBYTE*, const UBYTE*, UWORD, UWORD);
/* Small offline-composed glyph; copy planar words, never rasterize pixels. */
typedef struct AmigaHudGlyph {
    UWORD low[16], high[16];
    int left, top;
    unsigned width, height;
} AmigaHudGlyph;
int amigaRendererPrepareHudGlyph(AmigaHudGlyph*, const UBYTE*, const UBYTE*, UWORD, UWORD);
/* HUD text staging: compose a fixed HUD region's layers in source order in
   ordinary memory, then publish the result to the HUD layer in one pass.
   Art references point into the resident pack; nothing is converted. */
enum { amigaHudStageWords = 14, amigaHudStageRows = 30 }; /* W1.7 gem bar at timer 480: 14 words */
typedef struct AmigaHudArt {
    const UBYTE* data;
    int left, top;
    unsigned width, height, planeBytes;
} AmigaHudArt;
typedef struct AmigaHudStage {
    int x, y;
    unsigned words, rows;
    UWORD low[amigaHudStageRows][amigaHudStageWords];
    UWORD high[amigaHudStageRows][amigaHudStageWords];
    UWORD mask[amigaHudStageRows][amigaHudStageWords];
} AmigaHudStage;
int amigaRendererPrepareHudArt(AmigaHudArt*, const UBYTE*, const UBYTE*, UWORD, UWORD);
/* x must be word aligned. Placements outside the stage return 0. */
int amigaHudStageBegin(AmigaHudStage*, int x, int y, unsigned words, unsigned rows);
int amigaHudStageFill(AmigaHudStage*, int x, int y, int width, int height, unsigned pen);
/* pen < 0 keeps source colours, as amigaRendererDrawAsset; otherwise as DrawAssetPen. */
int amigaHudStagePlace(AmigaHudStage*, const AmigaHudArt*, int anchorX, int anchorY, int pen);
/* scrGemmyText's five layers of a string, each glyph i at anchorX + advance*i:
   pen 2 at (-1,0) and (0,-1), pen 3 at (1,0) and (0,1), then pen 1 at (0,0).
   The same result as those 5*count amigaHudStagePlace calls (any layer
   outside the stage returns 0 and writes nothing), with each glyph read once. */
int amigaHudStageGemmyText(AmigaHudStage*, const AmigaHudArt* const* glyphs, unsigned count,
                           int anchorX, int anchorY, int advance);
/* Publish stage, with overlay (same x, its own rows; may be null) on top. */
void amigaRendererPublishHudStage(AmigaRenderer*, const AmigaHudStage*, const AmigaHudStage*);
int amigaRendererDrawWordStamp(AmigaRenderer*, const AmigaWordStamp*, int, int);
int amigaRendererDrawWordStampRun(AmigaRenderer*, const AmigaWordStamp*, int, int, unsigned);
unsigned amigaRendererCopyHudWords(UBYTE*, const UBYTE*, int, int, int, int, int);
// Compose one retained masked column and publish only changed plane words.
unsigned amigaRendererPublishHudColumn(UBYTE*, UBYTE*, const UWORD*, unsigned,
                                      int, int, int, UWORD);
/* Host-composed column: byte dictionary index per eight interleaved planar rows.
   Dictionary block zero must be entirely transparent (all words zero).
   Readers consume the blocks directly; no runtime expansion or composition. */
typedef struct AmigaHudBlockedColumn {
    const UBYTE* indices;
    const UWORD* blocks;
} AmigaHudBlockedColumn;
unsigned amigaRendererPublishHudBlockedColumn(UBYTE*, UBYTE*, const AmigaHudBlockedColumn*,
                                             unsigned, int, int, int, UWORD);
void amigaRendererApplyHudBlockedColumn(AmigaRenderer*, const AmigaHudBlockedColumn*,
                                       unsigned, int, int, int);
/* One aligned HUD column, already clipped by caller; masked planar words. */
void amigaRendererFillHudWordColumn(UBYTE*, unsigned, unsigned, unsigned, UWORD, UWORD, UWORD);
void amigaRendererCacheHudColumns(UWORD*, const UBYTE*, unsigned, unsigned);
void amigaRendererApplyHudColumns(AmigaRenderer*, const UWORD*, unsigned, unsigned, int, int, int, int);
int amigaRendererAssetBounds(const UBYTE* pack, const UBYTE* packEnd,
                             UWORD sprite, UWORD frame, int anchorX,
                             int anchorY, int flipX, int* x, int* y,
                             int* width, int* height);
void amigaRendererDrawAssetPen(AmigaRenderer* renderer,
                               const UBYTE* pack, const UBYTE* packEnd,
                               UWORD sprite, UWORD frame,
                               int anchorX, int anchorY, int flipX,
                               unsigned pen);
/*
 * Returns zero on an invalid asset, one when at least one sprite pixel can
 * appear inside the game playfield, and two when the valid sprite is wholly
 * clipped.  A clipped result still writes a hidden hardware-sprite record.
 */
int amigaRendererBuildHardwareSprite(const UBYTE* pack, const UBYTE* packEnd,
                                     UWORD sprite, UWORD frame,
                                     int anchorX, int anchorY, int flipX,
                                     UWORD* destination,
                                     unsigned destinationHeight);
unsigned amigaRendererHardwareSpritePartCount(
    const UBYTE* pack, const UBYTE* packEnd, UWORD sprite, UWORD frame);
/* Short-lived descriptor: its bank must remain unchanged through drawing. */
typedef struct AmigaResolvedSpritePart {
    const UBYTE* pack;
    const UBYTE* end;
    const UBYTE* record;
    unsigned partCount;
} AmigaResolvedSpritePart;
int amigaRendererResolveSpritePart(AmigaResolvedSpritePart*, const UBYTE*, const UBYTE*, UWORD, UWORD, unsigned, int);
int amigaRendererDrawResolvedSpritePart(const AmigaResolvedSpritePart*, int, int, UWORD*, unsigned);
/* Invalidate copied payloads before source-bank or destination storage reuse. */
void amigaRendererReleaseHardwareSpriteCopies(void);
ULONG amigaRendererChipPackBytes(const UBYTE* pack, const UBYTE* packEnd);
int amigaRendererCopyChipPack(const UBYTE* pack,const UBYTE* end,UBYTE* destination,ULONG bytes);
int amigaRendererBuildHardwareSpritePart(
    const UBYTE* pack, const UBYTE* packEnd,
    UWORD sprite, UWORD frame, unsigned part,
    int anchorX, int anchorY, int flipX,
    UWORD* destination, unsigned destinationHeight);

#ifdef __cplusplus
}
#endif

#endif
