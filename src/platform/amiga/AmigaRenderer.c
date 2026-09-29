#include "AmigaRenderer.h"
#include "AmigaProfileCounters.h"

extern int amigaBlitterDrawPlanarMasked(
    const UBYTE* source, const UBYTE* mask,
    UWORD sourceWidth, UWORD sourceHeight, UWORD sourceDataWidth,
    UBYTE* destination, UWORD destinationHeight,
    UWORD destinationX, UWORD destinationY,
    UWORD sourceX, UWORD sourceY, UWORD width, UWORD height, int outputPen);
extern int amigaBlitterClearPlanar(UBYTE* destination,
                                   UWORD height, UWORD planes);
extern int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD);

static UWORD readWord(const UBYTE* data) {
#ifdef DOWNWELL_PLATFORM_AMIGA
    return *(const UWORD*)data; // DWAP fields/rows are word-aligned, native big endian.
#else
    return (UWORD)(((UWORD)data[0] << 8) | data[1]);
#endif
}

static ULONG readLong(const UBYTE* data) {
#ifdef DOWNWELL_PLATFORM_AMIGA
    return *(const ULONG*)data;
#else
    return ((ULONG)readWord(data) << 16) | readWord(data + 2);
#endif
}

static void writeWord(UBYTE* data, UWORD value) {
#ifdef DOWNWELL_PLATFORM_AMIGA
    *(UWORD*)data = value;
#else
    data[0] = (UBYTE)(value >> 8);
    data[1] = (UBYTE)value;
#endif
}

void amigaRendererDrawPlanarWord(AmigaRenderer* renderer,int x,int y,UWORD bits,UWORD low,UWORD high) {
    int left,right,first,last,column,shift,part;
    unsigned row;
    ULONG planeBytes;
    if(!bits || x<=-16 || x>=downwellVisibleWidth || y<0 || y>=downwellDisplayHeight ||
       y<renderer->clipY || y>=renderer->clipY+renderer->clipHeight)return;
    left=renderer->clipX>0?renderer->clipX:0;
    right=renderer->clipX+renderer->clipWidth;
    if(right>downwellVisibleWidth)right=downwellVisibleWidth;
    first=x>left?x:left;last=x+16<right?x+16:right;
    if(first>=last)return;
    bits&=(UWORD)(0xffffu>>(first-x));
    bits&=(UWORD)(0xffffu<<(16-(last-x)));
    column=x>=0?x/16:-1;shift=x-column*16;
    row=(unsigned)y;planeBytes=downwellPlaneBytes;
    if(renderer->target==renderer->game){
        row=(renderer->gameOrigin+row)%downwellGameRingHeight;
        planeBytes=downwellGameRingPlaneBytes;
    }
    for(part=0;part<(shift?2:1);++part){
        unsigned plane;
        const UWORD colors[2]={low,high};
        const int destination=column+part;
        const UWORD mask=part?(UWORD)((ULONG)bits<<(16-shift)):(UWORD)(bits>>shift);
        UBYTE* word;
        if(!mask || destination<0 || destination>=downwellDmaWidth/16)continue;
        word=renderer->target+row*downwellBytesPerPlaneLine+destination*2;
        for(plane=0;plane<2;++plane){
            const UWORD old=readWord(word);
            const UWORD paint=part?(UWORD)((ULONG)colors[plane]<<(16-shift)):(UWORD)(colors[plane]>>shift);
            const UWORD value=(old&(UWORD)~mask)|(paint&mask);
            if(value!=old)writeWord(word,value);
            word+=planeBytes;
        }
    }
}

void amigaRendererDrawCoverageWord(AmigaRenderer* renderer,int x,int y,UWORD bits,unsigned pen) {
    amigaRendererDrawPlanarWord(renderer,x,y,bits,pen&1u?bits:0,pen&2u?bits:0);
}

static void setPixel(AmigaRenderer* renderer, int x, int y, unsigned pen) {
    UBYTE* row;
    UBYTE mask;
    ULONG planeBytes = downwellPlaneBytes;
    int targetY = y;
    if (x < renderer->clipX || y < renderer->clipY ||
        x >= renderer->clipX + renderer->clipWidth ||
        y >= renderer->clipY + renderer->clipHeight ||
        x < 0 || x >= downwellVisibleWidth ||
        y < 0 || y >= downwellDisplayHeight)
        return;
    if (renderer->target == renderer->game) {
        targetY = (int)((renderer->gameOrigin + (unsigned)y) %
                        downwellGameRingHeight);
        planeBytes = downwellGameRingPlaneBytes;
    }
    row = renderer->target + targetY * downwellBytesPerPlaneLine;
    mask = (UBYTE)(0x80u >> (x & 7));
    row[x >> 3] &= (UBYTE)~mask;
    row[planeBytes + (x >> 3)] &= (UBYTE)~mask;
    if (pen & 1u) row[x >> 3] |= mask;
    if (pen & 2u) row[planeBytes + (x >> 3)] |= mask;
}

static UBYTE sourceSpanByte(const UBYTE* row, int planeBytes, int sourceX) {
    int sourceByte;
    int shift;
    UWORD window;
    if (sourceX < 0)
        return sourceX > -8 && planeBytes > 0
                   ? (UBYTE)(row[0] >> -sourceX) : 0;
    sourceByte = sourceX >> 3;
    shift = sourceX & 7;
    if (sourceByte >= planeBytes)
        return 0;
    window = (UWORD)row[sourceByte] << 8;
    if (sourceByte + 1 < planeBytes)
        window |= row[sourceByte + 1];
    return (UBYTE)((window << shift) >> 8);
}

static UBYTE spanMask(int first, int last) {
    UBYTE mask = 0;
    int bit;
    for (bit = first; bit < last; ++bit)
        mask |= (UBYTE)(0x80u >> bit);
    return mask;
}

static UWORD sourceSpanWord(const UBYTE* row, int planeBytes, int x) {
    unsigned byte, shift;
    ULONG window;
    if (x < 0) return (UWORD)(readWord(row) >> -x);
    byte = (unsigned)(x >> 4) * 2;
    if (byte >= (unsigned)planeBytes) return 0;
    shift = x & 15;
    window = (ULONG)readWord(row + byte) << 16;
    if (byte + 2 < (unsigned)planeBytes) window |= readWord(row + byte + 2);
    return (UWORD)((window << shift) >> 16);
}

/*
 * The packed assets already contain native one-bit planes.  Compose one
 * destination byte at a time instead of sending every set source pixel
 * through setPixel().  Apart from the two clipped edge bytes this turns eight
 * source tests and sixteen Chip-RAM read/modify/writes into one planar span.
 */
