#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/dmabits.h>
#include "AmigaRenderer.h"

struct ExecBase* SysBase = 0;
struct GfxBase* GfxBase = 0;

extern const UBYTE amiga_hud_pack[];
extern const UBYTE amiga_hud_pack_end[];
extern const UBYTE amiga_game_pack[];
extern const UBYTE amiga_game_pack_end[];

enum {
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

static void drawAsset(AmigaRenderer* renderer, UWORD sprite, UWORD frame,
                      int anchorX, int anchorY, int flipX) {
    amigaRendererDrawAsset(renderer, amiga_hud_pack, amiga_hud_pack_end,
                           sprite, frame, anchorX, anchorY, flipX);
}

static void drawGameAsset(AmigaRenderer* renderer, UWORD sprite, UWORD frame,
                          int anchorX, int anchorY, int flipX) {
    amigaRendererDrawAsset(renderer, amiga_game_pack, amiga_game_pack_end,
                           sprite, frame, anchorX, anchorY, flipX);
}

static void drawHealthNumber(AmigaRenderer* renderer, int x, int y,
                             unsigned health, unsigned maximum) {
    const int slashX = x - 4;
    const int digitY = y - 5;
    const int healthX = slashX - 9;
    const int maximumX = slashX + 9;
    drawAsset(renderer, 413, health + 20, healthX + 1, digitY, 0);
    drawAsset(renderer, 413, health + 20, healthX + 1, digitY + 1, 0);
    drawAsset(renderer, 413, health + 20, healthX, digitY + 1, 0);
    drawAsset(renderer, 413, health, healthX, digitY, 0);
    drawAsset(renderer, 414, 2, slashX + 1, digitY, 0);
    drawAsset(renderer, 414, 2, slashX + 1, digitY + 1, 0);
    drawAsset(renderer, 414, 2, slashX, digitY + 1, 0);
    drawAsset(renderer, 414, 0, slashX, digitY, 0);
    drawAsset(renderer, 413, maximum + 20, maximumX + 1, digitY, 0);
    drawAsset(renderer, 413, maximum + 20, maximumX + 1, digitY + 1, 0);
    drawAsset(renderer, 413, maximum + 20, maximumX, digitY + 1, 0);
    drawAsset(renderer, 413, maximum, maximumX, digitY, 0);
}

static void drawCurrencyDigit(AmigaRenderer* renderer, int x, int y,
                              unsigned digit) {
    const int digitX = x - 8;
    const int digitY = y - 5;
    drawAsset(renderer, 413, digit + 10, digitX - 1, digitY, 0);
    drawAsset(renderer, 413, digit + 10, digitX, digitY - 1, 0);
    drawAsset(renderer, 413, digit + 20, digitX + 1, digitY, 0);
    drawAsset(renderer, 413, digit + 20, digitX, digitY + 1, 0);
    drawAsset(renderer, 413, digit, digitX, digitY, 0);
}

static void drawAmmoDigit(AmigaRenderer* renderer, int x, int y,
                          unsigned digit) {
    static const BYTE offsets[14][2] = {
        {-1, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 1}, {0, 0}, {1, 1},
        {0, 2}, {0, 0}, {1, -1}, {2, 0}, {1, 1}, {0, 1}, {1, 0}
    };
    unsigned index;
    for (index = 0; index < 12; ++index)
        drawAsset(renderer, 413, digit + 20,
                  x + offsets[index][0], y + offsets[index][1], 0);
    for (; index < 14; ++index)
        drawAsset(renderer, 413, digit + 10,
                  x + offsets[index][0], y + offsets[index][1], 0);
    drawAsset(renderer, 413, digit, x, y, 0);
}

static void drawHorizontalTiles(AmigaRenderer* renderer,
                                int firstX, int y, int count) {
    int index;
    for (index = 0; index < count; ++index) {
        unsigned frame = 10;
        if (index == 0) frame = 2;
        if (index == count - 1) frame = 8;
        if (count == 1) frame = 0;
        drawGameAsset(renderer, 51, frame, firstX + index * 16, y, 0);
    }
}

static void buildReferencePicture(AmigaRenderer* renderer) {

    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 8, 228, 10);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 56, 180, 4);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 8, 116, 3);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 120, 100, 3);
    drawGameAsset(renderer, 31, 0, downwellPlayfieldX + 80, 200, 0);

    amigaRendererClearClipRect(renderer);
    drawAsset(renderer, 408, 0, 108, 0, 0);
    drawAsset(renderer, 408, 0, 272, 0, 1);

    drawAsset(renderer, 415, 1, 8, 6, 0);
    amigaRendererFillRect(renderer, 12, 11, 80, 6, 2);
    drawAsset(renderer, 415, 0, 8, 6, 0);
    drawHealthNumber(renderer, 52, 15, 4, 4);

    drawAsset(renderer, 416, 0, 284, 43, 0);
    amigaRendererFillRect(renderer, 289, 49, 10, 184, 1);
    drawAsset(renderer, 416, 1, 284, 43, 0);
    drawAsset(renderer, 376, 0, 362, 18, 0);
    drawCurrencyDigit(renderer, 354, 18, 0);
    drawAmmoDigit(renderer, 290, 239, 8);
}

