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
    bool takeover(const CopperList& firstCopper,
                  UWORD hStart, UWORD hStop,
                  UWORD vStart, UWORD vStop,
                  UWORD fetchStart, UWORD fetchStop);
    /* Set COP1LC; hardware restarts the list at the next vertical blank. */
    void publish(const CopperList& copper);
    const UWORD* publishedCopper() const { return publishedCopper_; }
    void waitVblank() const;
    // PAL idle work only. A callback performs one bounded unit, returns false
    // when idle/failed, and must not touch display state or simulation state.
    // reserveLines is a scheduling margin, not a preemptive execution limit.
    void waitVblank(bool (*idleWork)(void*),void* context,unsigned reserveLines) const;
    static UWORD vblankCount() { return vblankCount_; }
    void restore();

private:
    AmigaPlatform(const AmigaPlatform&);
    AmigaPlatform& operator=(const AmigaPlatform&);

    static ULONG vblankHandler();
    static volatile UWORD vblankCount_;
    const UWORD* publishedCopper_;

    struct View* savedView_;
    const UWORD* osCopper_;
    UWORD savedDmacon_;
    UWORD savedIntena_;
    struct Interrupt vblankServer_;
    struct IntVector savedVblankVector_;
    bool active_;
};

#endif
