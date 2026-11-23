/* The prepared-art stage's parts (derived.c runs them in order). Each takes
   the scene banks it reads, already loaded, and adds its own outputs. */
#ifndef DW_DERIVED_PARTS_H
#define DW_DERIVED_PARTS_H
#ifdef __cplusplus
extern "C" {
#endif
#include "context.h"

/* A DWAP v2 sprite record (24 bytes at 20 + 24 * i). */
typedef struct { u16 sprite, frame; s16 ox, oy; u16 width, height, stride, flags; u32 offset, size; } DwapRecord;
u32 dwap_count(const Buf* pack);
void dwap_record(const Buf* pack, u32 i, DwapRecord* r);

/* derived_hud.cpp: AMDV, AMFA, HART (Downwell.hud), HUDT (and Downwell.game). */
int derived_ammo_dividers(Context* c, const Buf* hud);
int derived_ammo_factored(Context* c, const Buf* hud);
int derived_health_art(Context* c, const Buf* hud);
int derived_hud_text(Context* c, const Buf* hud, const Buf* game);
/* derived_initial.cpp: TINI from the room pack and the TPH6 words. */
int derived_initial_terrain(Context* c, const u8* phase6);
/* derived_terrain.c: Downwell.terrain. */
int derived_pattern_terrain(Context* c, const Buf* game, const Buf* patterns);
/* derived_planes.c: Downwell.planes, Downwell.cavernplanes. */
int derived_plane_bank(Context* c, const Buf* pack, const char* name);

#ifdef __cplusplus
}
#endif
#endif
