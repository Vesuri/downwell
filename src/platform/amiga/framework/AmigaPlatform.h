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
    void publish(const CopperList& copper);
    void waitVblank() const;
    void restore();

private:
    AmigaPlatform(const AmigaPlatform&);
    AmigaPlatform& operator=(const AmigaPlatform&);

    static ULONG vblankHandler();
    static volatile UWORD vblankCount_;

    struct View* savedView_;
    const UWORD* osCopper_;
    UWORD savedDmacon_;
    UWORD savedIntena_;
    struct Interrupt vblankServer_;
    struct IntVector savedVblankVector_;
    bool active_;
};

#endif
