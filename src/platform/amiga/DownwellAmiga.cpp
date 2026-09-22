#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "AmigaInput.h"
#include "AmigaPlayer.h"
#include "AmigaPortRuntime.h"
#include "AmigaRenderer.h"
#include "framework/AmigaHardware.h"
#include "framework/AmigaMemory.h"
#include "framework/CopperList.h"
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/PortCheckpoint.h"
#include "core/PortReplay.h"
#include "core/RoomPack.h"
#include "core/Replay.h"

struct ExecBase* SysBase = 0;
struct GfxBase* GfxBase = 0;

static_assert(__builtin_offsetof(struct ExecBase, IntVects) == 84,
              "ExecBase IntVects layout must match the VERTB takeover");

extern "C" {
extern const UBYTE amiga_hud_pack[];
extern const UBYTE amiga_hud_pack_end[];
extern const UBYTE amiga_game_pack[];
extern const UBYTE amiga_game_pack_end[];
extern const UBYTE amiga_room_pack[];
extern const UBYTE amiga_room_pack_end[];
extern const UBYTE amiga_object_pack[];
extern const UBYTE amiga_object_pack_end[];
extern const UBYTE amiga_builder_pack[];
extern const UBYTE amiga_builder_pack_end[];
}

enum {
    copperWords = 128,
    playerSpriteHeight = 32,
    playerSpriteBytes = (playerSpriteHeight + 2) * 4,
    bulletSpriteHeight = 8,
    bulletSpriteBytes = (bulletSpriteHeight + 2) * 4,
    fetchCropBytes = 2,
    fetchWords = 22,
    fetchWidth = fetchWords * 16,
    displayHStart = downwellDisplayHStart + fetchCropBytes * 8,
    displayHStop = displayHStart + fetchWidth,
    displayVStart = downwellDisplayVStart,
    displayVStop = displayVStart + downwellDisplayHeight,
    fetchStart = 0x30,
    fetchStop = fetchStart + 8 * (fetchWords - 1),
    fetchModulo = downwellBytesPerPlaneLine - fetchWords * 2,
    initialRingOrigin = 300
};

static_assert(displayHStart == 0x71, "cropped display starts at logical x=16");
static_assert(displayHStop == 0x1d1, "352-pixel cropped display window");
static_assert(displayVStop == 0x12e, "284-line display window");
static_assert(fetchWords == 22, "352-pixel DMA fetch");
static_assert(fetchStop == 0xd8, "22-word lores fetch at hardware limit");
static_assert(fetchModulo == 4, "skip two bytes on each side per row");
static_assert(fetchStart == (displayHStart >> 1) - 8,
              "lores fetch/display alignment");

static volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;
static volatile UWORD vblankCount;
static struct Interrupt vblankServer;
static struct IntVector savedVblankVector;
static volatile unsigned long long lastPortCheckpoint;

#ifdef DOWNWELL_REPLAY_VERIFY
extern "C" {
volatile ULONG gReplayComplete;
volatile ULONG gReplayFrames;
volatile ULONG gReplayDigestHigh;
volatile ULONG gReplayDigestLow;

__attribute__((noinline, used)) void downwellReplayComplete(void) {
    __asm__ volatile("" ::: "memory");
}
}
#endif

static ULONG vblankHandler(void) {
    /* A raw IntVector owns acknowledgement; exec's server walker is absent. */
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    ++vblankCount;
    return 0;
}

