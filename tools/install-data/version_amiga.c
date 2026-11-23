/* The $VER string. Nothing references it, so --gc-sections keeps it only in a
   SHF_GNU_RETAIN ("R") section; __attribute__((retain)) is ignored on this
   target. (The pinned io_amiga.c's copy is collected away.) */
#include "install_config.h"
__asm__(".section .text.version,\"aR\"\n.asciz \"" INSTALL_VERSION "\"\n.previous");
