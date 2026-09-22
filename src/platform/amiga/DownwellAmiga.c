#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/dmabits.h>

struct ExecBase* SysBase = 0;
struct GfxBase* GfxBase = 0;

extern const UBYTE amiga_hud_pack[];
extern const UBYTE amiga_hud_pack_end[];

enum {
    visibleWidth = 380,
    dmaWidth = 384,
    displayHeight = 284,
    bytesPerPlaneLine = dmaWidth / 8,
    bytesPerLayerLine = bytesPerPlaneLine * 2,
    layerBytes = bytesPerLayerLine * displayHeight,
    backingBias = 2,
    wellX = backingBias + 110,
    wellWidth = 160,
    copperWords = 128
};

static volatile UWORD* const custom = (volatile UWORD*)0xdff000;
static volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;

enum {
    regCop1lc = 0x080,
    regDiwstrt = 0x08e,
    regDiwstop = 0x090,
    regDdfstrt = 0x092,
    regDdfstop = 0x094,
    regDmacon = 0x096,
    regBpl1pth = 0x0e0,
    regBplcon0 = 0x100,
    regBplcon1 = 0x102,
    regBplcon2 = 0x104,
    regBplcon3 = 0x106,
    regBpl1mod = 0x108,
    regBpl2mod = 0x10a,
    regSpr0pth = 0x120,
    regColor00 = 0x180,
    regFmode = 0x1fc
};

static void writeRegister(UWORD offset, UWORD value) {
    custom[offset >> 1] = value;
}

static void copperMove(UWORD* list, unsigned* count, UWORD reg, UWORD value) {
    list[(*count)++] = reg;
    list[(*count)++] = value;
}

static void copperPointer(UWORD* list, unsigned* count, UWORD reg, const void* ptr) {
    const ULONG address = (ULONG)ptr;
    copperMove(list, count, reg, (UWORD)(address >> 16));
    copperMove(list, count, reg + 2, (UWORD)address);
}

static void setPixel(UBYTE* layer, int x, int y, unsigned pen) {
    UBYTE* row;
    UBYTE mask;
    if (x < 0 || x >= dmaWidth || y < 0 || y >= displayHeight)
        return;
    row = layer + y * bytesPerLayerLine;
    mask = (UBYTE)(0x80u >> (x & 7));
    row[x >> 3] &= (UBYTE)~mask;
    row[bytesPerPlaneLine + (x >> 3)] &= (UBYTE)~mask;
    if (pen & 1u) row[x >> 3] |= mask;
    if (pen & 2u) row[bytesPerPlaneLine + (x >> 3)] |= mask;
}

static UWORD readWord(const UBYTE* data) {
    return (UWORD)(((UWORD)data[0] << 8) | data[1]);
}

static ULONG readLong(const UBYTE* data) {
    return ((ULONG)readWord(data) << 16) | readWord(data + 2);
}

static const UBYTE* findAsset(UWORD sprite, UWORD frame) {
    const UBYTE* record;
    ULONG count;
    ULONG index;
    if (readLong(amiga_hud_pack) != 0x44574150UL ||
        readWord(amiga_hud_pack + 4) != 1)
        return 0;
    count = readLong(amiga_hud_pack + 8);
    record = amiga_hud_pack + readWord(amiga_hud_pack + 6);
    for (index = 0; index < count; ++index, record += 24)
        if (readWord(record) == sprite && readWord(record + 2) == frame)
            return record;
    return 0;
}

static void drawAsset(UBYTE* layer, UWORD sprite, UWORD frame,
                      int anchorX, int anchorY, int flipX) {
    const UBYTE* record = findAsset(sprite, frame);
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
    data = amiga_hud_pack + readLong(record + 16);
    if (data + readLong(record + 20) > amiga_hud_pack_end)
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
                setPixel(layer, backingBias + drawX,
                         anchorY - originY + y, pen);
            }
        }
    }
}

