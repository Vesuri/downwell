#include <stddef.h>
typedef struct FILE FILE;
size_t fread(void*, size_t, size_t, FILE*); int fseek(FILE*, long, int); int fclose(FILE*); long ftell(FILE*);
FILE* fopen(const char*, const char*);
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