static void drawAssetNormal(AmigaRenderer* renderer,
                            const UBYTE* data,
                            int originX, int originY,
                            int width, int height, int planeBytes,
                            int anchorX, int anchorY, int outputPen) {
    const int left = anchorX - originX;
    const int top = anchorY - originY;
    int clipLeft = renderer->clipX;
    int clipTop = renderer->clipY;
    int clipRight = renderer->clipX + renderer->clipWidth;
    int clipBottom = renderer->clipY + renderer->clipHeight;
    int firstY;
    int lastY;
    int y;
    ULONG destinationPlaneBytes;

    if (clipLeft < 0) clipLeft = 0;
    if (clipTop < 0) clipTop = 0;
    if (clipRight > downwellVisibleWidth) clipRight = downwellVisibleWidth;
    if (clipBottom > downwellDisplayHeight)
        clipBottom = downwellDisplayHeight;
    if (clipLeft < left) clipLeft = left;
    if (clipTop < top) clipTop = top;
    if (clipRight > left + width) clipRight = left + width;
    if (clipBottom > top + height) clipBottom = top + height;
    if (clipLeft >= clipRight || clipTop >= clipBottom)
        return;

    firstY = clipTop;
    lastY = clipBottom;
    destinationPlaneBytes = renderer->target == renderer->game
                                ? downwellGameRingPlaneBytes
                                : downwellPlaneBytes;
    /* The ammo animation emits 184 overlapping 12x8 assets. Each fits one
       aligned word; setting up/draining two masked blits per tiny asset costs
       much more than these native word operations. The existing blitter
       wrappers drain before returning, so CPU writes cannot race pending DMA.
       Keep the general clipped/ring paths below for all other geometry. */
    if (renderer->target == renderer->hud && planeBytes == 2 && width <= 16 &&
        height <= 8 && left >= 0 && !(left & 15)) {
        const UWORD clipMask = (UWORD)((0xffffu >> (clipLeft - left)) &
            (0xffffu << (16 - (clipRight - left))));
        const ULONG sourcePlaneBytes = 2u * (unsigned)height;
        const UBYTE* source = data + 2 * (firstY - top);
        UBYTE* target = renderer->target + firstY * downwellBytesPerPlaneLine + (left >> 3);
        for (y = firstY; y < lastY; ++y) {
            UWORD low = readWord(source);
            UWORD high = readWord(source + sourcePlaneBytes);
            const UWORD mask = (low | high) & clipMask;
            if (mask) {
                if (outputPen >= 0) {
                    low = outputPen & 1 ? 0xffff : 0;
                    high = outputPen & 2 ? 0xffff : 0;
                }
                writeWord(target, (readWord(target) & (UWORD)~mask) | (low & mask));
                writeWord(target + destinationPlaneBytes,
                    (readWord(target + destinationPlaneBytes) & (UWORD)~mask) | (high & mask));
            }
            source += 2;
            target += downwellBytesPerPlaneLine;
        }
        return;
    }
    /* Small tiles use the same clipped word compositor as HUD artwork.
       Resolve the circular row once, then advance/wrap pointers; no per-byte
       ring arithmetic or blitter setup is needed for a short exposed strip. */
    if (width <= 32 &&
        (!renderer->blitterEnabled || lastY-firstY <= 24) && !(planeBytes & 1)) {
        const int firstWord = clipLeft >> 4, lastWord = (clipRight-1) >> 4;
        const UWORD firstMask = (UWORD)(0xffffu >> (clipLeft & 15));
        const UWORD lastMask = (UWORD)(0xffffu << (15 - ((clipRight-1) & 15)));
        const ULONG sourcePlaneBytes = (ULONG)planeBytes * height;
        const unsigned targetHeight = renderer->target == renderer->game
            ? downwellGameRingHeight : downwellDisplayHeight;
        unsigned targetY = firstY;
        if (renderer->target == renderer->game)
            targetY = (renderer->gameOrigin + targetY) % targetHeight;
        UBYTE* targetRow = renderer->target + targetY * downwellBytesPerPlaneLine;
        const UBYTE* source = data + (firstY-top)*planeBytes;
        for (y = firstY; y < lastY; ++y) {
            int word;
            for (word = firstWord; word <= lastWord; ++word) {
                UWORD low = sourceSpanWord(source, planeBytes, word*16-left);
                UWORD high = sourceSpanWord(source+sourcePlaneBytes, planeBytes, word*16-left);
                UWORD mask = low | high;
                UBYTE* target = targetRow + word*2;
                if (word == firstWord) mask &= firstMask;
                if (word == lastWord) mask &= lastMask;
                if (!mask) continue;
                if (outputPen >= 0) { low = outputPen & 1 ? 0xffff : 0; high = outputPen & 2 ? 0xffff : 0; }
                writeWord(target, (readWord(target) & (UWORD)~mask) | (low & mask));
                writeWord(target+destinationPlaneBytes,
                    (readWord(target+destinationPlaneBytes) & (UWORD)~mask) | (high & mask));
            }
            source += planeBytes; targetRow += downwellBytesPerPlaneLine;
            if (++targetY == targetHeight) {
                targetY = 0;
                targetRow = renderer->target;
            }
        }
        return;
    }
    if (renderer->blitterEnabled) {
        const ULONG sourcePlaneBytes = (ULONG)planeBytes * (ULONG)height;
        int remaining = lastY - firstY;
        int sourceY = firstY - top;
        unsigned targetY = (unsigned)firstY;
        const unsigned targetHeight = renderer->target == renderer->game
                                          ? downwellGameRingHeight
                                          : downwellDisplayHeight;
        if (renderer->target == renderer->game)
            targetY = (renderer->gameOrigin + targetY) % targetHeight;
        while (remaining > 0) {
            int rows = remaining;
            if (rows > (int)(targetHeight - targetY))
                rows = (int)(targetHeight - targetY);
            if (!amigaBlitterDrawPlanarMasked(
                    data, data + sourcePlaneBytes * 2,
                    (UWORD)width, (UWORD)height, (UWORD)(planeBytes * 8),
                    renderer->target, (UWORD)targetHeight,
                    (UWORD)clipLeft, (UWORD)targetY,
                    (UWORD)(clipLeft - left), (UWORD)sourceY,
                    (UWORD)(clipRight - clipLeft), (UWORD)rows, outputPen))
                break;
            remaining -= rows;
            sourceY += rows;
            targetY = 0;
        }
        if (remaining == 0)
            return;
    }
    for (y = firstY; y < lastY; ++y) {
        const ULONG sourcePlaneBytes = (ULONG)planeBytes * (ULONG)height;
        const UBYTE* source0 = data + (y - top) * planeBytes;
        const UBYTE* source1 = source0 + sourcePlaneBytes;
        unsigned targetY = (unsigned)y;
        ULONG rowOffset;
        int destinationByte;
        int firstByte = clipLeft >> 3;
        int lastByte = (clipRight - 1) >> 3;
        if (renderer->target == renderer->game)
            targetY = (renderer->gameOrigin + targetY) %
                      downwellGameRingHeight;
        rowOffset = targetY * downwellBytesPerPlaneLine;
        for (destinationByte = firstByte;
             destinationByte <= lastByte; ++destinationByte) {
            const int destinationX = destinationByte << 3;
            int firstBit = clipLeft - destinationX;
            int lastBit = clipRight - destinationX;
            UBYTE clipMask;
            UBYTE plane0;
            UBYTE plane1;
            UBYTE opaque;
            UBYTE* target0;
            UBYTE* target1;
            if (firstBit < 0) firstBit = 0;
            if (lastBit > 8) lastBit = 8;
            clipMask = spanMask(firstBit, lastBit);
            plane0 = sourceSpanByte(source0, planeBytes,
                                    destinationX - left);
            plane1 = sourceSpanByte(source1, planeBytes,
                                    destinationX - left);
            opaque = (UBYTE)((plane0 | plane1) & clipMask);
            if (outputPen >= 0) {
                plane0 = outputPen & 1 ? 0xff : 0;
                plane1 = outputPen & 2 ? 0xff : 0;
            }
            if (!opaque)
                continue;
            target0 = renderer->target + rowOffset + destinationByte;
            target1 = target0 + destinationPlaneBytes;
            *target0 = (UBYTE)((*target0 & (UBYTE)~opaque) |
                               (plane0 & opaque));
            *target1 = (UBYTE)((*target1 & (UBYTE)~opaque) |
                               (plane1 & opaque));
        }
    }
}

