#include <exec/types.h>
struct Library;
UWORD testAllocPotBits(Library*,UWORD);
void testFreePotBits(Library*,UWORD);
void testWritePotgo(Library*,UWORD,UWORD);
#define AllocPotBits(bits) testAllocPotBits(PotgoBase,bits)
#define FreePotBits(bits) testFreePotBits(PotgoBase,bits)
#define WritePotgo(word,mask) testWritePotgo(PotgoBase,word,mask)
