/* A first-fit heap over one block for Tremor and libogg (vorbis_alloc.h).
   Decoding one effect needs about 171 KB (measured on the A1200). */
#include "vorbis_heap.h"
#include "dw.h"

typedef struct Block { size_t size; struct Block* next; } Block; /* size includes the header */
static char* base;
static size_t cap, top, peak;
static Block* freelist;

void vh_init(void* p, size_t n) { base = p; cap = n; top = 0; freelist = 0; }
size_t vh_peak(void) { return peak; }
static size_t whole(size_t n) { return (n + sizeof(Block) + 7) & ~(size_t)7; }

void* vh_malloc(size_t n)
{
    size_t need = whole(n);
    Block **pp = &freelist, *b;
    for (; (b = *pp) != 0; pp = &b->next)
        if (b->size >= need) { *pp = b->next; return b + 1; }
    if (top + need > cap) return 0;
    b = (Block*)(base + top);
    b->size = need;
    top += need;
    if (top > peak) peak = top;
    return b + 1;
}

void vh_free(void* p)
{
    Block* b;
    if (!p) return;
    b = (Block*)p - 1;
    if ((char*)b + b->size == base + top) { top -= b->size; return; }
    b->next = freelist;
    freelist = b;
}

void* vh_calloc(size_t a, size_t n) { void* p = vh_malloc(a * n); if (p) memset(p, 0, a * n); return p; }

void* vh_realloc(void* p, size_t n)
{
    Block* b;
    void* q;
    size_t need = whole(n);
    if (!p) return vh_malloc(n);
    b = (Block*)p - 1;
    if (b->size >= need) return p;
    if ((char*)b + b->size == base + top && (size_t)((char*)b - base) + need <= cap) {
        top = (size_t)((char*)b - base) + need;
        b->size = need;
        if (top > peak) peak = top;
        return p;
    }
    q = vh_malloc(n);
    if (!q) return 0;
    memcpy(q, p, b->size - sizeof(Block));
    vh_free(p);
    return q;
}

/* Stable and the same everywhere; floor1 sorts at most a few dozen posts. */
void vh_qsort(void* items, size_t count, size_t size, int (*cmp)(const void*, const void*))
{
    char* p = items;
    size_t i, j, k;
    for (i = 1; i < count; ++i)
        for (j = i; j > 0 && cmp(p + (j - 1) * size, p + j * size) > 0; --j)
            for (k = 0; k < size; ++k) {
                char t = p[(j - 1) * size + k];
                p[(j - 1) * size + k] = p[j * size + k];
                p[j * size + k] = t;
            }
}

#ifdef INSTALL_AMIGA
/* The few C library functions Tremor and libogg call besides memcpy,
   memset, memcmp and strlen (io_amiga.c). */
int errno;
void* memchr(const void* s, int c, size_t n) { const unsigned char* p = s; for (; n; --n, ++p) if (*p == (unsigned char)c) return (void*)p; return 0; }
void* memmove(void* d, const void* s, size_t n) { char* a = d; const char* b = s; if (a < b) while (n--) *a++ = *b++; else { a += n; b += n; while (n--) *--a = *--b; } return d; }
char* strcpy(char* d, const char* s) { char* r = d; while ((*d++ = *s++) != 0) continue; return r; }
char* strcat(char* d, const char* s) { strcpy(d + strlen(d), s); return d; }
#endif
