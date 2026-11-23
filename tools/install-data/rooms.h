/* data.win OBJT and ROOM: objects (OBJT), runtime rooms (ROOM), room terrain plan (RTPL). */
#ifndef DW_ROOMS_H
#define DW_ROOMS_H
#include "context.h"
int stage_rooms(Context* c);
/* generated/downwell-rooms.dwrm's bytes (the full room pack, not installed;
   the initial terrain is prepared from it). */
int rooms_full_pack(Context* c, Buf* out);
#endif
