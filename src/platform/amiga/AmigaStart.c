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
 * _start is there.
 *
 * WI.6: main runs on at least downwellStackBytes of stack. A smaller one
 * (a Shell's default 4096, an icon's own setting) is not refused but
 * replaced: exec's StackSwap (V36) moves main onto a stack in public memory
 * and back afterwards. Kickstart 1.3 keeps its stack and main refuses a
 * small one there (Downwell needs Kickstart 2.0 anyway). */
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <workbench/startup.h>

typedef void (*DownwellInitFunction)(void);
extern DownwellInitFunction __preinit_array_start[] __attribute__((weak));
extern DownwellInitFunction __preinit_array_end[] __attribute__((weak));
extern DownwellInitFunction __init_array_start[] __attribute__((weak));
extern DownwellInitFunction __init_array_end[] __attribute__((weak));
extern DownwellInitFunction __fini_array_start[] __attribute__((weak));
extern DownwellInitFunction __fini_array_end[] __attribute__((weak));
int main(void);
enum { downwellStackBytes = 8192 };

struct WBStartup* gDownwellWorkbenchStartup = 0;

/* WI.6: a start that refuses says why, once, then main returns 20. From
 * the Shell the line goes to its output; from Workbench (no output) into a
 * requester, which needs intuition V36. Libraries are opened here, so this
 * works before AmigaPlatform exists and after it is gone.
 * The verifiers break here and read the message. */
const char* volatile gDownwellRefusal = 0;

void __attribute__((noinline, used)) downwellRefuse(const char* message)
{
    if (gDownwellRefusal) return;
    gDownwellRefusal = message;
    if (!gDownwellWorkbenchStartup) {
        struct DosLibrary* DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
        if (!DOSBase) return;
        const BPTR out = Output();
        if (out) {
            LONG length = 0;
            while (message[length]) ++length;
            Write(out, (APTR)message, length);
            Write(out, (APTR)"\n", 1);
        }
        CloseLibrary(&DOSBase->dl_lib);
        return;
    }
    struct IntuitionBase* IntuitionBase =
        (struct IntuitionBase*)OpenLibrary((CONST_STRPTR)"intuition.library", 36);
    if (!IntuitionBase) return;
    /* Static: a local aggregate would be copied in by memcpy, which is not
     * 68000 code (this file must call nothing built for AMIGA_CPU). */
    static struct EasyStruct request = {sizeof(struct EasyStruct), 0, (UBYTE*)"Downwell",
                                        (UBYTE*)"%s", (UBYTE*)"OK"};
    EasyRequestArgs(0, &request, 0, (APTR)&message);
    CloseLibrary((struct Library*)IntuitionBase);
}

#ifdef DOWNWELL_REQUIRE_68020
/* Before the constructors: downwellRefuse's library calls go through the
 * global SysBase, which GCCRuntime.cpp's constructor has not set yet. */
extern struct ExecBase* SysBase;
static void __attribute__((noinline)) downwellRefuseProcessor(struct ExecBase* exec)
{
    SysBase = exec;
    downwellRefuse("Downwell needs a 68020 or later.");
}
#endif

/* Audit seam (amiga/workbench_check.sh): the result DOS or Workbench gets. */
void __attribute__((noinline, used)) downwellStartReturn(int result)
{
    __asm__ volatile("" : : "r"(result) : "memory");
}

/* body() on the stack the swap describes, then back on the caller's. The
 * swap moves the stack pointer under the compiler, so this is assembly.
 * StackSwap (LVO -732) and body keep d2/a2, which carry the swap and the
 * result across; the saved registers stay on the caller's stack. */
int downwellCallOnStack(struct StackSwapStruct* swap, int (*body)(void));
__asm__(
    "\t.section .text.downwellCallOnStack,\"ax\",@progbits\n"
    "\t.globl downwellCallOnStack\n"
    "downwellCallOnStack:\n"
    "\tmovem.l %d2/%a2/%a6,-(%sp)\n"
    "\tmove.l 16(%sp),%a2\n"
    "\tmove.l 20(%sp),%d2\n"
    "\tmove.l 4.w,%a6\n"
    "\tmove.l %a2,%a0\n"
    "\tjsr -732(%a6)\n"
    "\tmove.l %d2,%a0\n"
    "\tjsr (%a0)\n"
    "\tmove.l %d0,%d2\n"
    "\tmove.l 4.w,%a6\n"
    "\tmove.l %a2,%a0\n"
    "\tjsr -732(%a6)\n"
    "\tmove.l %d2,%d0\n"
    "\tmovem.l (%sp)+,%d2/%a2/%a6\n"
    "\trts\n"
    "\t.previous\n");

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
#ifdef DOWNWELL_REQUIRE_68020
    /* Everything after this point, constructors included, is 68020 code;
     * this file is built for the 68000 so an older CPU hears why. */
    if (!(SysBase->AttnFlags & AFF_68020)) {
        downwellRefuseProcessor(SysBase);
        downwellStartReturn(20);
        if (gDownwellWorkbenchStartup) {
            Forbid();
            ReplyMsg(&gDownwellWorkbenchStartup->sm_Message);
        }
        return 20;
    }
#endif
    for (DownwellInitFunction* p = __preinit_array_start; p != __preinit_array_end; ++p) (*p)();
    for (DownwellInitFunction* p = __init_array_start; p != __init_array_end; ++p) (*p)();
    const ULONG lower = (ULONG)self->pr_Task.tc_SPLower, upper = (ULONG)self->pr_Task.tc_SPUpper;
    int result;
    if (upper - lower >= downwellStackBytes || SysBase->LibNode.lib_Version < 36) {
        result = main();
    } else {
        UBYTE* const stack = (UBYTE*)AllocMem(downwellStackBytes, MEMF_PUBLIC);
        if (stack) {
            struct StackSwapStruct swap;
            swap.stk_Lower = stack;
            swap.stk_Upper = (ULONG)(stack + downwellStackBytes);
            swap.stk_Pointer = stack + downwellStackBytes;
            result = downwellCallOnStack(&swap, main);
            FreeMem(stack, downwellStackBytes);
        } else {
            downwellRefuse("Not enough free memory: Downwell needs 8 KB for its stack.");
            result = 20;
        }
    }
    for (DownwellInitFunction* p = __fini_array_end; p != __fini_array_start;) (*--p)();
    downwellStartReturn(result);
    if (gDownwellWorkbenchStartup) {
        Forbid();
        ReplyMsg(&gDownwellWorkbenchStartup->sm_Message);
    }
    return result;
}
