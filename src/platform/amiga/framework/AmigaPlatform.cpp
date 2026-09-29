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
#include "../AmigaProfile.h"

extern struct ExecBase* SysBase;
extern struct GfxBase* GfxBase;

volatile UWORD AmigaPlatform::vblankCount_ = 0;

static_assert(__builtin_offsetof(struct ExecBase, IntVects) == 84,
              "ExecBase IntVects layout must match the VERTB takeover");

AmigaPlatform::AmigaPlatform()
    : publishedCopper_(0), savedView_(0), osCopper_(0), savedDmacon_(0), savedIntena_(0),
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
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
    downwellProfileVblank();
#endif
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
    publish(firstCopper);
    AmigaHardware::setInterrupts(INTF_INTEN | INTF_VERTB, true);
    AmigaHardware::setDMAChannels(DMAF_MASTER | DMAF_COPPER |
                                  DMAF_RASTER | DMAF_SPRITE |
                                  DMAF_BLITTER, true);
    Forbid();
    active_ = true;
    return true;
}

void AmigaPlatform::publish(const CopperList& copper)
{
    AmigaHardware::setCopperList(copper, false);
    publishedCopper_ = copper.data();
}

void AmigaPlatform::waitVblank() const
{
    const UWORD start = vblankCount_;
    while (vblankCount_ == start) { }
}

void AmigaPlatform::waitVblank(bool (*idleWork)(void*),void* context,unsigned reserveLines) const
{
    const UWORD start=vblankCount_;
    while(idleWork && vblankCount_==start){
        // Stable nine-bit vertical position across the 255/256 boundary.
        // A single longword read can combine the old high bit and new low byte.
        UWORD high,low;
        do{
            high=AmigaHardware::readRegister(vposr);
            low=AmigaHardware::readRegister(vhposr);
        }while((high^AmigaHardware::readRegister(vposr))&1u);
        const unsigned line=((high&1u)<<8)|(low>>8);
        // 312 is conservative for either PAL field length. Recheck the VBI
        // after sampling so a wrapped beam never looks like a new work budget.
        if(vblankCount_!=start || line>=312u || reserveLines>=312u-line || !idleWork(context))break;
    }
    // If work crossed the boundary, do not accidentally wait another field.
    while(vblankCount_==start){}
}

void AmigaPlatform::restore()
{
    if (!active_) return;

    publishedCopper_ = 0;
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
