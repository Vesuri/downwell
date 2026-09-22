// GCC runtime support shared with the dA JoRMaS Amiga ports.
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>

struct ExecBase* SysBase = 0;
struct GfxBase* GfxBase = 0;

__attribute__((constructor)) static void initializeSysBase()
{
    SysBase = *(struct ExecBase**)4UL;
}

void* operator new(unsigned long bytes)
{
    unsigned long* allocation =
        (unsigned long*)AllocMem(bytes + sizeof(unsigned long),
                                 MEMF_ANY | MEMF_CLEAR);
    if (!allocation) return 0;
    *allocation = bytes + sizeof(unsigned long);
    return allocation + 1;
}

void* operator new[](unsigned long bytes) { return operator new(bytes); }

void operator delete(void* pointer)
{
    if (!pointer) return;
    unsigned long* allocation = (unsigned long*)pointer - 1;
    FreeMem(allocation, *allocation);
}

void operator delete[](void* pointer) { operator delete(pointer); }
void operator delete(void* pointer, unsigned long) { operator delete(pointer); }
void operator delete[](void* pointer, unsigned long) { operator delete(pointer); }

extern "C" void __cxa_pure_virtual() { for (;;) { } }

extern "C" int abs(int value) { return value < 0 ? -value : value; }
extern "C" long labs(long value) { return value < 0 ? -value : value; }

extern "C" void qsort(void* base, unsigned long count, unsigned long size,
                       int (*compare)(const void*, const void*))
{
    char* bytes = (char*)base;
    for (unsigned long i = 1; i < count; ++i) {
        for (unsigned long j = i;
             j > 0 && compare(bytes + (j - 1) * size,
                              bytes + j * size) > 0;
             --j) {
            char* left = bytes + (j - 1) * size;
            char* right = bytes + j * size;
            for (unsigned long k = 0; k < size; ++k) {
                const char temporary = left[k];
                left[k] = right[k];
                right[k] = temporary;
            }
        }
    }
}
