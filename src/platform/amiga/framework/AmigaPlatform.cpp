#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
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
struct DosLibrary* DOSBase = 0;

volatile UWORD AmigaPlatform::vblankCount_ = 0;
void (*AmigaPlatform::vblankService_)(void*) = 0;
void* AmigaPlatform::vblankServiceContext_ = 0;
void (*AmigaPlatform::vblankServiceRetire_)(void*) = 0;

static_assert(__builtin_offsetof(struct ExecBase, IntVects) == 84,
              "ExecBase IntVects layout must match the VERTB takeover");

AmigaPlatform::AmigaPlatform()
    : publishedCopper_(0), publishedVblank_(0), savedView_(0), osCopper_(0), savedDmacon_(0), savedIntena_(0),
      active_(false)
{
    GfxBase = (struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library", 33);
    DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 36);
}

AmigaPlatform::~AmigaPlatform()
{
    restore();
    if (GfxBase) {
        CloseLibrary((struct Library*)GfxBase);
        GfxBase = 0;
    }
    if (DOSBase) {
        CloseLibrary((struct Library*)DOSBase);
        DOSBase = 0;
    }
}

bool AmigaPlatform::available() const
{
    return GfxBase != 0 && DOSBase != 0;
}

void AmigaPlatform::print(const char* text) const
{
    if (DOSBase && !active_) PutStr((CONST_STRPTR)text);
}

void AmigaPlatform::beginIo()
{
    // Undo exactly what takeover changed for the OS: its VERTB servers, the
    // blitter interrupt, and multitasking. Disk DMA is never switched off.
    // Our copper, raster and sprite DMA stay live.
    Disable();
    SysBase->IntVects[INTB_VERTB] = savedVblankVector_;
    AmigaHardware::setInterrupts((UWORD)(savedIntena_ & INTF_BLIT), true);
    Enable();
    Permit();
}

void AmigaPlatform::endIo()
{
    Forbid();
    Disable();
    SysBase->IntVects[INTB_VERTB].iv_Data = 0;
    SysBase->IntVects[INTB_VERTB].iv_Code = (void (*)())vblankHandler;
    SysBase->IntVects[INTB_VERTB].iv_Node = &vblankServer_.is_Node;
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    AmigaHardware::clearInterruptRequests(INTF_BLIT);
    Enable();
}

// The installer puts the converted files in data/ beside the program; a
// development drawer keeps them beside it (as the sibling ports do).
static const char* const dataDirectories[] = {"PROGDIR:data/", "PROGDIR:"};

static bool dataPath(char (&path)[96], const char* directory, const char* name)
{
    unsigned length = 0;
    for (const char* c = directory; *c; ++c) path[length++] = *c;
    for (const char* c = name; *c; ++c) {
        if (length + 1 >= sizeof path) return false;
        path[length++] = *c;
    }
    path[length] = 0;
    return true;
}

LONG AmigaPlatform::loadData(const char* name, UBYTE* destination, ULONG bytes)
{
    char path[96];
    for (const char* directory : dataDirectories) {
        if (!dataPath(path, directory, name)) return -1;
        bool opened = false;
        const LONG read = loadFile(path, destination, bytes, &opened);
        if (opened) return read;
    }
    return -1;
}

LONG AmigaPlatform::dataSize(const char* name, bool& installed) const
{
    installed = true;
    if (!DOSBase || active_) return -1;
    struct FileInfoBlock* const info = (struct FileInfoBlock*)AllocDosObject(DOS_FIB, 0);
    if (!info) return -1;
    char path[96];
    LONG size = -1;
    for (unsigned i = 0; i < 2 && dataPath(path, dataDirectories[i], name); ++i) {
        const BPTR lock = Lock((CONST_STRPTR)path, ACCESS_READ);
        if (!lock) continue;
        installed = i == 0;
        if (Examine(lock, info) && info->fib_DirEntryType < 0) size = info->fib_Size;
        UnLock(lock);
        break;
    }
    FreeDosObject(DOS_FIB, info);
    return size;
}

LONG AmigaPlatform::loadFile(const char* name, UBYTE* destination, ULONG bytes)
{
    return loadFile(name, destination, bytes, nullptr);
}

