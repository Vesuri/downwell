/* Force-included into Tremor and libogg: their allocations come from one
   block (vorbis_heap.c), so they behave the same on the host and the
   Amiga, and floor1's qsort is our stable insertion sort. */
#include <stddef.h>
void* vh_malloc(size_t n);
void* vh_calloc(size_t a, size_t n);
void* vh_realloc(void* p, size_t n);
void vh_free(void* p);
void vh_qsort(void* base, size_t count, size_t size, int (*cmp)(const void*, const void*));
#define malloc vh_malloc
#define calloc vh_calloc
#define realloc vh_realloc
#define free vh_free
#define qsort vh_qsort
#define alloca __builtin_alloca
