#ifndef DOWNWELL_AMIGA_PLATFORM_H
#define DOWNWELL_AMIGA_PLATFORM_H

#include <exec/interrupts.h>
#include <exec/types.h>

class CopperList;
struct View;

class AmigaPlatform {
public:
    AmigaPlatform();
    ~AmigaPlatform();

    bool available() const;
    // Before takeover only: write a line to the starting shell's output.
    void print(const char* text) const;
    bool takeover(const CopperList& firstCopper,
                  UWORD hStart, UWORD hStop,
                  UWORD vStart, UWORD vStop,
                  UWORD fetchStart, UWORD fetchStop);
    /* Set COP1LC; hardware restarts the list at the next vertical blank. */
    void publish(const CopperList& copper);
    const UWORD* publishedCopper() const { return publishedCopper_; }
    void waitVblank() const;
    // Returns once a VBlank has followed both the latest publish (its list
    // is live, so the previously displayed buffers are free) and `since`.
    // Returns the count it observed, for the next call's `since`.
    UWORD waitPublished(UWORD since) const;
    UWORD waitPublished(UWORD since,bool (*idleWork)(void*),void* context,unsigned reserveLines) const;
    // PAL idle work only. A callback performs one bounded unit, returns false
    // when idle/failed, and must not touch display state or simulation state.
    // reserveLines is a scheduling margin, not a preemptive execution limit.
    void waitVblank(bool (*idleWork)(void*),void* context,unsigned reserveLines) const;
    static UWORD vblankCount() { return vblankCount_; }
    // One optional device service, after beam-critical display publication and
    // profiling. The owner must detach before releasing its state or restoring
    // the machine; no service runs while DOS temporarily owns VERTB.
    bool attachVblankService(void (*service)(void*),void* context,void (*retire)(void*));
    void detachVblankService(void* context);
    // Scene-boundary file access: one Open/Read/Close of the whole file into
    // destination (Chip is fine). While it runs the OS gets back its VERTB
    // servers, interrupts and disk DMA; the display stays owned. Drain the
    // blitter first. Returns the bytes read, or -1.
    LONG loadFile(const char* name, UBYTE* destination, ULONG bytes);
    // A game data file by its bare name: PROGDIR:data/name (the installed
    // layout), else PROGDIR:name (a development drawer). The second is tried
    // only when the first does not open. Same result as loadFile.
    LONG loadData(const char* name, UBYTE* destination, ULONG bytes);
    // Before takeover: the size of the file loadData would read, or -1 when
    // neither place has it (or it is not a file). installed tells where it
    // was found (PROGDIR:data/) or, when missing, where it belongs.
    LONG dataSize(const char* name, bool& installed) const;
    // The same bracketing for one Open(MODE_NEWFILE)/Write/Close. DOS
    // requesters (a write-protected or missing volume) are suppressed: the
    // call fails instead of waiting on a Workbench window nobody can see.
    bool saveFile(const char* name, const UBYTE* source, ULONG bytes);
    void restore();

private:
    LONG loadFile(const char* name, UBYTE* destination, ULONG bytes, bool* opened);
    AmigaPlatform(const AmigaPlatform&);
    AmigaPlatform& operator=(const AmigaPlatform&);

    static ULONG vblankHandler();
    void serviceIdleUntilVblank(UWORD start,bool (*idleWork)(void*),void* context,unsigned reserveLines) const;
    void beginIo();
    void endIo();
    static volatile UWORD vblankCount_;
    static void (*vblankService_)(void*);
    static void* vblankServiceContext_;
    static void (*vblankServiceRetire_)(void*);
    const UWORD* publishedCopper_;
    UWORD publishedVblank_;

    struct View* savedView_;
    const UWORD* osCopper_;
    UWORD savedDmacon_;
    UWORD savedIntena_;
    struct Interrupt vblankServer_;
    struct IntVector savedVblankVector_;
    bool active_;
};

#endif