static void waitVblank(void) {
    const UWORD start = vblankCount;
    while (vblankCount == start) { }
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

static void redrawAmmo(AmigaRenderer* renderer, unsigned ammo) {
    amigaRendererClearClipRect(renderer);
    amigaRendererFillRect(renderer, 282, 232, 20, 20, 0);
    drawAmmoDigit(renderer, 290, 239, ammo);
}

static bool isTerrainSymbol(char symbol) {
    return symbol == '1' || symbol == '2' || symbol == '3' ||
           symbol == 'B' || symbol == '-' || symbol == '|';
}

static void drawEntrancePattern(AmigaRenderer* renderer,
                                const downwell::BuilderPatternPack& builders,
                                const downwell::PackedBuilderPattern& pattern) {
    const int baseY = downwellDisplayHeight - pattern.rowCount * 16;
    unsigned row;
    for (row = 0; row < pattern.rowCount; ++row) {
        ULONG width = 0;
        unsigned column;
        if (!builders.rowWidth(pattern, (UWORD)row, width))
            return;
        for (column = 0; column < width; ++column) {
            char symbol = 0;
            if (!builders.cell(pattern, (UWORD)column, (UWORD)row, symbol))
                return;
            if (isTerrainSymbol(symbol))
                drawGameAsset(renderer, 51, 0,
                              downwellPlayfieldX + 8 + column * 16,
                              baseY + row * 16, 0);
            if (symbol == 'd')
                drawGameAsset(renderer, 166, 0,
                              downwellPlayfieldX + 8 + column * 16,
                              baseY + row * 16, 0);
        }
    }
}

static void buildReferencePicture(
    AmigaRenderer* renderer,
    const downwell::BuilderPatternPack& builders,
    const downwell::PackedBuilderPattern& entrance) {

    amigaRendererClear(renderer, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    drawEntrancePattern(renderer, builders, entrance);

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

static void drawRevealedGameLine(AmigaRenderer* renderer,
                                 const downwell::BuilderPatternPack& builders,
                                 const downwell::PackedBuilderPattern& pattern,
                                 unsigned ringOrigin,
                                 unsigned worldY) {
    const int y = downwellDisplayHeight - 1;
    const unsigned patternY = worldY % (pattern.rowCount * 16u);
    const UWORD row = (UWORD)(patternY >> 4);
    const unsigned line = patternY & 15u;
    ULONG width = 0;
    unsigned column;
    amigaRendererSetGameOrigin(renderer, ringOrigin);
    amigaRendererClearGameRows(renderer, y, 1, 0);
    amigaRendererSetClipRect(renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);

    /* The side walls are permanent world geometry, revealed a line at a time. */
    amigaRendererFillRect(renderer, downwellPlayfieldX, y, 2, 1, 1);
    amigaRendererFillRect(renderer,
                          downwellPlayfieldX + downwellPlayfieldWidth - 2,
                          y, 2, 1, 1);

    if (builders.rowWidth(pattern, row, width)) {
        for (column = 0; column < width; ++column) {
            char symbol = 0;
            if (!builders.cell(pattern, (UWORD)column, row, symbol))
                break;
            if (isTerrainSymbol(symbol))
                amigaRendererFillRect(renderer,
                    downwellPlayfieldX + 8 + column * 16,
                    y, 16, 1, 1);
            if (symbol == '_' && line < 2u)
                amigaRendererFillRect(renderer,
                    downwellPlayfieldX + 8 + column * 16,
                    y, 16, 1, 1);
        }
    }
    amigaRendererClearClipRect(renderer);
}

static bool buildCopper(CopperList& list, UBYTE* game, UBYTE* hud,
                        const UWORD* playerSprite,
                        const UWORD* bulletSprite,
                        const UWORD* nullSprite,
                        unsigned ringOrigin) {
    unsigned channel;
    const unsigned firstSpan = downwellGameRingHeight - ringOrigin;
    UBYTE* gameStart = game + ringOrigin * downwellBytesPerPlaneLine;

    list.reset();
    list.move(AmigaHardware::bplcon0, 0x4401);
    list.move(AmigaHardware::bpl1mod, fetchModulo);
    list.move(AmigaHardware::bpl2mod, fetchModulo);
    list.pointer(AmigaHardware::bpl1pth, gameStart + fetchCropBytes);
    list.pointer(AmigaHardware::bpl1pth + 4, hud + fetchCropBytes);
    list.pointer(AmigaHardware::bpl1pth + 8,
                 gameStart + downwellGameRingPlaneBytes + fetchCropBytes);
    list.pointer(AmigaHardware::bpl1pth + 12,
                 hud + downwellPlaneBytes + fetchCropBytes);

    list.pointer(AmigaHardware::spr0pth, playerSprite);
    list.pointer(AmigaHardware::spr0pth + 4, bulletSprite);
    for (channel = 2; channel < 8; ++channel)
        list.pointer((UWORD)(AmigaHardware::spr0pth + channel * 4), nullSprite);

    list.move(AmigaHardware::color00 + 0 * 2, 0x000);
    list.move(AmigaHardware::color00 + 1 * 2, 0xfff);
    list.move(AmigaHardware::color00 + 2 * 2, 0xf00);
    list.move(AmigaHardware::color00 + 3 * 2, 0x000);
    list.move(AmigaHardware::color00 + 9 * 2, 0xfff);
    list.move(AmigaHardware::color00 + 10 * 2, 0xf00);
    list.move(AmigaHardware::color00 + 11 * 2, 0x000);
    list.move(AmigaHardware::color00 + 17 * 2, 0xfff);
    list.move(AmigaHardware::color00 + 18 * 2, 0xf00);
    list.move(AmigaHardware::color00 + 19 * 2, 0x000);

    if (firstSpan < downwellDisplayHeight) {
        const unsigned resetAfterLine =
            displayVStart + firstSpan - 1;
        /*
         * Repoint after the final fetch from the ring tail and before the next
         * line's DDFSTRT. Copper vertical comparison wraps at line 255, so an
         * explicit line-255 barrier is required for splits below that point.
         */
        if (resetAfterLine > 255)
            list.wait(255, 0xdf);
        list.wait((UWORD)resetAfterLine, 0xdf);
        list.pointer(AmigaHardware::bpl1pth, game + fetchCropBytes);
        list.pointer(AmigaHardware::bpl1pth + 8,
                     game + downwellGameRingPlaneBytes + fetchCropBytes);
    }

    return list.finish();
}

static void hideHardwareSprite(UWORD* sprite) {
    sprite[0] = 0;
    sprite[1] = 0;
}

static void buildBulletSprite(UWORD* sprite, int x, int y) {
    UBYTE* control = (UBYTE*)sprite;
    const int startX = downwellDisplayHStart + x;
    const int startY = downwellDisplayVStart + y;
    const int stopY = startY + bulletSpriteHeight;
    unsigned line;
    control[0] = (UBYTE)startY;
    control[1] = (UBYTE)(startX >> 1);
    control[2] = (UBYTE)stopY;
    control[3] = (UBYTE)((startX & 1) |
                         (stopY > 255 ? 2 : 0) |
                         (startY > 255 ? 4 : 0));
    for (line = 0; line < bulletSpriteHeight; ++line) {
        sprite[2 + line * 2] = line < 2 ? 0xc000 : 0;
        sprite[3 + line * 2] = 0xc000;
    }
    sprite[2 + bulletSpriteHeight * 2] = 0;
    sprite[3 + bulletSpriteHeight * 2] = 0;
}

static void offsetHardwareSprite(UWORD* sprite, int deltaX, int deltaY) {
    UBYTE* control = (UBYTE*)sprite;
    int startX = ((int)control[1] << 1) | (control[3] & 1);
    int startY = control[0] | ((control[3] & 4) << 6);
    int stopY = control[2] | ((control[3] & 2) << 7);
    startX += deltaX;
    startY += deltaY;
    stopY += deltaY;
    control[0] = (UBYTE)startY;
    control[1] = (UBYTE)(startX >> 1);
    control[2] = (UBYTE)stopY;
    control[3] = (UBYTE)((startX & 1) |
                         (stopY > 255 ? 2 : 0) |
                         (startY > 255 ? 4 : 0));
}

int main(void) {
    struct View* savedView;
    AmigaMemory memory;
    AmigaRenderer renderer;
    unsigned front = 0;
    unsigned worldBottom;
    AmigaPortRuntime port(initialRingOrigin);
    UWORD savedDmacon;
    UWORD savedIntena;
    const UWORD* osCopper;

    SysBase = *(struct ExecBase**)4UL;
    GfxBase = (struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library", 33);
    if (!GfxBase)
        return 1;
    const downwell::RoomPack rooms(
        amiga_room_pack,
        (unsigned long)(amiga_room_pack_end - amiga_room_pack));
    const downwell::ObjectPack objects(
        amiga_object_pack,
        (unsigned long)(amiga_object_pack_end - amiga_object_pack));
    const downwell::BuilderPatternPack builders(
        amiga_builder_pack,
        (unsigned long)(amiga_builder_pack_end - amiga_builder_pack));
#ifdef DOWNWELL_REPLAY_VERIFY
    const downwell::ReplayReader verificationReplay(
        downwell::portReplayBytes, downwell::portReplaySize);
#endif
    downwell::PackedBuilderPattern entrance;
    if (!rooms.valid() || !objects.valid() || !builders.valid() ||
#ifdef DOWNWELL_REPLAY_VERIFY
        !verificationReplay.valid() ||
#endif
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneStart,
                              0, entrance)) {
        CloseLibrary((struct Library*)GfxBase);
        GfxBase = 0;
        return 1;
    }
    worldBottom = entrance.rowCount * 16u - 1u;

    if (!memory.allocate(downwellGameRingBytes, downwellLayerBytes,
                         copperWords * 2 * sizeof(UWORD), playerSpriteBytes,
                         bulletSpriteBytes)) {
        CloseLibrary((struct Library*)GfxBase);
        GfxBase = 0;
        return 1;
    }

    CopperList copperLists[2] = {
        CopperList(memory.copper(), copperWords),
        CopperList(memory.copper() + copperWords, copperWords)
    };

    amigaRendererInit(&renderer, memory.game(), memory.hud());
    amigaRendererSetGameOrigin(&renderer, initialRingOrigin);
    buildReferencePicture(&renderer, builders, entrance);
    if (!amigaRendererBuildHardwareSprite(
            amiga_game_pack, amiga_game_pack_end, 31, 0,
            downwellPlayfieldX + 80, 200,
            memory.playerSprite(0), playerSpriteHeight) ||
        !amigaRendererBuildHardwareSprite(
            amiga_game_pack, amiga_game_pack_end, 37, 0,
            downwellPlayfieldX + 88, 192,
            memory.playerSprite(1), playerSpriteHeight)) {
        memory.release();
        CloseLibrary((struct Library*)GfxBase);
        GfxBase = 0;
        return 1;
    }
    hideHardwareSprite(memory.bulletSprite());
    if (!buildCopper(copperLists[0], memory.game(), memory.hud(),
                     memory.playerSprite(0), memory.bulletSprite(),
                     memory.nullSprite(),
                     initialRingOrigin) ||
        !buildCopper(copperLists[1], memory.game(), memory.hud(),
                     memory.playerSprite(1), memory.bulletSprite(),
                     memory.nullSprite(),
                     initialRingOrigin)) {
        memory.release();
        CloseLibrary((struct Library*)GfxBase);
        GfxBase = 0;
        return 1;
    }

    /*
     * Follow the Rescue on Fractalus takeover boundary: all allocations and
     * construction happen while the OS owns the machine. WaitTOF is used only
     * to retire its display before the raw VERTB vector is installed.
     */
    savedView = GfxBase->ActiView;
    osCopper = (const UWORD*)GfxBase->copinit;
    savedDmacon = AmigaHardware::readRegister(AmigaHardware::dmaconr);
    savedIntena = AmigaHardware::readRegister(AmigaHardware::intenar);
    LoadView(0);
    WaitTOF();
    WaitTOF();
    AmigaHardware::setDMAChannels(DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER,
                                  false);
    AmigaHardware::writeRegister(AmigaHardware::bplcon0, 0x0000);
    AmigaHardware::setDisplay(displayHStart, displayHStop,
                              displayVStart, displayVStop,
                              fetchStart, fetchStop);

    /* Nothing in this program consumes blit-done interrupts. */
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    AmigaHardware::clearInterruptRequests(INTF_BLIT);

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
    AmigaHardware::setCopperList(copperLists[0].data());
    AmigaHardware::setInterrupts(INTF_INTEN | INTF_VERTB, true);
    AmigaHardware::setDMAChannels(DMAF_MASTER | DMAF_COPPER |
                                  DMAF_RASTER | DMAF_SPRITE, true);

    /* No scheduling or OS calls are permitted inside the takeover window. */
    Forbid();
    while ((*ciaaPra & 0x40u) == 0) waitVblank();
    while ((*ciaaPra & 0x40u) != 0) {
        UBYTE packedInput;
        waitVblank();
        {
#ifdef DOWNWELL_REPLAY_VERIFY
            downwell::Input replayInput;
            if (!verificationReplay.frame(port.tick(), replayInput))
                break;
            packedInput = downwell::packReplayInput(replayInput);
#else
            const downwell::Input liveInput = AmigaInput::sample();
            packedInput = downwell::packReplayInput(liveInput);
#endif
            const AmigaPortFrame result = port.step(packedInput);
            const AmigaPlayerMotion motion = result.playerMotion;
            if (motion.deltaX || motion.deltaY) {
                offsetHardwareSprite(memory.playerSprite(0),
                                     motion.deltaX, motion.deltaY);
                offsetHardwareSprite(memory.playerSprite(1),
                                     motion.deltaX, motion.deltaY);
            }
            if (result.bulletChanged) {
                if (port.bulletActive())
                    buildBulletSprite(memory.bulletSprite(),
                        downwellPlayfieldX + 80 + port.bulletX(),
                        200 + port.bulletY());
                else
                    hideHardwareSprite(memory.bulletSprite());
            }
            if (result.ammoChanged)
                redrawAmmo(&renderer, (unsigned)port.player().ammo());
        }
        if ((port.tick() & 1u) == 0) {
            const unsigned back = front ^ 1u;
            ++worldBottom;
            drawRevealedGameLine(&renderer, builders, entrance,
                                 port.ringOrigin(), worldBottom);
            if (!buildCopper(copperLists[back], memory.game(), memory.hud(),
                             memory.playerSprite(
                                 port.player().grounded() ? 0u : 1u),
                             memory.bulletSprite(),
                             memory.nullSprite(), port.ringOrigin()))
                break;
            /* COP1LC is sampled at the next VERTB; never restart mid-frame. */
            AmigaHardware::setCopperList(copperLists[back].data());
            front = back;
        }
        lastPortCheckpoint = port.trajectoryDigest();
#ifdef DOWNWELL_REPLAY_VERIFY
        if (port.tick() == verificationReplay.frameCount()) {
            gReplayFrames = port.tick();
            gReplayDigestHigh = (ULONG)(lastPortCheckpoint >> 32);
            gReplayDigestLow = (ULONG)lastPortCheckpoint;
            gReplayComplete = 1;
            downwellReplayComplete();
            for (;;) waitVblank();
        }
#endif
    }

    /* Stop our interrupt and display before handing either back to exec. */
    AmigaHardware::setInterrupts(INTF_VERTB, false);
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    AmigaHardware::setDMAChannels(DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER,
                                  false);
    AmigaHardware::writeRegister(AmigaHardware::bplcon0, 0x0000);

    /* Restore graphics.library's startup copper, not merely its View list. */
    AmigaHardware::setCopperList(osCopper, true);
    AmigaHardware::writeRegister(AmigaHardware::bplcon3, 0x0c00);

    Disable();
    SysBase->IntVects[INTB_VERTB] = savedVblankVector;
    Enable();

    /* Restore the complete DMA and interrupt enable sets captured above. */
    AmigaHardware::setDMAChannels(DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG,
                                  false);
    AmigaHardware::setDMAChannels(
        (UWORD)(savedDmacon & (DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG)), true);
    AmigaHardware::setInterrupts(0x7fffu, false);
    AmigaHardware::clearInterruptRequests(INTF_BLIT | INTF_VERTB);
    AmigaHardware::setInterrupts((UWORD)(savedIntena & 0x7fffu), true);

    Permit();
    LoadView(savedView);
    WaitTOF();
    WaitTOF();

    memory.release();
    CloseLibrary((struct Library*)GfxBase);
    GfxBase = 0;
    return 0;
}
