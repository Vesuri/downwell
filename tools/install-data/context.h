/* The converter's state for one run. */
#ifndef DW_CONTEXT_H
#define DW_CONTEXT_H
#include "win.h"
#include "sound.h"
#include "tables.h"
struct Context {
    const char* source;  /* the player's Steam drawer */
    const char* dest;    /* where the installed files go */
    Win win;             /* data.win */
    Sounds sounds;
    TablesFile tables;   /* Downwell.tables, packed last */
};
/* Open a file in the source drawer. */
int ctx_open(Context* c, const char* name, File* f);
/* Read back a file an earlier stage wrote to the destination (the scene
   banks): the whole file into a new buffer. */
int ctx_read_output(Context* c, const char* name, Buf* out);
#endif