static const UBYTE* findAsset(const UBYTE* pack, const UBYTE* packEnd,
                              UWORD sprite, UWORD frame) {
    const UBYTE* records;
    const ULONG wanted = ((ULONG)sprite << 16) | frame;
    ULONG count;
    ULONG first = 0;
    ULONG last;
    static struct { const UBYTE* pack; const UBYTE* end; ULONG key; const UBYTE* record; } cache[64];
    const unsigned slot = (sprite ^ (frame * 17u)) & 63u;
    if (packEnd - pack < 20 || readLong(pack) != 0x44574150UL ||
        readWord(pack + 4) != 2)
        return 0;
    count = readLong(pack + 8);
    records = pack + readWord(pack + 6);
    if (records < pack || records + count * 24 > packEnd)
        return 0;
    /* A selected artwork bank may be replaced in place, even at equal size.
       A matching address/key alone does not identify the new directory slot.
       Keep the cache enabled in host tests so this target path is exercised. */
    if (cache[slot].pack == pack && cache[slot].end == packEnd && cache[slot].key == wanted &&
        cache[slot].record >= records && cache[slot].record < records + count * 24 &&
        readLong(cache[slot].record) == wanted)
        return cache[slot].record;
    last = count;
    while (first < last) {
        const ULONG middle = first + (last - first) / 2;
        const UBYTE* record = records + middle * 24;
        const ULONG key = readLong(record);
        if (key < wanted)
            first = middle + 1;
        else if (key > wanted)
            last = middle;
        else {
            cache[slot].pack = pack;
            cache[slot].end = packEnd;
            cache[slot].key = wanted;
            cache[slot].record = record;
            return record;
        }
    }
    return 0;
}

void amigaRendererInit(AmigaRenderer* renderer, UBYTE* game, UBYTE* hud) {
    renderer->game = game;
    renderer->hud = hud;
    renderer->gameOrigin = 0;
    renderer->blitterEnabled = 0;
    amigaRendererClearClipRect(renderer);
}

void amigaRendererSetGameOrigin(AmigaRenderer* renderer, unsigned origin) {
    renderer->gameOrigin = origin % downwellGameRingHeight;
}

void amigaRendererEnableBlitter(AmigaRenderer* renderer) {
    renderer->blitterEnabled = 1;
}

void amigaRendererClear(AmigaRenderer* renderer, unsigned pen) {
    ULONG index;
    UBYTE* savedTarget = renderer->target;
    int savedX = renderer->clipX;
    int savedY = renderer->clipY;
    int savedWidth = renderer->clipWidth;
    int savedHeight = renderer->clipHeight;
    if (renderer->blitterEnabled && pen == 0 &&
        amigaBlitterClearPlanar(renderer->game, downwellGameRingHeight, 2) &&
        amigaBlitterClearPlanar(renderer->hud, downwellDisplayHeight, 2))
        return;
    for (index = 0; index < downwellGameRingPlaneBytes; ++index) {
        renderer->game[index] = (pen & 1u) ? 0xff : 0;
        renderer->game[downwellGameRingPlaneBytes + index] =
            (pen & 2u) ? 0xff : 0;
    }
    for (index = 0; index < downwellLayerBytes; ++index)
        renderer->hud[index] = 0;
    renderer->target = savedTarget;
    renderer->clipX = savedX;
    renderer->clipY = savedY;
    renderer->clipWidth = savedWidth;
    renderer->clipHeight = savedHeight;
}

void amigaRendererClearGameRows(AmigaRenderer* renderer,
                                int firstY, int height, unsigned pen) {
    int y;
    const UBYTE low = (pen & 1u) ? 0xff : 0;
    const UBYTE high = (pen & 2u) ? 0xff : 0;
    if (firstY < 0) { height += firstY; firstY = 0; }
    if (firstY + height > downwellDisplayHeight)
        height = downwellDisplayHeight - firstY;
    if (height <= 0) return;
    if (renderer->blitterEnabled && pen == 0) {
        unsigned row = (renderer->gameOrigin + (unsigned)firstY) % downwellGameRingHeight;
        unsigned count = (unsigned)height;
        unsigned first = count;
        unsigned plane;
        if (first > downwellGameRingHeight - row) first = downwellGameRingHeight - row;
        for (plane = 0; plane < 2; ++plane) {
            UBYTE* base = renderer->game + plane * downwellGameRingPlaneBytes;
            if (!amigaBlitterClearPlanar(base + row * downwellBytesPerPlaneLine, first, 1))
                break;
            if (first < count && !amigaBlitterClearPlanar(base, count - first, 1))
                break;
        }
        if (plane == 2) return;
    }
    for (y = firstY; y < firstY + height; ++y) {
        unsigned physicalY;
        ULONG offset;
        int byte;
        if (y < 0 || y >= downwellDisplayHeight)
            continue;
        physicalY = (renderer->gameOrigin + (unsigned)y) %
                    downwellGameRingHeight;
        offset = physicalY * downwellBytesPerPlaneLine;
        for (byte = 0; byte < downwellBytesPerPlaneLine; ++byte) {
            renderer->game[offset + byte] = low;
            renderer->game[downwellGameRingPlaneBytes + offset + byte] = high;
        }
    }
}

void amigaRendererSetClipRect(AmigaRenderer* renderer,
                              int x, int y, int width, int height) {
    renderer->target = renderer->game;
    renderer->clipX = x;
    renderer->clipY = y;
    renderer->clipWidth = width;
    renderer->clipHeight = height;
}

