/* Amiga build of Tremor and libogg: the C library declarations they use. */
#include <stddef.h>
void* memcpy(void*, const void*, size_t);
void* memset(void*, int, size_t);
void* memmove(void*, const void*, size_t);
void* memchr(const void*, int, size_t);
int memcmp(const void*, const void*, size_t);
size_t strlen(const char*);
char* strcpy(char*, const char*);
char* strcat(char*, const char*);