static void drawHealthNumber(UBYTE* hud, int x, int y,
                             unsigned health, unsigned maximum) {
    const int slashX = x - 4;
    const int digitY = y - 5;
    const int healthX = slashX - 9;
    const int maximumX = slashX + 9;
    drawAsset(hud, 413, health + 20, healthX + 1, digitY, 0);
    drawAsset(hud, 413, health + 20, healthX + 1, digitY + 1, 0);
    drawAsset(hud, 413, health + 20, healthX, digitY + 1, 0);
    drawAsset(hud, 413, health, healthX, digitY, 0);
    drawAsset(hud, 414, 2, slashX + 1, digitY, 0);
    drawAsset(hud, 414, 2, slashX + 1, digitY + 1, 0);
    drawAsset(hud, 414, 2, slashX, digitY + 1, 0);
    drawAsset(hud, 414, 0, slashX, digitY, 0);
    drawAsset(hud, 413, maximum + 20, maximumX + 1, digitY, 0);
    drawAsset(hud, 413, maximum + 20, maximumX + 1, digitY + 1, 0);
    drawAsset(hud, 413, maximum + 20, maximumX, digitY + 1, 0);
    drawAsset(hud, 413, maximum, maximumX, digitY, 0);
}

static void drawCurrencyDigit(UBYTE* hud, int x, int y, unsigned digit) {
    const int digitX = x - 8;
    const int digitY = y - 5;
    drawAsset(hud, 413, digit + 10, digitX - 1, digitY, 0);
    drawAsset(hud, 413, digit + 10, digitX, digitY - 1, 0);
    drawAsset(hud, 413, digit + 20, digitX + 1, digitY, 0);
    drawAsset(hud, 413, digit + 20, digitX, digitY + 1, 0);
    drawAsset(hud, 413, digit, digitX, digitY, 0);
}

static void drawAmmoDigit(UBYTE* hud, int x, int y, unsigned digit) {
    static const BYTE offsets[14][2] = {
        {-1, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 1}, {0, 0}, {1, 1},
        {0, 2}, {0, 0}, {1, -1}, {2, 0}, {1, 1}, {0, 1}, {1, 0}
    };
    unsigned index;
    for (index = 0; index < 12; ++index)
        drawAsset(hud, 413, digit + 20,
                  x + offsets[index][0], y + offsets[index][1], 0);
    for (; index < 14; ++index)
        drawAsset(hud, 413, digit + 10,
                  x + offsets[index][0], y + offsets[index][1], 0);
    drawAsset(hud, 413, digit, x, y, 0);
}

static void fillRect(UBYTE* layer, int x, int y, int width, int height,
                     unsigned pen) {
    int py;
    int px;
    for (py = y; py < y + height; ++py)
        for (px = x; px < x + width; ++px)
            setPixel(layer, px, py, pen);
}

static void buildDiagnosticPicture(UBYTE* game, UBYTE* hud) {
    int y;
    int x;

    fillRect(game, wellX, 0, wellWidth, displayHeight, 1);
    fillRect(game, wellX + 4, 4, wellWidth - 8, displayHeight - 8, 0);
    fillRect(game, wellX + 24, 52, wellWidth - 48, 40, 2);
    for (y = 116; y < 244; y += 16)
        for (x = wellX + 8; x < wellX + wellWidth - 8; x += 16)
            fillRect(game, x, y, 8, 8, ((x + y) >> 3) & 1 ? 1 : 2);

    drawAsset(hud, 408, 0, 108, 0, 0);
    drawAsset(hud, 408, 0, 272, 0, 1);

    drawAsset(hud, 415, 1, 8, 6, 0);
    fillRect(hud, backingBias + 12, 11, 80, 6, 2);
    drawAsset(hud, 415, 0, 8, 6, 0);
    drawHealthNumber(hud, 52, 15, 4, 4);

    drawAsset(hud, 416, 0, 284, 43, 0);
    fillRect(hud, backingBias + 289, 49, 10, 184, 1);
    drawAsset(hud, 416, 1, 284, 43, 0);
    drawAsset(hud, 376, 0, 362, 18, 0);
    drawCurrencyDigit(hud, 354, 18, 0);
    drawAmmoDigit(hud, 290, 239, 8);
}

