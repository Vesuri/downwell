#include "AmigaRenderer.h"

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
    for (y = firstY; y < lastY; ++y) {
        const UBYTE* source = data + (y - top) * planeBytes * 2;
        const UBYTE* source0 = source;
        const UBYTE* source1 = source + planeBytes;
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
    const UBYTE* record;
    ULONG count;
    ULONG index;
    if (packEnd - pack < 20 || readLong(pack) != 0x44574150UL ||
        readWord(pack + 4) != 1)
        return 0;
    count = readLong(pack + 8);
    record = pack + readWord(pack + 6);
    if (record < pack || record + count * 24 > packEnd)
        return 0;
    for (index = 0; index < count; ++index, record += 24)
        if (readWord(record) == sprite && readWord(record + 2) == frame)
            return record;
    return 0;
}

void amigaRendererInit(AmigaRenderer* renderer, UBYTE* game, UBYTE* hud) {
    renderer->game = game;
    renderer->hud = hud;
    renderer->gameOrigin = 0;
    amigaRendererClearClipRect(renderer);
}

void amigaRendererSetGameOrigin(AmigaRenderer* renderer, unsigned origin) {
    renderer->gameOrigin = origin % downwellGameRingHeight;
}

void amigaRendererClear(AmigaRenderer* renderer, unsigned pen) {
    ULONG index;
    UBYTE* savedTarget = renderer->target;
    int savedX = renderer->clipX;
    int savedY = renderer->clipY;
    int savedWidth = renderer->clipWidth;
    int savedHeight = renderer->clipHeight;
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
        const UBYTE* row = data + y * planeBytes * 2;
        for (x = 0; x < width; ++x) {
            const UBYTE mask = (UBYTE)(0x80u >> (x & 7));
            unsigned pen = (row[x >> 3] & mask) ? 1u : 0u;
            pen |= (row[planeBytes + (x >> 3)] & mask) ? 2u : 0u;
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
        const UBYTE* row = data + y * planeBytes * 2;
        for (x = 0; x < width; ++x) {
            const UBYTE mask = (UBYTE)(0x80u >> (x & 7));
            if ((row[x >> 3] | row[planeBytes + (x >> 3)]) & mask) {
                const int drawX = flipX ? anchorX + originX - 1 - x
                                        : anchorX - originX + x;
                setPixel(renderer, drawX, anchorY - originY + y, outputPen);
            }
        }
    }
}

int amigaRendererBuildHardwareSprite(const UBYTE* pack, const UBYTE* packEnd,
                                     UWORD sprite, UWORD frame,
                                     int anchorX, int anchorY, int flipX,
                                     UWORD* destination,
                                     unsigned destinationHeight) {
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
    int startX;
    int startY;
    int stopY;
    int y;
    int x;
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

    cropX = (width - 16) >> 1;
    startX = downwellDisplayHStart +
             (flipX ? anchorX + originX - width + cropX
                    : anchorX - originX + cropX);
    startY = downwellDisplayVStart + anchorY - originY;
    stopY = startY + height;
    control[0] = (UBYTE)startY;
    control[1] = (UBYTE)(startX >> 1);
    control[2] = (UBYTE)stopY;
    control[3] = (UBYTE)((startX & 1) |
                         (stopY > 255 ? 2 : 0) |
                         (startY > 255 ? 4 : 0));

    for (y = 0; y < height; ++y) {
        const UBYTE* row = data + y * planeBytes * 2;
        UWORD plane0 = 0;
        UWORD plane1 = 0;
        for (x = 0; x < 16; ++x) {
            const int sourceX = flipX ? width - 1 - (cropX + x)
                                      : cropX + x;
            const UBYTE mask = (UBYTE)(0x80u >> (sourceX & 7));
            if (row[sourceX >> 3] & mask)
                plane0 |= (UWORD)(0x8000u >> x);
            if (row[planeBytes + (sourceX >> 3)] & mask)
                plane1 |= (UWORD)(0x8000u >> x);
        }
        destination[2 + y * 2] = plane0;
        destination[3 + y * 2] = plane1;
    }
    destination[2 + height * 2] = 0;
    destination[3 + height * 2] = 0;
    return 1;
}
