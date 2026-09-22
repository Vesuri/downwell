#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "AmigaRenderer.h"

struct ExecBase* SysBase = 0;
struct GfxBase* GfxBase = 0;

_Static_assert(__builtin_offsetof(struct ExecBase, IntVects) == 84,
               "ExecBase IntVects layout must match the VERTB takeover");

extern const UBYTE amiga_hud_pack[];
extern const UBYTE amiga_hud_pack_end[];
extern const UBYTE amiga_game_pack[];
extern const UBYTE amiga_game_pack_end[];

enum {
    copperWords = 128,
    playerSpriteHeight = 32,
    playerSpriteBytes = (playerSpriteHeight + 2) * 4,
    fetchCropBytes = 2,
    fetchWords = 22,
    fetchWidth = fetchWords * 16,
    displayHStart = downwellDisplayHStart + fetchCropBytes * 8,
    displayHStop = displayHStart + fetchWidth,
    displayVStart = downwellDisplayVStart,
    displayVStop = displayVStart + downwellDisplayHeight,
    fetchStart = 0x30,
    fetchStop = fetchStart + 8 * (fetchWords - 1),
    fetchModulo = downwellBytesPerPlaneLine - fetchWords * 2
};

_Static_assert(displayHStart == 0x71, "cropped display starts at logical x=16");
_Static_assert(displayHStop == 0x1d1, "352-pixel cropped display window");
_Static_assert(displayVStop == 0x12e, "284-line display window");
_Static_assert(fetchWords == 22, "352-pixel DMA fetch");
_Static_assert(fetchStop == 0xd8, "22-word lores fetch at hardware limit");
_Static_assert(fetchModulo == 4, "skip two bytes on each side per row");
_Static_assert(fetchStart == (displayHStart >> 1) - 8,
               "lores fetch/display alignment");

static volatile UWORD* const custom = (volatile UWORD*)0xdff000;
static volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;
static volatile UWORD vblankCount;
static struct Interrupt vblankServer;
static struct IntVector savedVblankVector;

enum {
    regDmaconr = 0x002,
    regIntenar = 0x01c,
    regCop1lc = 0x080,
    regDiwstrt = 0x08e,
    regDiwstop = 0x090,
    regDdfstrt = 0x092,
    regDdfstop = 0x094,
    regDmacon = 0x096,
    regIntena = 0x09a,
    regIntreq = 0x09c,
    regCopjmp1 = 0x088,
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

static ULONG vblankHandler(void) {
    /* A raw IntVector owns acknowledgement; exec's server walker is absent. */
    writeRegister(regIntreq, INTF_VERTB);
    writeRegister(regIntreq, INTF_VERTB);
    ++vblankCount;
    return 0;
}

static void waitVblank(void) {
    const UWORD start = vblankCount;
    while (vblankCount == start) { }
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

static void buildReferencePicture(AmigaRenderer* renderer, unsigned phase) {

    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 8, 228, 10);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 56, 180, 4);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 8, 116, 3);
    drawHorizontalTiles(renderer, downwellPlayfieldX + 120, 100, 3);
    (void)phase;

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
                            const UWORD* playerSprite,
                            const UWORD* nullSprite) {
    unsigned count = 0;
    unsigned channel;

    copperMove(list, &count, regBplcon0, 0x4401);
    copperMove(list, &count, regBpl1mod, fetchModulo);
    copperMove(list, &count, regBpl2mod, fetchModulo);
    copperPointer(list, &count, regBpl1pth, game + fetchCropBytes);
    copperPointer(list, &count, regBpl1pth + 4, hud + fetchCropBytes);
    copperPointer(list, &count, regBpl1pth + 8,
                  game + downwellPlaneBytes + fetchCropBytes);
    copperPointer(list, &count, regBpl1pth + 12,
                  hud + downwellPlaneBytes + fetchCropBytes);

    copperPointer(list, &count, regSpr0pth, playerSprite);
    for (channel = 1; channel < 8; ++channel)
        copperPointer(list, &count, regSpr0pth + channel * 4, nullSprite);

    copperMove(list, &count, regColor00 + 0 * 2, 0x000);
    copperMove(list, &count, regColor00 + 1 * 2, 0xfff);
    copperMove(list, &count, regColor00 + 2 * 2, 0xf00);
    copperMove(list, &count, regColor00 + 3 * 2, 0x000);
    copperMove(list, &count, regColor00 + 9 * 2, 0xfff);
    copperMove(list, &count, regColor00 + 10 * 2, 0xf00);
    copperMove(list, &count, regColor00 + 11 * 2, 0x000);
    copperMove(list, &count, regColor00 + 17 * 2, 0xfff);
    copperMove(list, &count, regColor00 + 18 * 2, 0xf00);
    copperMove(list, &count, regColor00 + 19 * 2, 0x000);

    list[count++] = 0xffff;
    list[count++] = 0xfffe;
    return count;
}