static unsigned buildCopper(UWORD* list, UBYTE* game, UBYTE* hud,
                            const UWORD* nullSprite) {
    unsigned count = 0;
    unsigned channel;

    copperMove(list, &count, regBplcon0, 0x4401);
    copperMove(list, &count, regBpl1mod, bytesPerPlaneLine);
    copperMove(list, &count, regBpl2mod, bytesPerPlaneLine);
    copperPointer(list, &count, regBpl1pth, game);
    copperPointer(list, &count, regBpl1pth + 4, hud);
    copperPointer(list, &count, regBpl1pth + 8, game + bytesPerPlaneLine);
    copperPointer(list, &count, regBpl1pth + 12, hud + bytesPerPlaneLine);

    for (channel = 0; channel < 8; ++channel)
        copperPointer(list, &count, regSpr0pth + channel * 4, nullSprite);

    copperMove(list, &count, regColor00 + 0 * 2, 0x000);
    copperMove(list, &count, regColor00 + 1 * 2, 0xfff);
    copperMove(list, &count, regColor00 + 2 * 2, 0xf00);
    copperMove(list, &count, regColor00 + 3 * 2, 0x888);
    copperMove(list, &count, regColor00 + 9 * 2, 0xfff);
    copperMove(list, &count, regColor00 + 10 * 2, 0xf00);
    copperMove(list, &count, regColor00 + 11 * 2, 0x000);

    list[count++] = 0xffff;
    list[count++] = 0xfffe;
    return count;
}

static void setDisplayRegisters(void) {
    writeRegister(regDiwstrt, 0x1263);
    writeRegister(regDiwstop, 0x2edf);
    writeRegister(regDdfstrt, 0x0028);
    writeRegister(regDdfstop, 0x00e0);
    writeRegister(regFmode, 0x0000);
    writeRegister(regBplcon1, 0x0000);
    writeRegister(regBplcon2, 0x0064);
    writeRegister(regBplcon3, 0x0c30);
}

int main(void) {
    struct View* savedView;
    UBYTE* game;
    UBYTE* hud;
    UWORD* copper;
    UWORD* nullSprite;

    SysBase = *(struct ExecBase**)4UL;
    GfxBase = (struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library", 33);
    if (!GfxBase)
        return 1;

    game = (UBYTE*)AllocMem(layerBytes, MEMF_CHIP | MEMF_CLEAR);
    hud = (UBYTE*)AllocMem(layerBytes, MEMF_CHIP | MEMF_CLEAR);
    copper = (UWORD*)AllocMem(copperWords * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    nullSprite = (UWORD*)AllocMem(8, MEMF_CHIP | MEMF_CLEAR);
    if (!game || !hud || !copper || !nullSprite) {
        if (nullSprite) FreeMem(nullSprite, 8);
        if (copper) FreeMem(copper, copperWords * sizeof(UWORD));
        if (hud) FreeMem(hud, layerBytes);
        if (game) FreeMem(game, layerBytes);
        CloseLibrary((struct Library*)GfxBase);
        return 1;
    }

    buildDiagnosticPicture(game, hud);
    buildCopper(copper, game, hud, nullSprite);

    savedView = GfxBase->ActiView;
    LoadView(0);
    WaitTOF();
    WaitTOF();
    writeRegister(regDmacon, DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER);
    writeRegister(regBplcon0, 0x0000);
    setDisplayRegisters();
    custom[regCop1lc >> 1] = (UWORD)((ULONG)copper >> 16);
    custom[(regCop1lc + 2) >> 1] = (UWORD)(ULONG)copper;
    writeRegister(regDmacon, DMAF_SETCLR | DMAF_MASTER | DMAF_COPPER |
                                DMAF_RASTER | DMAF_SPRITE);

    while ((*ciaaPra & 0x40u) == 0) WaitTOF();
    while ((*ciaaPra & 0x40u) != 0) WaitTOF();

    writeRegister(regDmacon, DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER);
    writeRegister(regBplcon0, 0x0000);
    LoadView(savedView);
    WaitTOF();
    WaitTOF();

    FreeMem(nullSprite, 8);
    FreeMem(copper, copperWords * sizeof(UWORD));
    FreeMem(hud, layerBytes);
    FreeMem(game, layerBytes);
    CloseLibrary((struct Library*)GfxBase);
    GfxBase = 0;
    return 0;
}
