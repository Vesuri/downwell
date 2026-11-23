/* The NDK's basic types for the src/platform/amiga headers the prepared-art
   stage reuses (derived_hud.cpp): the NDK's own on the Amiga, the host
   tests' subset (tests/platform/amiga/compat) on the host. */
#ifdef INSTALL_AMIGA
#include_next <exec/types.h>
#elif !defined(DW_EXEC_TYPES_H)
#define DW_EXEC_TYPES_H
#include <stdint.h>
typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef int16_t WORD;
#endif
