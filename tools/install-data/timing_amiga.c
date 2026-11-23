/* Timing build only: the 50 Hz clock for install.c's stage marks. */
#ifdef INSTALL_TIMING
#include <proto/dos.h>
long install_now(void)
{
    struct DateStamp d;
    DateStamp(&d);
    return d.ds_Days * 4320000L + d.ds_Minute * 3000L + d.ds_Tick;
}
#endif
