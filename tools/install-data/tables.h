/* Downwell.tables: the derived tables, one tagged section each.
   Section layout (tools/install-table-headers.py, big-endian): u16 tables,
   u16 bound flag (0), u32 relocations, u32 offset of each table, u32
   position of each relocated pointer, then the tables (4-byte aligned)
   and the strings they point at. */
#ifndef DW_TABLES_H
#define DW_TABLES_H
#include "dw.h"
#define SECTION_TABLES 64
typedef struct { u32 table, at, kind, target, len; } SectionPointer; /* kind 0: string at strings+target */
typedef struct {
    int ntables;
    Buf tables[SECTION_TABLES];
    Buf pointers; /* SectionPointer records */
    Buf strings;  /* the bytes of string targets */
} Section;
void sec_init(Section* s);
void sec_free(Section* s);
/* A new table's data buffer; tables keep the order they were added in. */
Buf* sec_table(Section* s);
/* Pointer fields: 4 zero bytes already in the table at `at`. */
void sec_ptr_string(Section* s, int table, u32 at, const void* bytes, u32 n);
void sec_ptr_table(Section* s, int table, u32 at, int target);
int sec_finish(Section* s, Buf* out);

/* The packed file: "DWTB", u16 version, u16 count, {tag, offset, bytes}... */
#define TABLES_MAX 32
typedef struct { u32 tag; Buf data; } TableSection;
typedef struct { int n; TableSection s[TABLES_MAX]; } TablesFile;
void tables_init(TablesFile* t);
/* The section of `tag`; the file keeps tools/pack-amiga-tables.py's order. */
Buf* tables_add(TablesFile* t, const char* tag);
/* Only the sections built so far, for tests while the converter grows. */
int tables_partial(TablesFile* t, const char* dir);
int tables_pack(TablesFile* t, Buf* out);
#endif
