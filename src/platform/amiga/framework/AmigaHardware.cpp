#include "AmigaHardware.h"
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

namespace {
volatile UWORD* const custom = (volatile UWORD*)0xdff000;
}

UWORD AmigaHardware::readRegister(UWORD offset)
{
    return custom[offset >> 1];
}

void AmigaHardware::writeRegister(UWORD offset, UWORD value)
{
    custom[offset >> 1] = value;
}

void AmigaHardware::setCopperList(const UWORD* list, bool immediate)
{
    const ULONG address = (ULONG)list;
    writeRegister(cop1lc, (UWORD)(address >> 16));
    writeRegister(cop1lc + 2, (UWORD)address);
    if (immediate)
        writeRegister(copjmp1, 0);
}

void AmigaHardware::setDMAChannels(UWORD channels, bool enabled)
{
    writeRegister(dmacon, (UWORD)(channels | (enabled ? DMAF_SETCLR : 0)));
}

void AmigaHardware::setInterrupts(UWORD interrupts, bool enabled)
{
    writeRegister(intena, (UWORD)(interrupts | (enabled ? INTF_SETCLR : 0)));
}

void AmigaHardware::clearInterruptRequests(UWORD interrupts)
{
    /* The repeated write is the proven sibling-port acknowledgement sequence. */
    writeRegister(intreq, interrupts);
    writeRegister(intreq, interrupts);
}

void AmigaHardware::setDisplay(UWORD hStart, UWORD hStop,
                               UWORD vStart, UWORD vStop,
                               UWORD dataFetchStart, UWORD dataFetchStop)
{
    writeRegister(diwstrt, (UWORD)((vStart << 8) | (hStart & 0xff)));
    writeRegister(diwstop,
                  (UWORD)(((vStop & 0xff) << 8) | (hStop & 0xff)));
    writeRegister(ddfstrt, dataFetchStart);
    writeRegister(ddfstop, dataFetchStop);
    writeRegister(fmode, 0x0000);
    writeRegister(bplcon1, 0x0000);
    writeRegister(bplcon2, 0x0064);
    writeRegister(bplcon3, 0x0c30);
}