static unsigned buildCopper(UWORD* list, UBYTE* game, UBYTE* hud,
                            const UWORD* nullSprite) {
    unsigned count = 0;
    unsigned channel;

    copperMove(list, &count, regBplcon0, 0x4401);
    copperMove(list, &count, regBpl1mod, downwellBytesPerPlaneLine);
    copperMove(list, &count, regBpl2mod, downwellBytesPerPlaneLine);
    copperPointer(list, &count, regBpl1pth, game);
    copperPointer(list, &count, regBpl1pth + 4, hud);
    copperPointer(list, &count, regBpl1pth + 8,
                  game + downwellBytesPerPlaneLine);
    copperPointer(list, &count, regBpl1pth + 12,
                  hud + downwellBytesPerPlaneLine);

    for (channel = 0; channel < 8; ++channel)
        copperPointer(list, &count, regSpr0pth + channel * 4, nullSprite);

    copperMove(list, &count, regColor00 + 0 * 2, 0x000);
    copperMove(list, &count, regColor00 + 1 * 2, 0xfff);
    copperMove(list, &count, regColor00 + 2 * 2, 0xf00);
    copperMove(list, &count, regColor00 + 3 * 2, 0x000);
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
    AmigaRenderer renderer;

    SysBase = *(struct ExecBase**)4UL;
    GfxBase = (struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library", 33);
    if (!GfxBase)
        return 1;

    game = (UBYTE*)AllocMem(downwellLayerBytes, MEMF_CHIP | MEMF_CLEAR);
    hud = (UBYTE*)AllocMem(downwellLayerBytes, MEMF_CHIP | MEMF_CLEAR);
    copper = (UWORD*)AllocMem(copperWords * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    nullSprite = (UWORD*)AllocMem(8, MEMF_CHIP | MEMF_CLEAR);
    if (!game || !hud || !copper || !nullSprite) {
        if (nullSprite) FreeMem(nullSprite, 8);
        if (copper) FreeMem(copper, copperWords * sizeof(UWORD));
        if (hud) FreeMem(hud, downwellLayerBytes);
        if (game) FreeMem(game, downwellLayerBytes);
        CloseLibrary((struct Library*)GfxBase);
        return 1;
    }

    amigaRendererInit(&renderer, game, hud);
    buildReferencePicture(&renderer);
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
    FreeMem(hud, downwellLayerBytes);
    FreeMem(game, downwellLayerBytes);
    CloseLibrary((struct Library*)GfxBase);
    GfxBase = 0;
    return 0;
}