void amigaRendererClearClipRect(AmigaRenderer* renderer) {
    renderer->target = renderer->hud;
    renderer->clipX = 0;
    renderer->clipY = 0;
    renderer->clipWidth = downwellVisibleWidth;
    renderer->clipHeight = downwellDisplayHeight;
}

void amigaRendererFillRect(AmigaRenderer* renderer,
                           int x, int y, int width, int height,
                           unsigned pen) {
    int left = x;
    int top = y;
    int right = x + width;
    int bottom = y + height;
    int py;
    ULONG destinationPlaneBytes;
    if (left < renderer->clipX) left = renderer->clipX;
    if (top < renderer->clipY) top = renderer->clipY;
    if (right > renderer->clipX + renderer->clipWidth)
        right = renderer->clipX + renderer->clipWidth;
    if (bottom > renderer->clipY + renderer->clipHeight)
        bottom = renderer->clipY + renderer->clipHeight;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > downwellVisibleWidth) right = downwellVisibleWidth;
    if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
    if (left >= right || top >= bottom)
        return;
    if (renderer->target == renderer->hud && right-left <= 96 &&
        (!renderer->blitterEnabled || bottom-top <= 32)) {
        const int firstWord = left >> 4, lastWord = (right-1) >> 4;
        const UWORD firstMask = (UWORD)(0xffffu >> (left & 15));
        const UWORD lastMask = (UWORD)(0xffffu << (15 - ((right-1) & 15)));
        const UWORD low = pen & 1 ? 0xffff : 0, high = pen & 2 ? 0xffff : 0;
        UBYTE* row = renderer->hud + top*downwellBytesPerPlaneLine;
        for (py = top; py < bottom; ++py) {
            int word;
            for (word = firstWord; word <= lastWord; ++word) {
                UWORD mask = 0xffff;
                UBYTE* target = row + word*2;
                if (word == firstWord) mask &= firstMask;
                if (word == lastWord) mask &= lastMask;
                if (mask == 0xffff) { writeWord(target, low); writeWord(target+downwellPlaneBytes, high); }
                else {
                    writeWord(target, (readWord(target) & (UWORD)~mask) | (low & mask));
                    writeWord(target+downwellPlaneBytes,
                        (readWord(target+downwellPlaneBytes) & (UWORD)~mask) | (high & mask));
                }
            }
            row += downwellBytesPerPlaneLine;
        }
        return;
    }
    if (renderer->blitterEnabled && renderer->target == renderer->hud &&
        amigaBlitterSolidRect(renderer->hud, downwellDisplayHeight,
                             left, top, right - left, bottom - top, pen)) return;
    destinationPlaneBytes = renderer->target == renderer->game
                                ? downwellGameRingPlaneBytes
                                : downwellPlaneBytes;
    {
    const int firstByte = left >> 3;
    const int lastByte = (right - 1) >> 3;
    const UBYTE firstMask = (UBYTE)(0xffu >> (left & 7));
    const UBYTE lastMask = (UBYTE)(0xffu << (7 - ((right - 1) & 7)));
    const UBYTE low = (pen & 1) ? 0xff : 0;
    const UBYTE high = (pen & 2) ? 0xff : 0;
    for (py = top; py < bottom; ++py) {
        unsigned targetY = (unsigned)py;
        ULONG rowOffset;
        int destinationByte;
        if (renderer->target == renderer->game)
            targetY = (renderer->gameOrigin + targetY) %
                      downwellGameRingHeight;
        rowOffset = targetY * downwellBytesPerPlaneLine;
        for (destinationByte = firstByte;
             destinationByte <= lastByte; ++destinationByte) {
            UBYTE mask = 0xff;
            UBYTE* target0;
            UBYTE* target1;
            if (destinationByte == firstByte) mask &= firstMask;
            if (destinationByte == lastByte) mask &= lastMask;
            target0 = renderer->target + rowOffset + destinationByte;
            target1 = target0 + destinationPlaneBytes;
            *target0 = (UBYTE)((*target0 & (UBYTE)~mask) |
                               (low & mask));
            *target1 = (UBYTE)((*target1 & (UBYTE)~mask) |
                               (high & mask));
        }
    }
    }
}

void amigaRendererCacheHudColumns(UWORD* cache, const UBYTE* source, unsigned firstWord, unsigned words) {
    unsigned y, plane, word;
    for (y = 0; y < downwellDisplayHeight; ++y)
        for (plane = 0; plane < 2; ++plane)
            for (word = 0; word < words; ++word)
                *cache++ = readWord(source + plane*downwellPlaneBytes + y*downwellBytesPerPlaneLine + (firstWord+word)*2);
}

void amigaRendererApplyHudColumns(AmigaRenderer* renderer, const UWORD* cache,
    unsigned firstWord, unsigned words, int shiftY, int top, int bottom, int masked) {
    int y, word;
    UWORD clips[3];
    const UWORD* source;
    UBYTE* targetRow;
    if (words > 3) return;
    if (top < renderer->clipY) top = renderer->clipY;
    if (bottom > renderer->clipY + renderer->clipHeight) bottom = renderer->clipY + renderer->clipHeight;
    if (top < shiftY) top = shiftY;
    if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
    if (bottom > downwellDisplayHeight + shiftY) bottom = downwellDisplayHeight + shiftY;
    if (top < 0) top = 0;
    for (word = 0; word < (int)words; ++word) {
        const int x = (firstWord+word)*16;
        int left = renderer->clipX > x ? renderer->clipX-x : 0;
        int right = renderer->clipX+renderer->clipWidth-x;
        if (right > 16) right = 16;
        clips[word] = left >= right || x >= downwellVisibleWidth ? 0 :
            (UWORD)((0xffffu >> left) & (0xffffu << (16-right)));
    }
    source = cache + (top-shiftY)*words*2;
    targetRow = renderer->hud + top*downwellBytesPerPlaneLine + firstWord*2;
    for (y = top; y < bottom; ++y) {
        for (word = 0; word < (int)words; ++word) {
            UWORD mask = clips[word];
            UBYTE* target;
            if (masked) mask &= source[word] | source[words+word];
            if (!mask) continue;
            target = targetRow + word*2;
            if (mask == 0xffff) {
                writeWord(target, source[word]); writeWord(target+downwellPlaneBytes, source[words+word]);
            } else {
                writeWord(target, (readWord(target) & (UWORD)~mask) | (source[word] & mask));
                target += downwellPlaneBytes;
                writeWord(target, (readWord(target) & (UWORD)~mask) | (source[words+word] & mask));
            }
        }
        source += words*2; targetRow += downwellBytesPerPlaneLine;
    }
}

void amigaRendererFillHudWordColumn(UBYTE* hud, unsigned word, unsigned top,
                                    unsigned bottom, UWORD mask, UWORD low, UWORD high) {
    UBYTE* target;
    if (word >= downwellBytesPerPlaneLine/2 || top >= bottom || bottom > downwellDisplayHeight) return;
    target = hud + top*downwellBytesPerPlaneLine + word*2;
    low &= mask; high &= mask;
    for (; top < bottom; ++top) {
        writeWord(target, (readWord(target) & (UWORD)~mask) | low);
        writeWord(target+downwellPlaneBytes,
                  (readWord(target+downwellPlaneBytes) & (UWORD)~mask) | high);
        target += downwellBytesPerPlaneLine;
    }
}

