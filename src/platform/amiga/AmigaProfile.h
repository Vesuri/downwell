#ifndef DOWNWELL_AMIGA_PROFILE_H
#define DOWNWELL_AMIGA_PROFILE_H
#include "core/Profile.h"
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
// Called with interrupts disabled after restoring our VERTB handler.
void downwellProfileOsFields(unsigned long fields);
#endif
#ifdef DOWNWELL_PROBES
unsigned long downwellAmigaProfileTimestamp();
#endif
#endif
