/* data.win OBJT and ROOM: objects (OBJT), runtime rooms (ROOM), room terrain plan (RTPL). */
#include "rooms.h"

int stage_rooms(Context* c)
{
    (void)c;
    return 1;
}

int rooms_full_pack(Context* c, Buf* out)
{
    (void)c; (void)out;
    return dw_fail("the full room pack is not built yet");
}