unsigned amigaRendererCopyHudWords(UBYTE* destination, const UBYTE* source,
                                    int left, int top, int right, int bottom, int changedOnly) {
    unsigned writes = 0, plane;
    int y;
    if (left < 0) left = 0;
    if (right > downwellVisibleWidth) right = downwellVisibleWidth;
    if (top < 0) top = 0;
    if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
    if (left >= right || top >= bottom) return 0;
    left >>= 4; right = (right+15) >> 4;
    if (changedOnly && right-left == 1) {
        /* Gauge interiors occupy one word. Walk both planes together without
           a per-row width loop, still never writing an unchanged plane. */
        const ULONG start = top*downwellBytesPerPlaneLine + left*2;
        UBYTE* target = destination + start;
        const UBYTE* input = source + start;
        for (y = top; y < bottom; ++y) {
            const UWORD low = readWord(input);
            const UWORD high = readWord(input+downwellPlaneBytes);
            if (readWord(target) != low) { writeWord(target, low); ++writes; }
            if (readWord(target+downwellPlaneBytes) != high) {
                writeWord(target+downwellPlaneBytes, high); ++writes;
            }
            target += downwellBytesPerPlaneLine; input += downwellBytesPerPlaneLine;
        }
        return writes;
    }
    for (plane = 0; plane < 2; ++plane) {
        const ULONG start = plane*downwellPlaneBytes + top*downwellBytesPerPlaneLine + left*2;
        UBYTE* target = destination + start;
        const UBYTE* input = source + start;
        for (y = top; y < bottom; ++y) {
            int word;
            for (word = 0; word < right-left; ++word) {
                const UWORD value = readWord(input + word*2);
                if (!changedOnly || readWord(target + word*2) != value) {
                    writeWord(target + word*2, value); ++writes;
                }
            }
            target += downwellBytesPerPlaneLine; input += downwellBytesPerPlaneLine;
        }
    }
    return writes;
}

unsigned amigaRendererPublishHudColumn(UBYTE* destination, UBYTE* working,
    const UWORD* overlay, unsigned word, int top, int bottom, int shiftY, UWORD clip) {
    unsigned writes = 0;
    UBYTE* target;
    UBYTE* input;
    const UWORD* source;
    int first = top > shiftY ? top : shiftY;
    int last = bottom < downwellDisplayHeight+shiftY ? bottom : downwellDisplayHeight+shiftY;
    int y;
    if (word >= downwellVisibleWidth / 16 || top < 0 ||
        bottom > downwellDisplayHeight || top >= bottom) return 0;
    if (first >= last)
        return amigaRendererCopyHudWords(destination, working, word*16, top, word*16+16, bottom, 1);
    if (first > top) writes += amigaRendererCopyHudWords(destination, working,
        word*16, top, word*16+16, first, 1);
    if (last < bottom) writes += amigaRendererCopyHudWords(destination, working,
        word*16, last, word*16+16, bottom, 1);
    target = destination + first*downwellBytesPerPlaneLine + word*2;
    input = working + first*downwellBytesPerPlaneLine + word*2;
    source = overlay + (first-shiftY)*2;
    for (y = first; y < last; ++y) {
        UWORD low = readWord(input), high = readWord(input+downwellPlaneBytes);
        {
            const UWORD a = source[0], b = source[1];
            const UWORD mask = (a | b) & clip;
            if (mask) {
                low = (low & (UWORD)~mask) | (a & mask);
                high = (high & (UWORD)~mask) | (b & mask);
                writeWord(input, low);
                writeWord(input+downwellPlaneBytes, high);
            }
        }
        if (readWord(target) != low) { writeWord(target, low); ++writes; }
        if (readWord(target+downwellPlaneBytes) != high) {
            writeWord(target+downwellPlaneBytes, high); ++writes;
        }
        input += downwellBytesPerPlaneLine;
        target += downwellBytesPerPlaneLine;
        source += 2;
    }
    return writes;
}

int amigaRendererPrepareWordStamp(AmigaWordStamp* stamp, const UBYTE* pack,
                                  const UBYTE* end, UWORD sprite, UWORD frame) {
    const UBYTE* record = findAsset(pack, end, sprite, frame);
    unsigned row, height, width;
    const UBYTE* data;
    if (!record || readWord(record+4) || readWord(record+6) || readWord(record+12) != 16) return 0;
    width = readWord(record+8); height = readWord(record+10);
    if (!width || width > 16 || !height || height > 8) return 0;
    if (readLong(record+16) > (ULONG)(end-pack) ||
        (ULONG)(end-pack) - readLong(record+16) < height*4) return 0;
    data = pack + readLong(record+16);
    stamp->first = 8; stamp->last = 0;
    for (row = 0; row < 8; ++row) {
        stamp->low[row] = row < height ? readWord(data + row*2) : 0;
        stamp->high[row] = row < height ? readWord(data + height*2 + row*2) : 0;
        stamp->mask[row] = (stamp->low[row] | stamp->high[row]) & (UWORD)(0xffffu << (16-width));
        if (stamp->mask[row]) {
            if (row < stamp->first) stamp->first = row;
            stamp->last = row+1;
        }
    }
    /* Retain all short row-overlap combinations. Repeated stamps drawn one
       pixel upward become one output word per scanline instead of eight
       overlapping masked draws. Highest source row is drawn last. */
    for (row = 0; row < 8; ++row) {
        unsigned last;
        UWORD low = 0, high = 0, mask = 0;
        for (last = row; last < 8; ++last) {
            const UWORD next = stamp->mask[last];
            low = (low & (UWORD)~next) | (stamp->low[last] & next);
            high = (high & (UWORD)~next) | (stamp->high[last] & next);
            mask |= next;
            stamp->runLow[row*8+last] = low;
            stamp->runHigh[row*8+last] = high;
            stamp->runMask[row*8+last] = mask;
        }
    }
    return 1;
}

