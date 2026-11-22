/* Program entry, in place of the shared gcc8 _start (renamed away in
 * amiga/Makefile). That one neither takes a Workbench message nor returns
 * main's result. Same structure as the sibling ports' start.c.
 *
 * A process started from its icon has no pr_CLI. Its WBStartup message is
 * taken before anything else runs, constructors included. It is replied
 * after the finalizers, under Forbid(): once Workbench has the message back
 * it may unload this code, and Forbid() keeps it from running before this
 * process has ended (the exit breaks the Forbid). A Shell start has no
 * message. DOS enters the first hunk at offset 0; amiga/Makefile checks that
 * _start is there. */
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>

typedef void (*DownwellInitFunction)(void);
extern DownwellInitFunction __preinit_array_start[] __attribute__((weak));
extern DownwellInitFunction __preinit_array_end[] __attribute__((weak));
extern DownwellInitFunction __init_array_start[] __attribute__((weak));
extern DownwellInitFunction __init_array_end[] __attribute__((weak));
extern DownwellInitFunction __fini_array_start[] __attribute__((weak));
extern DownwellInitFunction __fini_array_end[] __attribute__((weak));
int main(void);

struct WBStartup* gDownwellWorkbenchStartup = 0;

/* Audit seam (amiga/workbench_check.sh): the result DOS or Workbench gets. */
void __attribute__((noinline, used)) downwellStartReturn(int result)
{
    __asm__ volatile("" : : "r"(result) : "memory");
}

__attribute__((used, section(".text.unlikely"))) int _start(void)
{
    /* The global SysBase is set by a constructor (GCCRuntime.cpp), which has
     * not run yet: read ExecBase through asm into a local of the same name. */
    struct ExecBase* SysBase;
    __asm__ volatile("move.l 4.w,%0" : "=r"(SysBase));
    struct Process* self = (struct Process*)FindTask(0);
    if (!self->pr_CLI) {
        WaitPort(&self->pr_MsgPort);
        gDownwellWorkbenchStartup = (struct WBStartup*)GetMsg(&self->pr_MsgPort);
    }
    for (DownwellInitFunction* p = __preinit_array_start; p != __preinit_array_end; ++p) (*p)();
    for (DownwellInitFunction* p = __init_array_start; p != __init_array_end; ++p) (*p)();
    const int result = main();
    for (DownwellInitFunction* p = __fini_array_end; p != __fini_array_start;) (*--p)();
    downwellStartReturn(result);
    if (gDownwellWorkbenchStartup) {
        Forbid();
        ReplyMsg(&gDownwellWorkbenchStartup->sm_Message);
    }
    return result;
}
