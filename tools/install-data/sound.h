/* SOND and AUDO: where each sound's bytes are, and what the tables need. */
#ifndef DW_SOUND_H
#define DW_SOUND_H
#include "win.h"
#define SOUND_MAX 400
typedef struct {
    char name[40], file[40];
    u32 volumeBits;  /* the stored float */
    int group, audioId;
} SoundInfo;
typedef struct {
    int count;
    SoundInfo s[SOUND_MAX];
} Sounds;
int sounds_read(Win* w, Sounds* out);
/* A sound's payload: file offset and size in its container (data.win, an
   audiogroup or a loose file named after the SOND entry). */
typedef struct { const char* container; u32 off, size; } SoundPayload;
typedef struct Context Context;
int sound_payload(Context* c, int id, SoundPayload* out);
/* Frames and sample rate from the payload's header (WAV) or last Ogg page. */
int sound_length(Context* c, int id, u32* frames, u32* rate);
int sound_tables(Context* c);
#endif