int amigaRendererDrawWordStamp(AmigaRenderer* renderer, const AmigaWordStamp* stamp, int x, int y) {
    return amigaRendererDrawWordStampRun(renderer, stamp, x, y, 1);
}
int amigaRendererDrawWordStampRun(AmigaRenderer* renderer, const AmigaWordStamp* stamp, int x, int y, unsigned count) {
    int first = y + stamp->first - (int)count + 1, last = y + stamp->last, left, right, row;
    UWORD clip;
    UBYTE* target;
    if (renderer->target != renderer->hud || x < 0 || (x & 15)) return 0;
    if (!count || stamp->first >= stamp->last) return 1;
    left = renderer->clipX > x ? renderer->clipX : x;
    right = renderer->clipX + renderer->clipWidth;
    if (right > x+16) right = x+16;
    if (right > downwellVisibleWidth) right = downwellVisibleWidth;
    if (first < renderer->clipY) first = renderer->clipY;
    if (first < 0) first = 0;
    if (last > renderer->clipY + renderer->clipHeight) last = renderer->clipY + renderer->clipHeight;
    if (last > downwellDisplayHeight) last = downwellDisplayHeight;
    if (first >= last || left >= right) return 1;
    clip = (UWORD)((0xffffu >> (left-x)) & (0xffffu << (16-(right-x))));
    target = renderer->hud + first*downwellBytesPerPlaneLine + (x >> 3);
    for (row = first-y; row < last-y; ++row) {
        /* Long repeated runs have a constant fully-overlapped middle. Use
           its already-composed words once, not three table lookups and two
           source-row clamps for every scanline. Only the ends vary. */
        if (row <= 0 && row + (int)count - 1 >= 7) {
            const int end = last-y < 1 ? last-y : 1;
            const UWORD mask = stamp->runMask[7] & clip;
            if (mask) amigaRendererFillHudWordColumn(renderer->hud, x >> 4,
                row+y, end+y, mask, stamp->runLow[7], stamp->runHigh[7]);
            target += (end-row)*downwellBytesPerPlaneLine;
            row = end-1;
            continue;
        }
        const unsigned startRow = row < 0 ? 0 : row;
        const unsigned endRow = row + (int)count - 1 > 7 ? 7 : row + count - 1;
        const unsigned index = startRow*8 + endRow;
        const UWORD mask = stamp->runMask[index] & clip;
        if (mask) {
            writeWord(target, (readWord(target) & (UWORD)~mask) | (stamp->runLow[index] & mask));
            writeWord(target + downwellPlaneBytes,
                (readWord(target + downwellPlaneBytes) & (UWORD)~mask) | (stamp->runHigh[index] & mask));
        }
        target += downwellBytesPerPlaneLine;
    }
    return 1;
}

int amigaRendererAssetBounds(const UBYTE* pack, const UBYTE* packEnd,
                             UWORD sprite, UWORD frame, int anchorX,
                             int anchorY, int flipX, int* x, int* y,
                             int* width, int* height) {
    const UBYTE* record = findAsset(pack, packEnd, sprite, frame);
    if (!record) return 0;
    *width = readWord(record + 8);
    *height = readWord(record + 10);
    *x = flipX ? anchorX + (WORD)readWord(record + 4) - *width
               : anchorX - (WORD)readWord(record + 4);
    *y = anchorY - (WORD)readWord(record + 6);
    return 1;
}

void amigaRendererDrawAsset(AmigaRenderer* renderer,
                            const UBYTE* pack, const UBYTE* packEnd,
                            UWORD sprite, UWORD frame,
                            int anchorX, int anchorY, int flipX) {
    const UBYTE* record = flipX ? findAsset(pack, packEnd, 62000 + sprite, frame) : 0;
    if (record) flipX = 0;
    else record = findAsset(pack, packEnd, sprite, frame);
    const UBYTE* data;
    int originX;
    int originY;
    int width;
    int height;
    int dataWidth;
    int planeBytes;
    int y;
    int x;
    if (!record)
        return;
    originX = (WORD)readWord(record + 4);
    originY = (WORD)readWord(record + 6);
    width = readWord(record + 8);
    height = readWord(record + 10);
    dataWidth = readWord(record + 12);
    planeBytes = dataWidth >> 3;
    data = pack + readLong(record + 16);
    if (data < pack || data + readLong(record + 20) > packEnd)
        return;

    if (!flipX) {
        drawAssetNormal(renderer, data, originX, originY,
                        width, height, planeBytes, anchorX, anchorY, -1);
        return;
    }

    for (y = 0; y < height; ++y) {
        const ULONG sourcePlaneBytes = (ULONG)planeBytes * (ULONG)height;
        const UBYTE* row = data + y * planeBytes;
        for (x = 0; x < width; ++x) {
            const UBYTE mask = (UBYTE)(0x80u >> (x & 7));
            unsigned pen = (row[x >> 3] & mask) ? 1u : 0u;
            pen |= (row[sourcePlaneBytes + (x >> 3)] & mask) ? 2u : 0u;
            if (pen) {
                const int drawX = flipX ? anchorX + originX - 1 - x
                                        : anchorX - originX + x;
                setPixel(renderer, drawX, anchorY - originY + y, pen);
            }
        }
    }
}

void amigaRendererDrawAssetPen(AmigaRenderer* renderer,
                               const UBYTE* pack, const UBYTE* packEnd,
                               UWORD sprite, UWORD frame,
                               int anchorX, int anchorY, int flipX,
                               unsigned outputPen) {
    const UBYTE* record = findAsset(pack, packEnd, sprite, frame);
    const UBYTE* data;
    int originX;
    int originY;
    int width;
    int height;
    int dataWidth;
    int planeBytes;
    int y;
    int x;
    if (!record)
        return;
    originX = (WORD)readWord(record + 4);
    originY = (WORD)readWord(record + 6);
    width = readWord(record + 8);
    height = readWord(record + 10);
    dataWidth = readWord(record + 12);
    planeBytes = dataWidth >> 3;
    data = pack + readLong(record + 16);
    if (data < pack || data + readLong(record + 20) > packEnd)
        return;
    if (!flipX) {
        drawAssetNormal(renderer, data, originX, originY,
                        width, height, planeBytes, anchorX, anchorY, (int)(outputPen & 3));
        return;
    }
    for (y = 0; y < height; ++y) {
        const ULONG sourcePlaneBytes = (ULONG)planeBytes * (ULONG)height;
        const UBYTE* row = data + y * planeBytes;
        for (x = 0; x < width; ++x) {
            const UBYTE mask = (UBYTE)(0x80u >> (x & 7));
            if ((row[x >> 3] | row[sourcePlaneBytes + (x >> 3)]) & mask) {
                const int drawX = flipX ? anchorX + originX - 1 - x
                                        : anchorX - originX + x;
                setPixel(renderer, drawX, anchorY - originY + y, outputPen);
            }
        }
    }
}

static int hardwareSpriteOpaqueBounds(const UBYTE* record,
                                      const UBYTE* pack,
                                      const UBYTE* packEnd,
                                      int* left, int* right) {
    const int width = readWord(record + 8);
    const int height = readWord(record + 10);
    const int planeBytes = readWord(record + 12) >> 3;
    const ULONG planeSize = (ULONG)planeBytes * (ULONG)height;
    const UBYTE* data = pack + readLong(record + 16);
    int minimum = width;
    int maximum = -1;
    int y;
    int x;
    if (data < pack || data + readLong(record + 20) > packEnd ||
        readLong(record + 20) < planeSize * 2)
        return 0;
    for (y = 0; y < height; ++y) {
        const UBYTE* plane0 = data + y * planeBytes;
        const UBYTE* plane1 = plane0 + planeSize;
        for (x = 0; x < width; ++x) {
            const UBYTE mask = (UBYTE)(0x80u >> (x & 7));
            if (((plane0[x >> 3] | plane1[x >> 3]) & mask) == 0)
                continue;
            if (x < minimum) minimum = x;
            if (x > maximum) maximum = x;
        }
    }
    if (maximum < minimum) {
        minimum = 0;
        maximum = 0;
    }
    *left = minimum;
    *right = maximum;
    return 1;
}

