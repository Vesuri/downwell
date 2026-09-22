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
    if (x < renderer->clipX || y < renderer->clipY ||
        x >= renderer->clipX + renderer->clipWidth ||
        y >= renderer->clipY + renderer->clipHeight ||
        x < 0 || x >= downwellVisibleWidth ||
        y < 0 || y >= downwellDisplayHeight)
        return;
    x += downwellBackingBias;
    row = renderer->target + y * downwellBytesPerLayerLine;
    mask = (UBYTE)(0x80u >> (x & 7));
    row[x >> 3] &= (UBYTE)~mask;
    row[downwellBytesPerPlaneLine + (x >> 3)] &= (UBYTE)~mask;
    if (pen & 1u) row[x >> 3] |= mask;
    if (pen & 2u) row[downwellBytesPerPlaneLine + (x >> 3)] |= mask;
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
    amigaRendererClearClipRect(renderer);
}

void amigaRendererClear(AmigaRenderer* renderer, unsigned pen) {
    UBYTE* savedTarget = renderer->target;
    int savedX = renderer->clipX;
    int savedY = renderer->clipY;
    int savedWidth = renderer->clipWidth;
    int savedHeight = renderer->clipHeight;
    renderer->clipX = 0;
    renderer->clipY = 0;
    renderer->clipWidth = downwellVisibleWidth;
    renderer->clipHeight = downwellDisplayHeight;
    renderer->target = renderer->game;
    amigaRendererFillRect(renderer, 0, 0, downwellVisibleWidth,
                          downwellDisplayHeight, pen);
    renderer->target = renderer->hud;
    amigaRendererFillRect(renderer, 0, 0, downwellVisibleWidth,
                          downwellDisplayHeight, 0);
    renderer->target = savedTarget;
    renderer->clipX = savedX;
    renderer->clipY = savedY;
    renderer->clipWidth = savedWidth;
    renderer->clipHeight = savedHeight;
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
    int py;
    int px;
    for (py = y; py < y + height; ++py)
        for (px = x; px < x + width; ++px)
            setPixel(renderer, px, py, pen);
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