LONG AmigaPlatform::loadFile(const char* name, UBYTE* destination, ULONG bytes, bool* opened)
{
    DW_PROFILE_SCOPE(profileSceneFileIo);
    if (!DOSBase || !destination) return -1;
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
    const ULONG osFieldsBefore = active_ ?
        *static_cast<volatile ULONG*>(&GfxBase->VBCounter) : 0;
#endif
    if (active_) beginIo();
    LONG read = -1;
    const BPTR file = Open((CONST_STRPTR)name, MODE_OLDFILE);
    if (file) {
        if (opened) *opened = true;
        read = Read(file, destination, (LONG)bytes);
        Close(file);
    }
    if (active_) {
        endIo();
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
        // VBCounter runs only while the OS owns VERTB. Transfer its elapsed
        // fields before the phase's next beam stamp; do not race our ISR's
        // read/modify/write of the same profiling epoch.
        Disable();
        downwellProfileOsFields(
            *static_cast<volatile ULONG*>(&GfxBase->VBCounter)-osFieldsBefore);
        Enable();
#endif
        // graphics.library's VERTB server may have loaded its own copper
        // list, and the OS may have left a blit running.
        AmigaHardware::blitterDrain();
        if (publishedCopper_) AmigaHardware::setCopperList(publishedCopper_, false);
    }
    return read;
}

bool AmigaPlatform::saveFile(const char* name, const UBYTE* source, ULONG bytes)
{
    DW_PROFILE_SCOPE(profileSceneFileIo);
    if (!DOSBase || !source || !name) return false;
    if (active_) beginIo();
    struct Process* const self = (struct Process*)FindTask(0);
    const APTR window = self->pr_WindowPtr;
    self->pr_WindowPtr = (APTR)-1;
    bool written = false;
    const BPTR file = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (file) {
        written = Write(file, (APTR)source, (LONG)bytes) == (LONG)bytes;
        Close(file); // Kickstart 1.3's Close returns no result.
    }
    self->pr_WindowPtr = window;
    if (active_) {
        endIo();
        AmigaHardware::blitterDrain();
        if (publishedCopper_) AmigaHardware::setCopperList(publishedCopper_, false);
    }
    return written;
}

ULONG AmigaPlatform::vblankHandler()
{
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    ++vblankCount_;
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
    downwellProfileVblank();
#endif
    if(vblankService_)vblankService_(vblankServiceContext_);
    return 0;
}

bool AmigaPlatform::attachVblankService(void (*service)(void*),void* context,void (*retire)(void*))
{
    if(!active_||!service||!context||!retire)return false;
    Disable();
    const bool available=!vblankService_;
    if(available){vblankServiceContext_=context;vblankServiceRetire_=retire;vblankService_=service;}
    Enable();return available;
}

void AmigaPlatform::detachVblankService(void* context)
{
    Disable();
    if(vblankServiceContext_==context){vblankService_=0;vblankServiceContext_=0;vblankServiceRetire_=0;}
    Enable();
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
    // Sample after COP1LC: a VBlank between them only delays waitPublished.
    publishedVblank_ = vblankCount_;
}

void AmigaPlatform::waitVblank() const
{
    const UWORD start = vblankCount_;
    while (vblankCount_ == start) { }
}

UWORD AmigaPlatform::waitPublished(UWORD since) const
{
    UWORD now;
    while ((now = vblankCount_) == publishedVblank_ || now == since) { }
    return now;
}

void AmigaPlatform::waitVblank(bool (*idleWork)(void*),void* context,unsigned reserveLines) const
{
    const UWORD start=vblankCount_;
    serviceIdleUntilVblank(start,idleWork,context,reserveLines);
    // If work crossed the boundary, do not accidentally wait another field.
    while(vblankCount_==start){}
}

UWORD AmigaPlatform::waitPublished(UWORD since,bool (*idleWork)(void*),void* context,unsigned reserveLines) const
{
    const UWORD start=vblankCount_;
    if(start==publishedVblank_||start==since)
        serviceIdleUntilVblank(start,idleWork,context,reserveLines);
    return waitPublished(since);
}

void AmigaPlatform::serviceIdleUntilVblank(UWORD start,bool (*idleWork)(void*),void* context,unsigned reserveLines) const
{
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
}

void AmigaPlatform::restore()
{
    if (!active_) return;

    // Retire a borrowed device before restoring OS interrupts, including on
    // early-return paths. Its detach clears this callback before freeing state.
    if(vblankServiceRetire_)vblankServiceRetire_(vblankServiceContext_);

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
