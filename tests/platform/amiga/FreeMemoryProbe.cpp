// WR.1 budget probe: the memory a program finds free right after a boot to
// the shell. Run it as dh1:Downwell with amiga/probe_free_memory.gdb; its own
// hunks are a few hundred bytes, so this is the game's whole budget.
#include <proto/exec.h>
#include <exec/memory.h>

extern "C" {
volatile unsigned long gFreeChip=0,gFreeChipLargest=0,gFreeFast=0,gFreeFastLargest=0;
volatile unsigned long gFreePublic=0,gFreePublicLargest=0;
__attribute__((noinline)) void downwellFreeMemoryMeasured(){asm volatile("" ::: "memory");}
}

int main()
{
    gFreeChip=AvailMem(MEMF_CHIP);
    gFreeChipLargest=AvailMem(MEMF_CHIP|MEMF_LARGEST);
    gFreeFast=AvailMem(MEMF_FAST);
    gFreeFastLargest=AvailMem(MEMF_FAST|MEMF_LARGEST);
    gFreePublic=AvailMem(MEMF_PUBLIC);
    gFreePublicLargest=AvailMem(MEMF_PUBLIC|MEMF_LARGEST);
    downwellFreeMemoryMeasured();
    return 0;
}
