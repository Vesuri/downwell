#ifndef DOWNWELL_AMIGA_HARDWARE_H
#define DOWNWELL_AMIGA_HARDWARE_H

#include <exec/types.h>

class AmigaHardware {
public:
    enum Register {
        dmaconr = 0x002,
        intenar = 0x01c,
        cop1lc = 0x080,
        copjmp1 = 0x088,
        diwstrt = 0x08e,
        diwstop = 0x090,
        ddfstrt = 0x092,
        ddfstop = 0x094,
        dmacon = 0x096,
        intena = 0x09a,
        intreq = 0x09c,
        bpl1pth = 0x0e0,
        bplcon0 = 0x100,
        bplcon1 = 0x102,
        bplcon2 = 0x104,
        bplcon3 = 0x106,
        bpl1mod = 0x108,
        bpl2mod = 0x10a,
        spr0pth = 0x120,
        color00 = 0x180,
        fmode = 0x1fc
    };

    static UWORD readRegister(UWORD offset);
    static void writeRegister(UWORD offset, UWORD value);
    static void setCopperList(const UWORD* list, bool immediate = false);
    static void setDMAChannels(UWORD channels, bool enabled);
    static void setInterrupts(UWORD interrupts, bool enabled);
    static void clearInterruptRequests(UWORD interrupts);
    static void setDisplay(UWORD hStart, UWORD hStop,
                           UWORD vStart, UWORD vStop,
                           UWORD fetchStart, UWORD fetchStop);
};

#endif
