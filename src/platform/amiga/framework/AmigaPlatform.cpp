#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "AmigaHardware.h"
#include "AmigaPlatform.h"
#include "CopperList.h"

extern struct ExecBase* SysBase;
extern struct GfxBase* GfxBase;

volatile UWORD AmigaPlatform::vblankCount_ = 0;

static_assert(__builtin_offsetof(struct ExecBase, IntVects) == 84,
              "ExecBase IntVects layout must match the VERTB takeover");

AmigaPlatform::AmigaPlatform()
    : savedView_(0), osCopper_(0), savedDmacon_(0), savedIntena_(0),
      active_(false)
{
    GfxBase = (struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library", 33);
}

AmigaPlatform::~AmigaPlatform()
{
    restore();
    if (GfxBase) {
        CloseLibrary((struct Library*)GfxBase);
        GfxBase = 0;
    }
}

bool AmigaPlatform::available() const
{
    return GfxBase != 0;
}

ULONG AmigaPlatform::vblankHandler()
{
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    ++vblankCount_;
    return 0;
}

bool AmigaPlatform::takeover(const CopperList& firstCopper,
                             UWORD hStart, UWORD hStop,
                             UWORD vStart, UWORD vStop,
                             UWORD fetchStart, UWORD fetchStop)
{
    if (!GfxBase || active_) return false;

    savedView_ = GfxBase->ActiView;
    osCopper_ = (const UWORD*)GfxBase->copinit;
    savedDmacon_ = AmigaHardware::enabledDMAChannels();
    savedIntena_ = AmigaHardware::enabledInterrupts();

    LoadView(0);
    WaitTOF();
    WaitTOF();

    AmigaHardware::setDMAChannels(DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER,
                                  false);
    AmigaHardware::writeRegister(bplcon0, 0);
    AmigaHardware::setDisplay(hStart, hStop, vStart, vStop,
                              fetchStart, fetchStop);
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    AmigaHardware::clearInterruptRequests(INTF_BLIT);

    vblankServer_.is_Node.ln_Type = NT_INTERRUPT;
    vblankServer_.is_Node.ln_Pri = 127;
    vblankServer_.is_Node.ln_Name = (char*)"Downwell VBI";
    vblankServer_.is_Data = 0;
    vblankServer_.is_Code = (void (*)())vblankHandler;

    Disable();
    savedVblankVector_ = SysBase->IntVects[INTB_VERTB];
    SysBase->IntVects[INTB_VERTB].iv_Data = 0;
    SysBase->IntVects[INTB_VERTB].iv_Code = (void (*)())vblankHandler;
    SysBase->IntVects[INTB_VERTB].iv_Node = &vblankServer_.is_Node;
    Enable();

    vblankCount_ = 0;
    AmigaHardware::setCopperList(firstCopper);
    AmigaHardware::setInterrupts(INTF_INTEN | INTF_VERTB, true);
    AmigaHardware::setDMAChannels(DMAF_MASTER | DMAF_COPPER |
                                  DMAF_RASTER | DMAF_SPRITE |
                                  DMAF_BLITTER, true);
    Forbid();
    active_ = true;
    return true;
}

void AmigaPlatform::publish(const CopperList& copper, bool immediate)
{
    AmigaHardware::setCopperList(copper, immediate);
}

void AmigaPlatform::waitVblank() const
{
    const UWORD start = vblankCount_;
    while (vblankCount_ == start) { }
}

void AmigaPlatform::restore()
{
    if (!active_) return;

    AmigaHardware::setInterrupts(INTF_VERTB, false);
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    AmigaHardware::setDMAChannels(DMAF_RASTER | DMAF_SPRITE | DMAF_COPPER,
                                  false);
    AmigaHardware::writeRegister(bplcon0, 0);
    AmigaHardware::setCopperList(osCopper_, true);
    AmigaHardware::writeRegister(bplcon3, 0x0c00);

    Disable();
    SysBase->IntVects[INTB_VERTB] = savedVblankVector_;
    Enable();

    AmigaHardware::setDMAChannels(DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG,
                                  false);
    AmigaHardware::setDMAChannels(
        (UWORD)(savedDmacon_ & (DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG)), true);
    AmigaHardware::setInterrupts(0x7fffu, false);
    AmigaHardware::clearInterruptRequests(INTF_BLIT | INTF_VERTB);
    AmigaHardware::setInterrupts((UWORD)(savedIntena_ & 0x7fffu), true);

    Permit();
    LoadView(savedView_);
    WaitTOF();
    WaitTOF();
    active_ = false;
}
