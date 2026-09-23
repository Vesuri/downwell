#include "AmigaRenderer.h"

extern int amigaBlitterDrawPlanarMasked(
    const UBYTE* source, const UBYTE* mask,
    UWORD sourceWidth, UWORD sourceHeight, UWORD sourceDataWidth,
    UBYTE* destination, UWORD destinationHeight,
    UWORD destinationX, UWORD destinationY,
    UWORD sourceX, UWORD sourceY, UWORD width, UWORD height);
extern int amigaBlitterClearPlanar(UBYTE* destination,
                                   UWORD height, UWORD planes);

static UWORD readWord(const UBYTE* data) {
    return (UWORD)(((UWORD)data[0] << 8) | data[1]);
}

static ULONG readLong(const UBYTE* data) {
    return ((ULONG)readWord(data) << 16) | readWord(data + 2);
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
                            int anchorX, int anchorY) {
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
                    (UWORD)(clipRight - clipLeft), (UWORD)rows))
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
    if (packEnd - pack < 20 || readLong(pack) != 0x44574150UL ||
        readWord(pack + 4) != 2)
        return 0;
    count = readLong(pack + 8);
    records = pack + readWord(pack + 6);
    if (records < pack || records + count * 24 > packEnd)
        return 0;
    last = count;
    while (first < last) {
        const ULONG middle = first + (last - first) / 2;
        const UBYTE* record = records + middle * 24;
        const ULONG key = readLong(record);
        if (key < wanted)
            first = middle + 1;
        else if (key > wanted)
            last = middle;
        else
            return record;
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
    destinationPlaneBytes = renderer->target == renderer->game
                                ? downwellGameRingPlaneBytes
                                : downwellPlaneBytes;
    for (py = top; py < bottom; ++py) {
        unsigned targetY = (unsigned)py;
        ULONG rowOffset;
        int destinationByte;
        const int firstByte = left >> 3;
        const int lastByte = (right - 1) >> 3;
        if (renderer->target == renderer->game)
            targetY = (renderer->gameOrigin + targetY) %
                      downwellGameRingHeight;
        rowOffset = targetY * downwellBytesPerPlaneLine;
        for (destinationByte = firstByte;
             destinationByte <= lastByte; ++destinationByte) {
            const int destinationX = destinationByte << 3;
            int firstBit = left - destinationX;
            int lastBit = right - destinationX;
            UBYTE mask;
            UBYTE* target0;
            UBYTE* target1;
            if (firstBit < 0) firstBit = 0;
            if (lastBit > 8) lastBit = 8;
            mask = spanMask(firstBit, lastBit);
            target0 = renderer->target + rowOffset + destinationByte;
            target1 = target0 + destinationPlaneBytes;
            *target0 = (UBYTE)((*target0 & (UBYTE)~mask) |
                               ((pen & 1u) ? mask : 0));
            *target1 = (UBYTE)((*target1 & (UBYTE)~mask) |
                               ((pen & 2u) ? mask : 0));
        }
    }
}

void amigaRendererDrawAsset(AmigaRenderer* renderer,
                            const UBYTE* pack, const UBYTE* packEnd,
                            UWORD sprite, UWORD frame,
                            int anchorX, int anchorY, int flipX) {
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
                        width, height, planeBytes, anchorX, anchorY);
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

ULONG amigaRendererChipPackBytes(const UBYTE* pack, const UBYTE* packEnd) {
    const UBYTE* first = findAsset(pack, packEnd, 63000, 0);
    return first ? readLong(first + 16) : (ULONG)(packEnd - pack);
}

int amigaRendererBuildHardwareSpritePart(
    const UBYTE* pack, const UBYTE* packEnd,
    UWORD sprite, UWORD frame, unsigned part,
    int anchorX, int anchorY, int flipX,
    UWORD* destination, unsigned destinationHeight) {
    const UBYTE* record = findAsset(pack, packEnd, sprite, frame);
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
    const UBYTE* prepared = findAsset(pack, packEnd, 63000 + sprite,
                                     frame * 8 + (flipX ? 4 : 0) + part);
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
                    slots[slot].source == source && slots[slot].height == visibleHeight)
                    return 1; // Only the four-byte position header changed.
                slots[slot].destination = destination;
                slots[slot].source = source;
                slots[slot].height = visibleHeight;
            }
        }
#endif
        for (y = 0; y < visibleHeight * 2; ++y)
            destination[2 + y] = readWord(data + sourceTop * 4 + y * 2);
        destination[2 + visibleHeight * 2] = destination[3 + visibleHeight * 2] = 0;
        return 1;
    }
#ifdef DOWNWELL_PLATFORM_AMIGA
    return 0;
#endif
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

int amigaRendererBuildHardwareSprite(const UBYTE* pack, const UBYTE* packEnd,
                                     UWORD sprite, UWORD frame,
                                     int anchorX, int anchorY, int flipX,
                                     UWORD* destination,
                                     unsigned destinationHeight) {
    return amigaRendererBuildHardwareSpritePart(
        pack, packEnd, sprite, frame, 0,
        anchorX, anchorY, flipX, destination, destinationHeight);
}