unsigned amigaRendererHardwareSpritePartCount(
    const UBYTE* pack, const UBYTE* packEnd, UWORD sprite, UWORD frame) {
    const UBYTE* prepared = findAsset(pack, packEnd, 63000 + sprite, frame * 8);
    if (prepared) return readWord(prepared + 14) >> 8;
#ifdef DOWNWELL_PLATFORM_AMIGA
    return 0; // A missing offline asset is an error, never runtime conversion.
#endif
    const UBYTE* record = findAsset(pack, packEnd, sprite, frame);
    int left;
    int right;
    if (!record || !hardwareSpriteOpaqueBounds(
            record, pack, packEnd, &left, &right))
        return 0;
    return (unsigned)(right - left + 16) / 16u;
}

/* Gameplay part streams stay in ordinary RAM; garden DMA records (flag 4,
   no part count) remain resident. Compact only their unused directory entries,
   never the bytes/relative layout of the retained planar and garden payload. */
static int chipPackLayout(const UBYTE* pack,const UBYTE* end,ULONG* count,
                         ULONG* retained,ULONG* payload,ULONG* cutoff) {
    ULONG i,size,previous=0;
    if(!pack||!end||end-pack<20)return 0;
    size=(ULONG)(end-pack);
    if(readLong(pack)!=0x44574150UL||readWord(pack+4)!=2||readWord(pack+6)!=20)return 0;
    *count=readLong(pack+8);
    if(*count>(size-20)/24)return 0;
    *payload=20+*count*24;*cutoff=size;*retained=0;
    for(i=0;i<*count;++i){
        const UBYTE* r=pack+20+i*24;
        const ULONG key=readLong(r),offset=readLong(r+16),length=readLong(r+20);
        const UWORD flags=readWord(r+14);
        if((i&&key<=previous)||offset<*payload||(offset&1)||offset>size||length>size-offset)return 0;
        previous=key;
        if((flags&4)&&(flags>>8)){if(offset<*cutoff)*cutoff=offset;}
        else ++*retained;
    }
    for(i=0;i<*count;++i){
        const UBYTE* r=pack+20+i*24;const UWORD flags=readWord(r+14);
        if(!((flags&4)&&(flags>>8))&&
           (readLong(r+16)>*cutoff||readLong(r+20)>*cutoff-readLong(r+16)))return 0;
    }
    return 1;
}
ULONG amigaRendererChipPackBytes(const UBYTE* pack, const UBYTE* end) {
    ULONG count,retained,payload,cutoff;
    return chipPackLayout(pack,end,&count,&retained,&payload,&cutoff)
        ?20+retained*24+cutoff-payload:0;
}
int amigaRendererCopyChipPack(const UBYTE* pack,const UBYTE* end,UBYTE* destination,ULONG bytes){
    ULONG count,retained,payload,cutoff,i,j,out=20,shift,size;
    if(!destination||!chipPackLayout(pack,end,&count,&retained,&payload,&cutoff))return 0;
    shift=(count-retained)*24;size=cutoff-shift;
    if(bytes<size)return 0;
    for(i=0;i<20;++i)destination[i]=pack[i];
    writeWord(destination+8,(UWORD)(retained>>16));writeWord(destination+10,(UWORD)retained);
    for(i=0;i<count;++i){
        const UBYTE* r=pack+20+i*24;const UWORD flags=readWord(r+14);
        ULONG offset;
        if((flags&4)&&(flags>>8))continue;
        for(j=0;j<24;++j)destination[out+j]=r[j];
        offset=readLong(r+16)-shift;
        writeWord(destination+out+16,(UWORD)(offset>>16));writeWord(destination+out+18,(UWORD)offset);
        out+=24;
    }
    for(i=payload;i<cutoff;++i)destination[i-shift]=pack[i];
    return 1;
}