static void setDisplayRegisters(void) {
    writeRegister(regDiwstrt,
                  (UWORD)((displayVStart << 8) | (displayHStart & 0xff)));
    writeRegister(regDiwstop,
                  (UWORD)(((displayVStop & 0xff) << 8) |
                          (displayHStop & 0xff)));
    writeRegister(regDdfstrt, fetchStart);
    writeRegister(regDdfstop, fetchStop);
    writeRegister(regFmode, 0x0000);
    writeRegister(regBplcon1, 0x0000);
    writeRegister(regBplcon2, 0x0064);
    writeRegister(regBplcon3, 0x0c30);
}

static void setCopperList(const UWORD* list, int immediate) {
    custom[regCop1lc >> 1] = (UWORD)((ULONG)list >> 16);
    custom[(regCop1lc + 2) >> 1] = (UWORD)(ULONG)list;
    if (immediate)
        writeRegister(regCopjmp1, 0);
}

int main(void) {
    struct View* savedView;
    UBYTE* game[2];
    UBYTE* hud;
    UWORD* copper;
    UWORD* nullSprite;
    UWORD* playerSprite[2];
    AmigaRenderer renderer;
    unsigned frame = 0;
    unsigned front = 0;
    UWORD savedDmacon;
    UWORD savedIntena;
    const UWORD* osCopper;

    SysBase = *(struct ExecBase**)4UL;
    GfxBase = (struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library", 33);
    if (!GfxBase)
        return 1;

    game[0] = (UBYTE*)AllocMem(downwellLayerBytes, MEMF_CHIP | MEMF_CLEAR);
    game[1] = (UBYTE*)AllocMem(downwellLayerBytes, MEMF_CHIP | MEMF_CLEAR);
    hud = (UBYTE*)AllocMem(downwellLayerBytes, MEMF_CHIP | MEMF_CLEAR);
    copper = (UWORD*)AllocMem(copperWords * 2 * sizeof(UWORD),
                             MEMF_CHIP | MEMF_CLEAR);
    nullSprite = (UWORD*)AllocMem(8, MEMF_CHIP | MEMF_CLEAR);
    playerSprite[0] = (UWORD*)AllocMem(playerSpriteBytes,
                                      MEMF_CHIP | MEMF_CLEAR);
    playerSprite[1] = (UWORD*)AllocMem(playerSpriteBytes,
                                      MEMF_CHIP | MEMF_CLEAR);
    if (!game[0] || !game[1] || !hud || !copper || !nullSprite ||
        !playerSprite[0] || !playerSprite[1]) {
        if (playerSprite[1]) FreeMem(playerSprite[1], playerSpriteBytes);
        if (playerSprite[0]) FreeMem(playerSprite[0], playerSpriteBytes);
        if (nullSprite) FreeMem(nullSprite, 8);
        if (copper) FreeMem(copper, copperWords * 2 * sizeof(UWORD));
        if (hud) FreeMem(hud, downwellLayerBytes);
        if (game[1]) FreeMem(game[1], downwellLayerBytes);
        if (game[0]) FreeMem(game[0], downwellLayerBytes);
        CloseLibrary((struct Library*)GfxBase);
        return 1;
    }

    amigaRendererInit(&renderer, game[0], hud);
    buildReferencePicture(&renderer, 0);
    amigaRendererInit(&renderer, game[1], hud);
    buildReferencePicture(&renderer, 1);
    if (!amigaRendererBuildHardwareSprite(
            amiga_game_pack, amiga_game_pack_end, 31, 0,
            downwellPlayfieldX + 80, 200,
            playerSprite[0], playerSpriteHeight) ||
        !amigaRendererBuildHardwareSprite(
            amiga_game_pack, amiga_game_pack_end, 37, 0,
            downwellPlayfieldX + 88, 192,
            playerSprite[1], playerSpriteHeight)) {
        FreeMem(playerSprite[1], playerSpriteBytes);
        FreeMem(playerSprite[0], playerSpriteBytes);
        FreeMem(nullSprite, 8);
        FreeMem(copper, copperWords * 2 * sizeof(UWORD));
        FreeMem(hud, downwellLayerBytes);
        FreeMem(game[1], downwellLayerBytes);
        FreeMem(game[0], downwellLayerBytes);
        CloseLibrary((struct Library*)GfxBase);
        return 1;
    }
    buildCopper(copper, game[0], hud, playerSprite[0], nullSprite);
    buildCopper(copper + copperWords, game[1], hud,
                playerSprite[1], nullSprite);

    /*
     * Follow the Rescue on Fractalus takeover boundary: all allocations and
     * construction happen while the OS owns the machine. WaitTOF is used only
     * to retire its display before the raw VERTB vector is installed.
     */
    savedView = GfxBase->ActiView;
    osCopper = (const UWORD*)GfxBase->copinit;
    savedDmacon = custom[regDmaconr >> 1];
    savedIntena = custom[regIntenar >> 1];
    LoadView(0);
    WaitTOF();
    WaitTOF();
    writeRegister(regDmacon, DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER);
    writeRegister(regBplcon0, 0x0000);
    setDisplayRegisters();

    /* Nothing in this program consumes blit-done interrupts. */
    writeRegister(regIntena, INTF_BLIT);
    writeRegister(regIntreq, INTF_BLIT);
    writeRegister(regIntreq, INTF_BLIT);

    vblankServer.is_Node.ln_Type = NT_INTERRUPT;
    vblankServer.is_Node.ln_Pri = 127;
    vblankServer.is_Node.ln_Name = (char*)"Downwell VBI";
    vblankServer.is_Data = 0;
    vblankServer.is_Code = (void (*)())vblankHandler;
    Disable();
    savedVblankVector = SysBase->IntVects[INTB_VERTB];
    SysBase->IntVects[INTB_VERTB].iv_Data = 0;
    SysBase->IntVects[INTB_VERTB].iv_Code = (void (*)())vblankHandler;
    SysBase->IntVects[INTB_VERTB].iv_Node = &vblankServer.is_Node;
    Enable();

    vblankCount = 0;
    setCopperList(copper, 0);
    writeRegister(regIntena, INTF_SETCLR | INTF_INTEN | INTF_VERTB);
    writeRegister(regDmacon, DMAF_SETCLR | DMAF_MASTER | DMAF_COPPER |
                                DMAF_RASTER | DMAF_SPRITE);

    /* No scheduling or OS calls are permitted inside the takeover window. */
    Forbid();
    while ((*ciaaPra & 0x40u) == 0) waitVblank();
    while ((*ciaaPra & 0x40u) != 0) {
        waitVblank();
        ++frame;
        if ((frame & 15u) == 0) {
            front ^= 1u;
            /* COP1LC is sampled at the next VERTB; never restart mid-frame. */
            setCopperList(copper + front * copperWords, 0);
        }
    }

    /* Stop our interrupt and display before handing either back to exec. */
    writeRegister(regIntena, INTF_VERTB);
    writeRegister(regIntreq, INTF_VERTB);
    writeRegister(regIntreq, INTF_VERTB);
    writeRegister(regDmacon, DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER);
    writeRegister(regBplcon0, 0x0000);

    /* Restore graphics.library's startup copper, not merely its View list. */
    setCopperList(osCopper, 1);
    writeRegister(regBplcon3, 0x0c00);

    Disable();
    SysBase->IntVects[INTB_VERTB] = savedVblankVector;
    Enable();

    /* Restore the complete DMA and interrupt enable sets captured above. */
    writeRegister(regDmacon, DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG);
    writeRegister(regDmacon,
                  DMAF_SETCLR | (savedDmacon &
                  (DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG)));
    writeRegister(regIntena, 0x7fffu);
    writeRegister(regIntreq, INTF_BLIT | INTF_VERTB);
    writeRegister(regIntreq, INTF_BLIT | INTF_VERTB);
    writeRegister(regIntena, INTF_SETCLR | (savedIntena & 0x7fffu));

    Permit();
    LoadView(savedView);
    WaitTOF();
    WaitTOF();

    FreeMem(playerSprite[1], playerSpriteBytes);
    FreeMem(playerSprite[0], playerSpriteBytes);
    FreeMem(nullSprite, 8);
    FreeMem(copper, copperWords * 2 * sizeof(UWORD));
    FreeMem(hud, downwellLayerBytes);
    FreeMem(game[1], downwellLayerBytes);
    FreeMem(game[0], downwellLayerBytes);
    CloseLibrary((struct Library*)GfxBase);
    GfxBase = 0;
    return 0;
}
