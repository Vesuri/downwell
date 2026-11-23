/* Timing build only: the 50 Hz clock for install.c's stage marks. */
#ifdef INSTALL_TIMING
#include <proto/dos.h>
#include <proto/exec.h>
/* CacheControl(0, 0): the CACR state the converter ran with (CACRF_*). */
volatile unsigned long install_cache_bits;
void install_cache_probe(void) { install_cache_bits = CacheControl(0, 0); }
long install_now(void)
{
    struct DateStamp d;
    DateStamp(&d);
    return d.ds_Days * 4320000L + d.ds_Minute * 3000L + d.ds_Tick;
}
#endif