static int buildHardwareSpritePart(
    const UBYTE* prepared,
    const UBYTE* pack, const UBYTE* packEnd,
    UWORD sprite, UWORD frame, unsigned part,
    int anchorX, int anchorY, int flipX,
    UWORD* destination, unsigned destinationHeight) {
    const UBYTE* record;
    const UBYTE* data;
    UBYTE* control = (UBYTE*)destination;
    int originX;
    int originY;
    int width;
    int height;
    int dataWidth;
    int planeBytes;
    int cropX;
    int opaqueLeft;
    int opaqueRight;
    int displayedLeft;
    int displayedRight;
    unsigned partCount;
    int logicalStartX;
    int visibleLeft;
    int visibleRight;
    int startX;
    int startY;
    int stopY;
    int logicalTop;
    int sourceTop;
    int visibleHeight;
    int y;
    int x;
    if (prepared) {
        height = readWord(prepared + 10);
        if (height > (int)destinationHeight) return 0;
        logicalStartX = anchorX - (WORD)readWord(prepared + 4);
        logicalTop = anchorY - (WORD)readWord(prepared + 6);
        sourceTop = logicalTop < 0 ? -logicalTop : 0;
        visibleHeight = height - sourceTop;
        if (logicalTop < 0) logicalTop = 0;
        if (logicalTop + visibleHeight > downwellDisplayHeight)
            visibleHeight = downwellDisplayHeight - logicalTop;
        if (logicalStartX + 16 <= downwellPlayfieldX ||
            logicalStartX >= downwellPlayfieldX + downwellPlayfieldWidth ||
            visibleHeight <= 0) {
            destination[0] = destination[1] = 0;
            return 2;
        }
        data = pack + readLong(prepared + 16);
        if (data + height * 4 > packEnd || readLong(prepared + 20) < (ULONG)height * 4)
            return 0;
        startX = downwellDisplayHStart + logicalStartX;
        startY = downwellDisplayVStart + logicalTop;
        stopY = startY + visibleHeight;
        control[0] = (UBYTE)startY;
        control[1] = (UBYTE)(startX >> 1);
        control[2] = (UBYTE)stopY;
        control[3] = (UBYTE)((startX & 1) | (stopY > 255 ? 2 : 0) | (startY > 255 ? 4 : 0));
        /* Immutable host-converted words. PF2 priority masks horizontal edges. */
#ifdef DOWNWELL_PLATFORM_AMIGA
        {
            static struct {
                UWORD* destination;
                const UBYTE* source;
                int height;
            } slots[16];
            unsigned slot;
            const UBYTE* source = data + sourceTop * 4;
            for (slot = 0; slot < 16; ++slot)
                if (!slots[slot].destination || slots[slot].destination == destination)
                    break;
            if (slot < 16) {
                if (slots[slot].destination == destination &&
                    slots[slot].source == source && slots[slot].height == visibleHeight) {
                    DW_PROFILE_COUNT(counterSpriteHeaderOnly);
                    return 1; // Only the four-byte position header changed.
                }
                slots[slot].destination = destination;
                slots[slot].source = source;
                slots[slot].height = visibleHeight;
            }
        }
#endif
        DW_PROFILE_COUNT(counterSpritePayloadCopies);
        DW_PROFILE_ADD(counterSpritePayloadWords, visibleHeight * 2);
        for (y = 0; y < visibleHeight * 2; ++y)
            destination[2 + y] = readWord(data + sourceTop * 4 + y * 2);
        destination[2 + visibleHeight * 2] = destination[3 + visibleHeight * 2] = 0;
        return 1;
    }
#ifdef DOWNWELL_PLATFORM_AMIGA
    return 0;
#endif
    record = findAsset(pack, packEnd, sprite, frame);
    if (!record)
        return 0;
    originX = (WORD)readWord(record + 4);
    originY = (WORD)readWord(record + 6);
    width = readWord(record + 8);
    height = readWord(record + 10);
    dataWidth = readWord(record + 12);
    planeBytes = dataWidth >> 3;
    data = pack + readLong(record + 16);
    if (height > (int)destinationHeight || width < 16 ||
        data < pack || data + readLong(record + 20) > packEnd)
        return 0;

    if (!hardwareSpriteOpaqueBounds(record, pack, packEnd,
                                    &opaqueLeft, &opaqueRight))
        return 0;
    displayedLeft = flipX ? width - 1 - opaqueRight : opaqueLeft;
    displayedRight = flipX ? width - 1 - opaqueLeft : opaqueRight;
    partCount = (unsigned)(displayedRight - displayedLeft + 16) / 16u;
    if (part >= partCount)
        return 0;
    if (partCount == 1) {
        cropX = displayedLeft -
                (16 - (displayedRight - displayedLeft + 1)) / 2;
        if (cropX < 0) cropX = 0;
        if (cropX > width - 16) cropX = width - 16;
    } else {
        cropX = displayedLeft + (int)part * 16;
    }
    logicalStartX = flipX ? anchorX + originX - width + cropX
                          : anchorX - originX + cropX;
    visibleLeft = logicalStartX;
    visibleRight = logicalStartX + 16;
    if (visibleLeft < downwellPlayfieldX)
        visibleLeft = downwellPlayfieldX;
    if (visibleRight > downwellPlayfieldX + downwellPlayfieldWidth)
        visibleRight = downwellPlayfieldX + downwellPlayfieldWidth;
    if (visibleLeft >= visibleRight) {
        destination[0] = 0;
        destination[1] = 0;
        destination[2] = 0;
        destination[3] = 0;
        return 2;
    }
    logicalTop = anchorY - originY;
    sourceTop = 0;
    visibleHeight = height;
    if (logicalTop < 0) {
        sourceTop = -logicalTop;
        visibleHeight -= sourceTop;
        logicalTop = 0;
    }
    if (logicalTop + visibleHeight > downwellDisplayHeight)
        visibleHeight = downwellDisplayHeight - logicalTop;
    if (visibleHeight <= 0) {
        destination[0] = 0;
        destination[1] = 0;
        destination[2] = 0;
        destination[3] = 0;
        return 2;
    }
    startX = downwellDisplayHStart + logicalStartX;
    startY = downwellDisplayVStart + logicalTop;
    stopY = startY + visibleHeight;
    control[0] = (UBYTE)startY;
    control[1] = (UBYTE)(startX >> 1);
    control[2] = (UBYTE)stopY;
    control[3] = (UBYTE)((startX & 1) |
                         (stopY > 255 ? 2 : 0) |
                         (startY > 255 ? 4 : 0));

    for (y = 0; y < visibleHeight; ++y) {
        const ULONG sourcePlaneBytes = (ULONG)planeBytes * (ULONG)height;
        const UBYTE* row = data + (sourceTop + y) * planeBytes;
        UWORD plane0 = 0;
        UWORD plane1 = 0;
        for (x = 0; x < 16; ++x) {
            const int logicalX = logicalStartX + x;
            const int sourceX = flipX ? width - 1 - (cropX + x)
                                      : cropX + x;
            if (logicalX < visibleLeft || logicalX >= visibleRight)
                continue;
            if (sourceX < 0 || sourceX >= width)
                continue;
            const UBYTE mask = (UBYTE)(0x80u >> (sourceX & 7));
            if (row[sourceX >> 3] & mask)
                plane0 |= (UWORD)(0x8000u >> x);
            if (row[sourcePlaneBytes + (sourceX >> 3)] & mask)
                plane1 |= (UWORD)(0x8000u >> x);
        }
        destination[2 + y * 2] = plane0;
        destination[3 + y * 2] = plane1;
    }
    destination[2 + visibleHeight * 2] = 0;
    destination[3 + visibleHeight * 2] = 0;
    return 1;
}

int amigaRendererResolveSpritePart(AmigaResolvedSpritePart* output,
    const UBYTE* pack, const UBYTE* end, UWORD sprite, UWORD frame, unsigned part, int flip) {
    output->record = findAsset(pack, end, 63000 + sprite, frame*8 + (flip ? 4 : 0) + part);
    output->pack = pack;
    output->end = end;
    output->partCount = output->record ? readWord(output->record+14) >> 8 : 0;
    return output->record != 0 && output->partCount != 0;
}

int amigaRendererDrawResolvedSpritePart(const AmigaResolvedSpritePart* prepared,
    int x, int y, UWORD* destination, unsigned height) {
    if (!prepared->record || !prepared->partCount) return 0;
    return buildHardwareSpritePart(prepared->record, prepared->pack, prepared->end,
        0, 0, 0, x, y, 0, destination, height);
}

int amigaRendererBuildHardwareSpritePart(const UBYTE* pack, const UBYTE* end,
    UWORD sprite, UWORD frame, unsigned part, int x, int y, int flip,
    UWORD* destination, unsigned height) {
    const UBYTE* prepared = findAsset(pack, end, 63000 + sprite, frame*8 + (flip ? 4 : 0) + part);
    return buildHardwareSpritePart(prepared, pack, end, sprite, frame, part,
        x, y, flip, destination, height);
}

int amigaRendererBuildHardwareSprite(const UBYTE* pack, const UBYTE* packEnd,
                                     UWORD sprite, UWORD frame,
                                     int anchorX, int anchorY, int flipX,
                                     UWORD* destination,
                                     unsigned destinationHeight) {
    return amigaRendererBuildHardwareSpritePart(
        pack, packEnd, sprite, frame, 0,
        anchorX, anchorY, flipX, destination, destinationHeight);
}
