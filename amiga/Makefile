# Downwell — native Amiga 68000 cross-build.

OUT      := out/Downwell
# Strip only the converted Hunk executable; retain the ELF for debugging.
STRIP ?= 0
ifneq ($(STRIP),0)
ifneq ($(STRIP),1)
$(error STRIP must be 0 or 1)
endif
endif
CC       := m68k-amiga-elf-gcc
AS       := m68k-amiga-elf-as
OBJDUMP  := m68k-amiga-elf-objdump
SDKDIR   := $(abspath $(dir $(shell which $(CC)))../m68k-amiga-elf/sys-include)
SUPPORT  := $(abspath $(dir $(shell which $(CC)))../../support)
REPO     := ..
AMIGA_SRC := $(REPO)/src/platform/amiga
REPLAY_VERIFY ?= 0
REPLAY_FRAME_TRACE ?= 1
SCENE_VERIFY ?= 0
SCENE_DESCENT_VERIFY ?= 0
CAMERA_VERIFY ?= 0
FIELD_VERIFY ?= 0
SHOP_VERIFY ?= 0
GEMHIGH_VERIFY ?= 0
LASER_VERIFY ?= 0
RESULTS_RESTART_VERIFY ?= 0
BOB_VERIFY ?= 0
PLANE_VERIFY ?= 0
EXIT_WIPE_VERIFY ?= 0
ENTRY_WIPE_VERIFY ?= 0
CLEAR_VERIFY ?= 0
NATURAL_VERIFY ?= 0
SKIP_INTRO ?= 0
SKIP_TO_GAME ?= 0
REFERENCE_RANDOM ?= 0
REFERENCE_BOX_RANDOM ?= 0
REFERENCE_PATTERNS ?= 0
REFERENCE_BREAKABLES ?= 0
REFERENCE_DAMAGE_TEXT ?= 0
REFERENCE_DAMAGE_BANK ?= 0
REFERENCE_HEALTH_ART ?= 0
REFERENCE_HEALTH_ADMISSION ?= 0
REFERENCE_AMMO_ADMISSION ?= 0
REFERENCE_GEM_POSITION ?= 0
REFERENCE_EFFECT_COMPOSITION ?= 0
REFERENCE_SHADE_PARTITION ?= 0
REFERENCE_CURRENCY_BOBS ?= 0
REFERENCE_STYLE_SPRITES ?= 0
REFERENCE_SELECTION_BINDINGS ?= 0
REFERENCE_SELECTION_BORDER_OWNERSHIP ?= 0
REFERENCE_SELECTION_STRIPS ?= 0
REFERENCE_CLEAR_OPTION_HEIGHT ?= 0
REFERENCE_CLEAR_HEADINGS ?= 0
REFERENCE_CLEAR_MOTION ?= 0
REFERENCE_ACTOR_PREFIX ?= 0
REFERENCE_DRAW_LIST ?= 0
# 1 keeps GameplayCollisionOwner and the registered-family session in the
# normal build: the owned A/B reference for WR.5. 0 ships the compact world.
COLLISION_OWNER ?= 0
# Parked section 0 D candidate (counted destruction results, empty-family
# admission, packed row debts); 0 is the production path.
FRAME_WORK_CANDIDATE ?= 0
REFERENCE_VALIDATION ?= 0
REFERENCE_ANIMATION ?= 0
REFERENCE_GENERATED_BOXES ?= 0
REFERENCE_INITIAL_RASTER ?= 0
REFERENCE_INITIAL_RASTER_STORAGE ?= 0
REFERENCE_ROOM_PREPARE ?= 0
# 1 selects the retained per-span terrain owners, the oracle for the
# source-record owner (see "Source-record terrain owner" in the audit).
REFERENCE_TERRAIN_RECORDS ?= 0
# Parked section 0 C candidate (prepared pattern rows, paint-ahead); 0 is
# the production row painter.
PREPARED_PATTERN_ROWS ?= 0
REFERENCE_FIELD_BANK ?= 0
REFERENCE_FIELD_ADMISSION ?= 0
REFERENCE_ENTRY_ASSETS ?= 0
REFERENCE_EMPTY_TERRAIN ?= 0
REFERENCE_ROOM_INDEX ?= 0
REFERENCE_ROOM_TERRAIN ?= 0
REFERENCE_ROOM_VIEWS ?= 0
REFERENCE_INITIAL_TERRAIN ?= 0
REFERENCE_BOX_ALARMS ?= 0
REFERENCE_HUD_TEXT ?= 0
REFERENCE_HUD_WORDS ?= 0
REFERENCE_HUD_COMPOSITION ?= 0
REFERENCE_HUD_BORDER ?= 0
REFERENCE_HUD_BORDER_EXTENT ?= 0
REFERENCE_CONTACT_MOTION ?= 0
REFERENCE_CONTACT_COORDINATES ?= 0
REFERENCE_ENEMY_ALARM_ROSTER ?= 0
# 1 selects the generic enemy alarm walk, room animation walk, per-setter
# publication and fifths-of-Q8 motion: the oracle for native enemy updates.
REFERENCE_ENEMY_UPDATES ?= 0
TABLE_CONTACT_ATAN ?= 1
RETAIN_HEALTH_HUD ?= 1
AMMO_DAMAGE_RANGE ?= 1
PREPARED_AMMO_DIVIDERS ?= 1
DRAW_CACHE_AUDIT ?= 0
DRAW_ORDER_AUDIT ?= 0
LOCAL_DRAW_CAPTURE ?= 2
AMMO_FACTORED ?= 2
AMMO_FACTORED_VERIFY ?= 0
PREPARED_FIELDS ?= 1
PREPARED_FIELDS_VERIFY ?= 0
PROFILE_FIELD_REFERENCE ?= 0
PREPARED_FIELDS_BACKING ?= 5
SCENERY_LAYERS ?= 1
NATIVE_BOXES ?= 1
NATIVE_FIELDS ?= 1
TERRAIN_BATCH ?= 0
TERRAIN_DIRECT ?= 1
TERRAIN_BATCH_VERIFY ?= 0
SKIP_LEVEL ?= 0
FRAME_DUMP ?= 0
PROBES ?= 0
GENERATION_DETAIL ?= 0
GENERATION_TIMING ?= 0
GENERATION_TIMING_PHASE ?= profileGeneration
PHASE_SAMPLING ?= 0
SAMPLE_FRAME ?= 701
TRACE_FRAMES ?=
FPSCOUNT ?= 0
PROFILE_AUTORUN ?= 0
PROFILE_FRAMES ?= 0
PROFILE_NAVIGATE ?= 0
PROFILE_TRANSITION ?= 0
PROFILE_LASER ?= 0
PROFILE_CONTACT ?= 0
PROFILE_IDLE ?= 0
PROFILE_ENTRY ?= 0
PROFILE_SHOP ?= 0
PROFILE_OVERFLOW ?= 0
OVERFLOW_VERIFY ?= 0
ifeq ($(OVERFLOW_VERIFY),1)
ifneq ($(PROFILE_OVERFLOW),1)
$(error OVERFLOW_VERIFY requires PROFILE_OVERFLOW=1)
endif
endif
ifeq ($(PROFILE_OVERFLOW),1)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(PROFILE_AUTORUN)$(SKIP_TO_GAME)$(SKIP_LEVEL)$(PROFILE_CONTACT)$(PROFILE_ENTRY)$(PROFILE_TRANSITION)$(PROFILE_LASER)$(PROFILE_FRAMES)$(PROFILE_SHOP),110000000)
$(error PROFILE_OVERFLOW requires ordinary autorun with SKIP_TO_GAME=1 and no other tape)
endif
endif
endif
ifneq ($(REFERENCE_FRAME_WORK)$(REFERENCE_ROW_PAINTING),)
$(error REFERENCE_FRAME_WORK and REFERENCE_ROW_PAINTING were renamed: use FRAME_WORK_CANDIDATE=1 and PREPARED_PATTERN_ROWS=1 for the candidates)
endif
# WR.3: -Os by default, the per-update hot paths at -O2. On the A500 the
# quiet fall takes 227 fields this way, 200 at -O3, 211 at -O2 and 261 at
# -Os alone; the executable is less than two-thirds of -O3's.
AMIGA_OPTIMIZATION ?= -Os
ifneq ($(words $(AMIGA_OPTIMIZATION)),1)
$(error AMIGA_OPTIMIZATION must be -O2, -O3 or -Os)
endif
ifneq ($(filter-out -O2 -O3 -Os,$(AMIGA_OPTIMIZATION)),)
$(error AMIGA_OPTIMIZATION must be -O2, -O3 or -Os)
endif
# WR.3: objects compiled at AMIGA_HOT_OPTIMIZATION instead. GCC records the
# level per function, so it survives the LTO link at AMIGA_OPTIMIZATION.
AMIGA_HOT_OPTIMIZATION ?= -O2
AMIGA_HOT_OBJECTS ?= obj/AmigaPortRuntime.o obj/AmigaTerrainRaster.o obj/AmigaTerrainPainter.o \
	obj/AmigaGameplayDrawList.o obj/AmigaGameplayRows.o obj/AmigaGameplayActors.o \
	obj/AmigaGameplayGeneration.o obj/AmigaPaulaAudio.o
ifneq ($(filter-out -O2 -O3 -Os,$(AMIGA_HOT_OPTIMIZATION)),)
$(error AMIGA_HOT_OPTIMIZATION must be -O2, -O3 or -Os)
endif
ifeq ($(PROBES)$(FPSCOUNT),11)
$(error Use PROBES or FPSCOUNT separately)
endif
ifeq ($(PHASE_SAMPLING),1)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(FPSCOUNT)$(PROBES)$(GENERATION_TIMING),100)
$(error PHASE_SAMPLING=1 requires FPSCOUNT=1, PROBES=0 and GENERATION_TIMING=0)
endif
endif
endif
ifeq ($(GENERATION_TIMING),1)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(FPSCOUNT),1)
$(error GENERATION_TIMING=1 requires FPSCOUNT=1)
endif
endif
endif
ifneq ($(TRACE_FRAMES),)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(FPSCOUNT)$(PROBES)$(GENERATION_TIMING)$(PHASE_SAMPLING)$(PROFILE_AUTORUN),10001)
$(error TRACE_FRAMES requires FPSCOUNT=1 PROFILE_AUTORUN=1 without probes, coarse timing or sampling)
endif
endif
endif
ifeq ($(PROFILE_AUTORUN)$(PROBES)$(FPSCOUNT),100)
$(error PROFILE_AUTORUN requires PROBES=1 or FPSCOUNT=1)
endif
ifeq ($(PROFILE_CONTACT),1)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(PROFILE_AUTORUN)$(SKIP_TO_GAME)$(SKIP_LEVEL),110)
$(error PROFILE_CONTACT requires PROFILE_AUTORUN=1 SKIP_TO_GAME=1 SKIP_LEVEL=0)
endif
endif
endif

ifeq ($(PROFILE_SHOP),1)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(PROFILE_AUTORUN)$(SKIP_TO_GAME)$(SKIP_LEVEL)$(PROFILE_CONTACT)$(PROFILE_ENTRY)$(PROFILE_TRANSITION)$(PROFILE_LASER)$(PROFILE_FRAMES),11000000)
$(error PROFILE_SHOP requires ordinary autorun with SKIP_TO_GAME=1 and no other tape)
endif
endif
endif

ifeq ($(PROFILE_ENTRY),1)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(PROFILE_AUTORUN)$(SKIP_INTRO)$(SKIP_TO_GAME)$(SKIP_LEVEL)$(PROFILE_CONTACT)$(PROFILE_TRANSITION),100000)
$(error PROFILE_ENTRY requires autorun without shortcuts, contact or transition mode)
endif
endif
endif

ifeq ($(PROFILE_IDLE),1)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(PROFILE_AUTORUN)$(SKIP_TO_GAME)$(SKIP_LEVEL),110)
$(error PROFILE_IDLE requires PROFILE_AUTORUN=1 SKIP_TO_GAME=1 SKIP_LEVEL=0)
endif
ifneq ($(filter 1,$(PROFILE_CONTACT) $(PROFILE_SHOP) $(PROFILE_ENTRY) $(PROFILE_TRANSITION) $(PROFILE_LASER) $(PROFILE_OVERFLOW) $(PROFILE_NAVIGATE)),)
$(error PROFILE_IDLE cannot be combined with another input tape)
endif
endif
endif

ifneq ($(PROFILE_FRAMES),0)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(PROFILE_AUTORUN)$(SKIP_TO_GAME)$(SKIP_LEVEL)$(PROFILE_CONTACT),1100)
$(error PROFILE_FRAMES requires ordinary gameplay profiling with PROFILE_AUTORUN=1 SKIP_TO_GAME=1)
endif
endif
endif

CFLAGS := -g $(AMIGA_OPTIMIZATION) -flto -m68000 -msoft-float -nostdlib -MP -MMD \
	-fno-tree-loop-distribution -ffunction-sections -fdata-sections \
	-Wall -Wextra -Wpedantic -DNO_ASSEMBLER -DDOWNWELL_PLATFORM_AMIGA
ifeq ($(EXIT_WIPE_VERIFY),1)
CFLAGS += -DDOWNWELL_EXIT_WIPE_VERIFY
endif
ifeq ($(ENTRY_WIPE_VERIFY),1)
ifeq ($(EXIT_WIPE_VERIFY),1)
$(error ENTRY_WIPE_VERIFY and EXIT_WIPE_VERIFY are mutually exclusive)
endif
CFLAGS += -DDOWNWELL_ENTRY_WIPE_VERIFY
endif
ifeq ($(CLEAR_VERIFY),1)
CFLAGS += -DDOWNWELL_CLEAR_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(NATURAL_VERIFY),1)
CFLAGS += -DDOWNWELL_NATURAL_VERIFY -DDOWNWELL_CLEAR_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(REPLAY_VERIFY),1)
CFLAGS += -DDOWNWELL_REPLAY_VERIFY
ifeq ($(REPLAY_FRAME_TRACE),1)
CFLAGS += -DDOWNWELL_REPLAY_FRAME_TRACE
endif
endif
ifeq ($(FRAME_DUMP),1)
CFLAGS += -DDOWNWELL_FRAME_DUMP
endif
ifeq ($(SCENE_VERIFY),1)
CFLAGS += -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(SCENE_DESCENT_VERIFY),1)
CFLAGS += -DDOWNWELL_SCENE_DESCENT_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(CAMERA_VERIFY),1)
CFLAGS += -DDOWNWELL_CAMERA_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(FIELD_VERIFY),1)
CFLAGS += -DDOWNWELL_FIELD_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(SHOP_VERIFY),1)
CFLAGS += -DDOWNWELL_SHOP_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(GEMHIGH_VERIFY),1)
CFLAGS += -DDOWNWELL_GEMHIGH_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(LASER_VERIFY),1)
CFLAGS += -DDOWNWELL_LASER_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(RESULTS_RESTART_VERIFY),1)
CFLAGS += -DDOWNWELL_RESULTS_RESTART_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(BOB_VERIFY),1)
CFLAGS += -DDOWNWELL_BOB_VERIFY -DDOWNWELL_SCENE_VERIFY
endif
ifeq ($(PLANE_VERIFY),1)
CFLAGS += -DDOWNWELL_PLANE_BANK_VERIFY
endif
ifeq ($(REFERENCE_EFFECT_COMPOSITION),1)
CFLAGS += -DDOWNWELL_REFERENCE_EFFECT_COMPOSITION
endif
ifeq ($(REFERENCE_SHADE_PARTITION),1)
CFLAGS += -DDOWNWELL_REFERENCE_SHADE_PARTITION
endif
ifeq ($(REFERENCE_CURRENCY_BOBS),1)
CFLAGS += -DDOWNWELL_REFERENCE_CURRENCY_BOBS
endif
ifeq ($(REFERENCE_STYLE_SPRITES),1)
CFLAGS += -DDOWNWELL_REFERENCE_STYLE_SPRITES
endif
ifeq ($(REFERENCE_SELECTION_BINDINGS),1)
CFLAGS += -DDOWNWELL_REFERENCE_SELECTION_BINDINGS
endif
ifeq ($(REFERENCE_SELECTION_BORDER_OWNERSHIP),1)
CFLAGS += -DDOWNWELL_REFERENCE_SELECTION_BORDER_OWNERSHIP
endif
ifeq ($(REFERENCE_SELECTION_STRIPS),1)
CFLAGS += -DDOWNWELL_REFERENCE_SELECTION_STRIPS
endif
ifeq ($(REFERENCE_CLEAR_OPTION_HEIGHT),1)
CFLAGS += -DDOWNWELL_REFERENCE_CLEAR_OPTION_HEIGHT
endif
ifeq ($(REFERENCE_CLEAR_HEADINGS),1)
CFLAGS += -DDOWNWELL_REFERENCE_CLEAR_HEADINGS
endif
ifeq ($(REFERENCE_CLEAR_MOTION),1)
CFLAGS += -DDOWNWELL_REFERENCE_CLEAR_MOTION
endif
ifeq ($(SKIP_INTRO),1)
CFLAGS += -DDOWNWELL_SKIP_INTRO
endif
ifeq ($(REFERENCE_RANDOM),1)
CFLAGS += -DDOWNWELL_REFERENCE_RANDOM
endif
ifeq ($(REFERENCE_BOX_RANDOM),1)
CFLAGS += -DDOWNWELL_REFERENCE_BOX_RANDOM
endif
ifeq ($(REFERENCE_PATTERNS),1)
CFLAGS += -DDOWNWELL_REFERENCE_PATTERNS
endif
ifeq ($(REFERENCE_BREAKABLES),1)
CFLAGS += -DDOWNWELL_REFERENCE_BREAKABLES
endif
ifeq ($(REFERENCE_DAMAGE_TEXT),1)
CFLAGS += -DDOWNWELL_REFERENCE_DAMAGE_TEXT
endif
ifeq ($(REFERENCE_DAMAGE_BANK),1)
CFLAGS += -DDOWNWELL_REFERENCE_DAMAGE_BANK
endif
ifeq ($(REFERENCE_HEALTH_ART),1)
CFLAGS += -DDOWNWELL_REFERENCE_HEALTH_ART
endif
ifeq ($(REFERENCE_HEALTH_ADMISSION),1)
CFLAGS += -DDOWNWELL_REFERENCE_HEALTH_ADMISSION
endif
ifeq ($(REFERENCE_AMMO_ADMISSION),1)
CFLAGS += -DDOWNWELL_REFERENCE_AMMO_ADMISSION
endif
ifeq ($(REFERENCE_GEM_POSITION),1)
CFLAGS += -DDOWNWELL_REFERENCE_GEM_POSITION
endif
ifeq ($(REFERENCE_ACTOR_PREFIX),1)
CFLAGS += -DDOWNWELL_REFERENCE_ACTOR_PREFIX
endif
ifeq ($(REFERENCE_DRAW_LIST),1)
CFLAGS += -DDOWNWELL_REFERENCE_DRAW_LIST
endif
ifeq ($(FRAME_WORK_CANDIDATE),1)
CFLAGS += -DDOWNWELL_FRAME_WORK_CANDIDATE
endif
ifeq ($(COLLISION_OWNER),1)
CFLAGS += -DDOWNWELL_COLLISION_OWNER
endif
ifeq ($(REFERENCE_VALIDATION),1)
CFLAGS += -DDOWNWELL_REFERENCE_VALIDATION
endif
ifeq ($(REFERENCE_ANIMATION),1)
CFLAGS += -DDOWNWELL_REFERENCE_ANIMATION
endif
ifeq ($(REFERENCE_GENERATED_BOXES),1)
CFLAGS += -DDOWNWELL_REFERENCE_GENERATED_BOXES
endif
ifeq ($(REFERENCE_INITIAL_RASTER),1)
CFLAGS += -DDOWNWELL_REFERENCE_INITIAL_RASTER
endif
ifeq ($(REFERENCE_INITIAL_RASTER_STORAGE),1)
CFLAGS += -DDOWNWELL_REFERENCE_INITIAL_RASTER_STORAGE
endif
ifeq ($(REFERENCE_ROOM_PREPARE),1)
CFLAGS += -DDOWNWELL_REFERENCE_ROOM_PREPARE
endif
ifeq ($(REFERENCE_FIELD_BANK),1)
CFLAGS += -DDOWNWELL_REFERENCE_FIELD_BANK
endif
ifeq ($(REFERENCE_FIELD_ADMISSION),1)
CFLAGS += -DDOWNWELL_REFERENCE_FIELD_ADMISSION
endif
ifeq ($(REFERENCE_ENTRY_ASSETS),1)
CFLAGS += -DDOWNWELL_REFERENCE_ENTRY_ASSETS
endif
ifeq ($(REFERENCE_EMPTY_TERRAIN),1)
CFLAGS += -DDOWNWELL_REFERENCE_EMPTY_TERRAIN
endif
ifeq ($(REFERENCE_ROOM_INDEX),1)
CFLAGS += -DDOWNWELL_REFERENCE_ROOM_INDEX
endif
ifeq ($(REFERENCE_ROOM_TERRAIN),1)
CFLAGS += -DDOWNWELL_REFERENCE_ROOM_TERRAIN
endif
ifeq ($(REFERENCE_ROOM_VIEWS),1)
CFLAGS += -DDOWNWELL_REFERENCE_ROOM_VIEWS
endif
ifeq ($(REFERENCE_INITIAL_TERRAIN),1)
CFLAGS += -DDOWNWELL_REFERENCE_INITIAL_TERRAIN
endif
ifeq ($(REFERENCE_BOX_ALARMS),1)
CFLAGS += -DDOWNWELL_REFERENCE_BOX_ALARMS
endif
ifeq ($(REFERENCE_HUD_TEXT),1)
CFLAGS += -DDOWNWELL_REFERENCE_HUD_TEXT
endif
ifeq ($(REFERENCE_HUD_WORDS),1)
CFLAGS += -DDOWNWELL_REFERENCE_HUD_WORDS
endif
ifeq ($(REFERENCE_HUD_COMPOSITION),1)
CFLAGS += -DDOWNWELL_REFERENCE_HUD_COMPOSITION
endif
ifeq ($(REFERENCE_HUD_BORDER),1)
CFLAGS += -DDOWNWELL_REFERENCE_HUD_BORDER
endif
ifeq ($(REFERENCE_HUD_BORDER_EXTENT),1)
CFLAGS += -DDOWNWELL_REFERENCE_HUD_BORDER_EXTENT
endif
ifeq ($(REFERENCE_CONTACT_MOTION),1)
CFLAGS += -DDOWNWELL_REFERENCE_CONTACT_MOTION
endif
ifeq ($(REFERENCE_CONTACT_COORDINATES),1)
CFLAGS += -DDOWNWELL_REFERENCE_CONTACT_COORDINATES
endif
ifeq ($(REFERENCE_ENEMY_ALARM_ROSTER),1)
CFLAGS += -DDOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
endif
ifeq ($(REFERENCE_ENEMY_UPDATES),1)
CFLAGS += -DDOWNWELL_REFERENCE_ENEMY_UPDATES
endif
ifeq ($(PREPARED_PATTERN_ROWS),1)
CFLAGS += -DDOWNWELL_PREPARED_PATTERN_ROWS
endif
ifeq ($(TABLE_CONTACT_ATAN),0)
CFLAGS += -DDOWNWELL_PORTABLE_CONTACT_ATAN
endif
ifeq ($(RETAIN_HEALTH_HUD),1)
CFLAGS += -DDOWNWELL_RETAIN_HEALTH_HUD
endif
ifeq ($(AMMO_DAMAGE_RANGE),1)
CFLAGS += -DDOWNWELL_AMMO_DAMAGE_RANGE
endif
ifeq ($(PREPARED_AMMO_DIVIDERS),1)
CFLAGS += -DDOWNWELL_PREPARED_AMMO_DIVIDERS
endif
ifeq ($(DRAW_CACHE_AUDIT),1)
ifeq ($(filter 1,$(FPSCOUNT) $(PROBES)),)
$(error DRAW_CACHE_AUDIT requires FPSCOUNT=1 or PROBES=1)
endif
CFLAGS += -DDOWNWELL_DRAW_CACHE_AUDIT
endif
ifeq ($(DRAW_ORDER_AUDIT),1)
ifeq ($(filter 1,$(FPSCOUNT) $(PROBES)),)
$(error DRAW_ORDER_AUDIT requires FPSCOUNT=1 or PROBES=1)
endif
CFLAGS += -DDOWNWELL_DRAW_ORDER_AUDIT
endif
ifneq ($(LOCAL_DRAW_CAPTURE),0)
CFLAGS += -DDOWNWELL_LOCAL_DRAW_CAPTURE=$(LOCAL_DRAW_CAPTURE)
endif
ifneq ($(AMMO_FACTORED),0)
CFLAGS += -DDOWNWELL_AMMO_FACTORED=$(AMMO_FACTORED)
endif
ifeq ($(AMMO_FACTORED_VERIFY),1)
ifneq ($(AMMO_FACTORED),2)
$(error AMMO_FACTORED_VERIFY requires AMMO_FACTORED=2)
endif
ifeq ($(filter 1,$(FPSCOUNT) $(PROBES)),)
$(error AMMO_FACTORED_VERIFY requires FPSCOUNT=1 or PROBES=1)
endif
CFLAGS += -DDOWNWELL_AMMO_FACTORED_VERIFY
endif
ifeq ($(PREPARED_FIELDS),1)
CFLAGS += -DDOWNWELL_PREPARED_FIELDS
ifeq ($(PROFILE_FIELD_REFERENCE),1)
CFLAGS += -DDOWNWELL_FIELD_MODE=2
else ifeq ($(PREPARED_FIELDS_BACKING),5)
CFLAGS += -DDOWNWELL_FIELD_MODE=7
else ifeq ($(PREPARED_FIELDS_BACKING),4)
CFLAGS += -DDOWNWELL_FIELD_MODE=6
else ifeq ($(PREPARED_FIELDS_BACKING),3)
CFLAGS += -DDOWNWELL_FIELD_MODE=5
else ifeq ($(PREPARED_FIELDS_BACKING),2)
CFLAGS += -DDOWNWELL_FIELD_MODE=4
else ifeq ($(PREPARED_FIELDS_BACKING),1)
CFLAGS += -DDOWNWELL_FIELD_MODE=3
else
CFLAGS += -DDOWNWELL_FIELD_MODE=1
endif
endif
ifeq ($(PREPARED_FIELDS_VERIFY),1)
CFLAGS += -DDOWNWELL_PREPARED_FIELDS_VERIFY
endif
ifeq ($(SCENERY_LAYERS),1)
CFLAGS += -DDOWNWELL_SCENERY_LAYERS
endif
CFLAGS += -DDOWNWELL_NATIVE_BOXES=$(NATIVE_BOXES)
CFLAGS += -DDOWNWELL_NATIVE_FIELDS=$(NATIVE_FIELDS)
ifneq ($(filter 1,$(TERRAIN_BATCH) $(TERRAIN_BATCH_VERIFY) $(TERRAIN_DIRECT)),)
CFLAGS += -DDOWNWELL_TERRAIN_BATCH
endif
ifeq ($(TERRAIN_DIRECT),1)
CFLAGS += -DDOWNWELL_TERRAIN_DIRECT
endif
ifeq ($(TERRAIN_BATCH_VERIFY),1)
CFLAGS += -DDOWNWELL_TERRAIN_BATCH_VERIFY
endif
ifneq ($(filter 1,$(SKIP_TO_GAME) $(SKIP_LEVEL)),)
CFLAGS += -DDOWNWELL_SKIP_TO_GAME
endif
ifeq ($(SKIP_LEVEL),1)
CFLAGS += -DDOWNWELL_SKIP_LEVEL
endif
ifeq ($(PROBES),1)
CFLAGS += -DDOWNWELL_PROBES
ifeq ($(GENERATION_DETAIL),1)
CFLAGS += -DDOWNWELL_GENERATION_DETAIL
endif
endif
ifeq ($(GENERATION_TIMING),1)
CFLAGS += -DDOWNWELL_GENERATION_TIMING -DDOWNWELL_GENERATION_TIMING_PHASE=$(GENERATION_TIMING_PHASE)
endif
ifeq ($(FPSCOUNT),1)
CFLAGS += -DDOWNWELL_FPSCOUNT
endif
ifeq ($(PHASE_SAMPLING),1)
CFLAGS += -DDOWNWELL_PHASE_SAMPLING -DDOWNWELL_SAMPLE_FRAME=$(SAMPLE_FRAME)
endif
ifneq ($(TRACE_FRAMES),)
CFLAGS += -DDOWNWELL_TRACE_FRAMES=$(TRACE_FRAMES)
endif
ifeq ($(TERRAIN_READ_CENSUS),1)
ifneq ($(FPSCOUNT),1)
$(error TERRAIN_READ_CENSUS requires FPSCOUNT=1)
endif
ifneq ($(TRACE_FRAMES),165)
$(error TERRAIN_READ_CENSUS requires TRACE_FRAMES=165)
endif
CFLAGS += -DDOWNWELL_TERRAIN_READ_CENSUS
endif
ifeq ($(PROFILE_AUTORUN),1)
CFLAGS += -DDOWNWELL_PROFILE_AUTORUN
endif
ifneq ($(PROFILE_FRAMES),0)
CFLAGS += -DDOWNWELL_PROFILE_FRAMES=$(PROFILE_FRAMES)
endif
ifeq ($(PROFILE_CONTACT),1)
CFLAGS += -DDOWNWELL_PROFILE_CONTACT
endif
ifeq ($(PROFILE_IDLE),1)
CFLAGS += -DDOWNWELL_PROFILE_IDLE
endif
ifeq ($(PROFILE_SHOP),1)
CFLAGS += -DDOWNWELL_PROFILE_SHOP
endif
ifeq ($(PROFILE_OVERFLOW),1)
CFLAGS += -DDOWNWELL_PROFILE_OVERFLOW
endif
ifeq ($(OVERFLOW_VERIFY),1)
CFLAGS += -DDOWNWELL_OVERFLOW_VERIFY
endif
ifeq ($(PROFILE_ENTRY),1)
CFLAGS += -DDOWNWELL_PROFILE_ENTRY
endif
ifeq ($(PROFILE_NAVIGATE),1)
CFLAGS += -DDOWNWELL_PROFILE_NAVIGATE
endif
ifeq ($(PROFILE_TRANSITION),1)
CFLAGS += -DDOWNWELL_PROFILE_TRANSITION
endif
ifneq ($(filter 1,$(PROBES) $(FPSCOUNT)),)
ifeq ($(PROFILE_LASER),1)
CFLAGS += -DDOWNWELL_PROFILE_LASER
endif
ifneq ($(filter 1,$(FIELD_VERIFY) $(SCENE_VERIFY) $(SCENE_DESCENT_VERIFY) $(CAMERA_VERIFY) $(SHOP_VERIFY) $(REPLAY_VERIFY) $(BOB_VERIFY) $(RESULTS_RESTART_VERIFY) $(GEMHIGH_VERIFY) $(LASER_VERIFY)),)
$(error Profiling must not be combined with verification instrumentation)
endif
endif
# This is the scene ownership contract, not an optional optimization mode.
# Match DownwellAmiga's reference-owner selection. Pixel/reference fixtures
# retain generic collision services; shipping, profiling and native reentry
# fixtures use only their declared external collision backend.
ifeq ($(filter -DDOWNWELL_FIELD_VERIFY -DDOWNWELL_ENTRY_WIPE_VERIFY,$(CFLAGS)),)
ifeq ($(findstring -DDOWNWELL_SCENE_VERIFY,$(CFLAGS)),)
CFLAGS += -DDOWNWELL_EXTERNAL_COLLISION_BACKEND
else ifneq ($(findstring -DDOWNWELL_RESULTS_RESTART_VERIFY,$(CFLAGS)),)
CFLAGS += -DDOWNWELL_EXTERNAL_COLLISION_BACKEND
endif
endif
# Source terrain records are bound only by GameplayCollisionOwner: match
# DownwellAmiga's DOWNWELL_GAMEPLAY_COLLISION_OWNER selection. Without the
# owner they stay empty (266 KB of BSS on the world-one route).
ifeq ($(REFERENCE_TERRAIN_RECORDS),0)
ifneq ($(filter -DDOWNWELL_SCENE_COLLISION_BACKEND -DDOWNWELL_SCENE_VERIFY -DDOWNWELL_FIELD_VERIFY \
	-DDOWNWELL_ENTRY_WIPE_VERIFY -DDOWNWELL_REPLAY_VERIFY -DDOWNWELL_COLLISION_OWNER,$(CFLAGS)),)
CFLAGS += -DDOWNWELL_TERRAIN_SOURCE_RECORDS
endif
endif
CPPFLAGS := $(CFLAGS) -x c++ -std=gnu++17 -fno-rtti -fno-exceptions \
	-fno-use-cxa-atexit -DDOWNWELL_FREESTANDING \
	-I$(AMIGA_SRC)/compat -I$(REPO)/src
ASFLAGS := -mcpu=68000 -g --register-prefix-optional -I"$(SDKDIR)" -I"$(CURDIR)"
ifeq ($(ENTRY_WIPE_VERIFY),1)
ASFLAGS += --defsym DOWNWELL_ENTRY_WIPE_VERIFY=1
endif
ifeq ($(EXIT_WIPE_VERIFY),1)
ASFLAGS += --defsym DOWNWELL_EXIT_WIPE_VERIFY=1
endif
ifneq ($(findstring -DDOWNWELL_SCENE_VERIFY,$(CFLAGS)),)
ifeq ($(findstring -DDOWNWELL_RESULTS_RESTART_VERIFY,$(CFLAGS)),)
ASFLAGS += --defsym DOWNWELL_REFERENCE_COLLISION_MASKS=1
REFERENCE_COLLISION_MASK_INPUT := $(REPO)/generated/downwell-sprite-masks.dwsm
endif
endif
# Pixel verifiers retain linked source bytes as an independent load oracle.
# Shipping/profiling executables must never link these transient scene banks.
ifneq ($(strip $(findstring VERIFY,$(CFLAGS)) $(filter 1,$(REFERENCE_ENTRY_ASSETS))),)
CPPFLAGS += -DDOWNWELL_REFERENCE_SCENE_BANKS
ASFLAGS += --defsym DOWNWELL_REFERENCE_SCENE_BANKS=1
endif
LDFLAGS := -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map
ifeq ($(TERRAIN_READ_CENSUS),1)
LDFLAGS += -Wl,--undefined=gDownwellTerrainReadSamples -Wl,--undefined=gDownwellTerrainReadFrames
LDFLAGS += -Wl,--undefined=gDownwellTerrainReadActive -Wl,--undefined=gDownwellTerrainConstructionDepth
endif

AMIGA_HUD := $(REPO)/generated/amiga-hud.dwap
AMIGA_GAME := $(REPO)/generated/amiga-game.dwap
AMIGA_EFFECT_STAMPS := $(REPO)/generated/amiga-effect-stamps.dwef
AMIGA_EXIT_MASK := $(REPO)/generated/amiga-game.exit-mask
AMIGA_ENTRY_MASK := $(REPO)/generated/amiga-game.entry-mask
AMIGA_SELECTION_MASK := $(REPO)/generated/amiga-game.selection-mask
# Scene banks are files beside the executable, read whole per scene.
AMIGA_BOOT_BANK := $(REPO)/generated/amiga-boot.dwap
AMIGA_FRONT_BANK := $(REPO)/generated/amiga-front.dwap
AMIGA_SCENE_BANKS := $(REPO)/generated/amiga-scene-banks.h
AMIGA_RESULTS := $(REPO)/generated/amiga-results.dwap
AMIGA_SHOP := $(REPO)/generated/amiga-shop.dwap
AMIGA_PAUSE := $(REPO)/generated/amiga-pause.dwap
AMIGA_CLEAR := $(REPO)/generated/amiga-clear.dwap
ROOM_PACK := $(REPO)/generated/downwell-runtime-rooms.dwrm
OBJECT_PACK := $(REPO)/generated/downwell-objects.dwob
COLLISION_PACK := $(REPO)/generated/downwell-sprites.dwsc
COLLISION_MASK_PACK := $(REPO)/generated/downwell-sprite-masks.dwsm
BUILDER_PACK := $(REPO)/generated/downwell-builder-patterns.dwbp
OBJECTS := obj/DownwellAmiga.o obj/AmigaProfile.o obj/AmigaInput.o obj/AmigaKeyboard.o obj/AmigaPaulaAudio.o obj/AmigaSoundBanks.o obj/AmigaPlayer.o \
	obj/AmigaPortRuntime.o obj/AmigaSceneRuntime.o \
	obj/DownwellCopperLists.o \
	obj/AmigaTerrain.o obj/AmigaGameplayGeneration.o obj/AmigaTerrainRaster.o obj/AmigaTerrainPainter.o obj/AmigaMenu.o obj/AmigaMenuScroll.o \
	obj/AmigaMenuCache.o obj/AmigaGardenScene.o obj/AmigaSplashCache.o obj/AmigaPublisherScene.o obj/AmigaCreditsScene.o obj/AmigaControlsScene.o obj/AmigaStyleRenderer.o obj/AmigaStyleScene.o obj/AmigaUpgradeScene.o obj/AmigaSelectionLayer.o \
	obj/AmigaRenderer.o obj/AmigaBlitter.o obj/AmigaGameplayRows.o obj/AmigaGameplayEffects.o obj/AmigaEffectSurface.o obj/AmigaGameplayScene.o obj/AmigaGameplayActors.o obj/AmigaGameplaySpriteAssignment.o obj/AmigaGameplayDrawList.o \
	obj/AmigaHardware.o obj/Bitmap.o obj/Palette.o obj/Sprite.o obj/Util.o \
	obj/GCCRuntime.o obj/AmigaMemoryCopy.o obj/AmigaPlatform.o \
	obj/AmigaMemory.o obj/CopperList.o obj/RoomPack.o obj/ObjectPack.o \
	obj/BuilderPatternPack.o obj/AreaOneBuilder.o obj/GameMakerRandom.o \
	obj/Replay.o \
	obj/AmigaAssets.o \
	obj/gcc8_c_support.o obj/gcc8_a_support.o
ifeq ($(TERRAIN_DIRECT),1)
OBJECTS += obj/AmigaTerrainPrepared.o
endif
OBJECTS += obj/AmigaPatternTransfer.o
ifeq ($(PREPARED_FIELDS),1)
OBJECTS += obj/AmigaFieldPrepared.o
endif

.PHONY: all check clean audit

all: check $(OUT).exe $(OUT).boot $(OUT).front $(OUT).game $(OUT).planes $(OUT).cavern $(OUT).cavernplanes $(OUT).results $(OUT).shop $(OUT).pause $(OUT).clear $(OUT).fields $(OUT).columns $(OUT).patterns $(OUT).exitmask $(OUT).selectionmask $(OUT).hud $(OUT).entrymask $(OUT).terrain $(OUT).encountermasks $(OUT).sndrun $(OUT).sndfront $(OUT).sndresults $(OUT).sndclear

$(OUT).entrymask: $(AMIGA_ENTRY_MASK)
	@mkdir -p $(@D)
	cp $< $@

$(OUT).hud: $(AMIGA_HUD)
	@mkdir -p $(@D)
	cp $< $@

$(OUT).exitmask: $(AMIGA_EXIT_MASK)
	@mkdir -p $(@D)
	cp $< $@

$(OUT).selectionmask: $(AMIGA_SELECTION_MASK)
	@mkdir -p $(@D)
	cp $< $@

$(OUT).patterns: $(REPO)/generated/downwell-builder-patterns.dwbp
	@mkdir -p $(@D)
	cp $< $@

$(OUT).terrain: $(REPO)/generated/amiga-pattern-terrain.bin
	cp $< $@

$(REPO)/generated/amiga-pattern-terrain.bin: $(AMIGA_GAME) $(REPO)/generated/downwell-builder-patterns.dwbp $(REPO)/tools/generate-pattern-terrain.py
	python3 $(REPO)/tools/generate-pattern-terrain.py

$(OUT).columns: $(REPO)/generated/amiga-field-columns.bin
	@mkdir -p $(@D)
	cp -f $< $@

$(REPO)/generated/amiga-field-columns.bin: $(REPO)/generated/downwell-field-columns.h
	$(MAKE) -C $(REPO) generated/amiga-field-columns.bin

$(OUT).fields: $(REPO)/generated/amiga-field-bands.bin
	@mkdir -p $(@D)
	cp -f $< $@

$(REPO)/generated/amiga-field-bands.bin: $(REPO)/generated/downwell-field-columns.h
	$(MAKE) -C $(REPO) generated/amiga-field-bands.bin

$(OUT).cavern: $(REPO)/generated/amiga-cavern.dwap
	@mkdir -p $(@D)
	cp -f $< $@

$(OUT).cavernplanes: $(REPO)/generated/amiga-cavern.planes
	@mkdir -p $(@D)
	cp -f $< $@

$(REPO)/generated/amiga-cavern.planes: $(REPO)/generated/amiga-game.planes
	@test -f $@ || python3 $(REPO)/tools/generate-amiga-plane-bank.py

$(REPO)/generated/amiga-cavern-planes.h: $(REPO)/generated/amiga-cavern.planes
	@test -f $@ || python3 $(REPO)/tools/generate-amiga-plane-bank.py

obj/DownwellAmiga.o: $(REPO)/generated/amiga-cavern-planes.h

$(REPO)/generated/downwell-area-titles.h: $(REPO)/tools/generate-area-titles.py $(REPO)/data/language.ini
	python3 $(REPO)/tools/generate-area-titles.py --output $@

obj/DownwellAmiga.o: $(REPO)/generated/downwell-area-titles.h

# W1.8 merchant speech strings (the same tool writes the full shop table).
$(REPO)/generated/downwell-shop-speech.h: $(REPO)/tools/generate-shop-text.py $(REPO)/data/language.ini
	python3 $(REPO)/tools/generate-shop-text.py

obj/DownwellAmiga.o: $(REPO)/generated/downwell-shop-speech.h

$(OUT).game: $(AMIGA_GAME)
	@mkdir -p $(@D)
	cp -f $< $@

$(REPO)/generated/amiga-transient-banks.h: $(AMIGA_RESULTS) $(AMIGA_SHOP) $(AMIGA_PAUSE) $(AMIGA_CLEAR) $(AMIGA_HUD) $(AMIGA_EXIT_MASK) $(AMIGA_ENTRY_MASK) $(AMIGA_SELECTION_MASK) $(REPO)/generated/downwell-builder-patterns.dwbp $(REPO)/generated/amiga-pattern-terrain.bin $(REPO)/tools/generate-amiga-transient-banks.py
	python3 $(REPO)/tools/generate-amiga-transient-banks.py

obj/DownwellAmiga.o: $(REPO)/generated/amiga-transient-banks.h

$(OUT).results: $(AMIGA_RESULTS)
	@mkdir -p $(@D)
	cp -f $< $@

$(OUT).shop: $(AMIGA_SHOP)
	@mkdir -p $(@D)
	cp -f $< $@

$(OUT).pause: $(AMIGA_PAUSE)
	@mkdir -p $(@D)
	cp -f $< $@

$(OUT).clear: $(AMIGA_CLEAR)
	@mkdir -p $(@D)
	cp -f $< $@

$(REPO)/generated/amiga-game.planes: $(AMIGA_GAME) $(REPO)/generated/amiga-cavern.dwap $(REPO)/tools/generate-amiga-plane-bank.py $(REPO)/tools/amiga_scene_contract.py
	python3 $(REPO)/tools/generate-amiga-plane-bank.py

$(REPO)/generated/amiga-game-planes.h: $(REPO)/generated/amiga-game.planes
	@test -f $@ || python3 $(REPO)/tools/generate-amiga-plane-bank.py

$(OUT).planes: $(REPO)/generated/amiga-game.planes
	@mkdir -p $(@D)
	cp -f $< $@

$(OUT).boot: $(AMIGA_BOOT_BANK)
	@mkdir -p $(@D)
	cp -f $< $@

$(OUT).front: $(AMIGA_FRONT_BANK)
	@mkdir -p $(@D)
	cp -f $< $@

.PHONY: FORCE_STARTUP_OPTIONS
obj/startup-options: FORCE_STARTUP_OPTIONS
	@mkdir -p obj
	@printf '%s\n' '$(SKIP_INTRO)' '$(SKIP_TO_GAME)' '$(SKIP_LEVEL)' '$(TABLE_CONTACT_ATAN)' '$(RETAIN_HEALTH_HUD)' '$(AMMO_DAMAGE_RANGE)' '$(PREPARED_AMMO_DIVIDERS)' '$(PREPARED_FIELDS)' '$(PREPARED_FIELDS_VERIFY)' '$(PROFILE_FIELD_REFERENCE)' '$(PREPARED_FIELDS_BACKING)' '$(SCENERY_LAYERS)' '$(NATIVE_BOXES)' '$(NATIVE_FIELDS)' '$(TERRAIN_BATCH)' '$(TERRAIN_DIRECT)' '$(TERRAIN_BATCH_VERIFY)' '$(AMIGA_OPTIMIZATION)' '$(AMIGA_HOT_OPTIMIZATION) $(AMIGA_HOT_OBJECTS)' '$(LASER_VERIFY)' '$(PROFILE_LASER)' > $@.tmp
	@printf '%s\n' '$(DRAW_CACHE_AUDIT)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_DRAW_LIST)' >> $@.tmp
	@printf '%s\n' '$(FRAME_WORK_CANDIDATE)' >> $@.tmp
	@printf '%s\n' '$(COLLISION_OWNER)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_VALIDATION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ANIMATION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_GENERATED_BOXES)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_BOX_RANDOM)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_STYLE_SPRITES)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_SELECTION_BINDINGS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_SELECTION_BORDER_OWNERSHIP)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_SELECTION_STRIPS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_CLEAR_OPTION_HEIGHT)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_CLEAR_HEADINGS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_CLEAR_MOTION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_INITIAL_RASTER)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_INITIAL_RASTER_STORAGE)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ROOM_PREPARE)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_TERRAIN_RECORDS)' >> $@.tmp
	@printf '%s\n' '$(PREPARED_PATTERN_ROWS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_FIELD_BANK)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_FIELD_ADMISSION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ENTRY_ASSETS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_EMPTY_TERRAIN)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ROOM_INDEX)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ROOM_TERRAIN)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ROOM_VIEWS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_INITIAL_TERRAIN)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_BOX_ALARMS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_HUD_TEXT)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_HUD_WORDS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_HUD_COMPOSITION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_DAMAGE_BANK)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_HEALTH_ART)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_HEALTH_ADMISSION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_AMMO_ADMISSION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_SHADE_PARTITION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_CURRENCY_BOBS)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_HUD_BORDER)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_HUD_BORDER_EXTENT)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_CONTACT_MOTION)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_CONTACT_COORDINATES)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ENEMY_ALARM_ROSTER)' >> $@.tmp
	@printf '%s\n' '$(REFERENCE_ENEMY_UPDATES)' >> $@.tmp
	@printf '%s\n' '$(DRAW_ORDER_AUDIT)' >> $@.tmp
	@printf '%s\n' '$(LOCAL_DRAW_CAPTURE)' >> $@.tmp
	@printf '%s\n' '$(AMMO_FACTORED)' >> $@.tmp
	@printf '%s\n' '$(AMMO_FACTORED_VERIFY)' >> $@.tmp
	@printf '%s\n' '$(FRAME_DUMP)' >> $@.tmp
	@printf '%s\n' '$(PROFILE_SHOP)' >> $@.tmp
	@printf '%s\n' '$(PROFILE_OVERFLOW)' >> $@.tmp
	@printf '%s\n' '$(OVERFLOW_VERIFY)' >> $@.tmp
	@printf '%s\n' '$(findstring VERIFY,$(CFLAGS))' >> $@.tmp
	@printf '%s\n' '$(CFLAGS)' '$(CPPFLAGS)' '$(ASFLAGS)' >> $@.tmp
	@cmp -s $@.tmp $@ || cp $@.tmp $@

$(OBJECTS): obj/startup-options
ifeq ($(OVERFLOW_VERIFY),1)
obj/DownwellAmiga.o: $(REPO)/generated/overflow-witness.h
$(REPO)/generated/overflow-witness.h:
	$(MAKE) -C $(REPO) generate-overflow-witness
endif
ifeq ($(PROFILE_SHOP),1)
obj/DownwellAmiga.o: $(REPO)/generated/shop-profile-input.h
$(REPO)/generated/shop-profile-input.h:
	$(MAKE) -C $(REPO) generate-shop-profile
endif
ifeq ($(PREPARED_AMMO_DIVIDERS),1)
$(OBJECTS): $(REPO)/generated/amiga-ammo-dividers.h
$(REPO)/generated/amiga-ammo-dividers.h: $(REPO)/tools/generate-ammo-dividers.py $(AMIGA_HUD)
	$(MAKE) -C $(REPO) generated/amiga-ammo-dividers.h
ifneq ($(AMMO_FACTORED),0)
obj/DownwellAmiga.o: $(REPO)/generated/amiga-ammo-factored.h
$(REPO)/generated/amiga-ammo-factored.h: $(REPO)/tools/probe-ammo-complete.py $(AMIGA_HUD)
	$(MAKE) -C $(REPO) generated/amiga-ammo-factored.h
endif
endif
$(OBJECTS): $(REPO)/generated/downwell-field-spans.h
$(REPO)/generated/downwell-field-spans.h: $(REPO)/tools/generate-field-spans.cpp $(wildcard $(REPO)/src/core/*.h)
	$(MAKE) -C $(REPO) generated/downwell-field-spans.h
$(OBJECTS): $(REPO)/generated/downwell-goal-mask.h
$(OBJECTS): $(REPO)/generated/downwell-time-field-mask.h
$(REPO)/generated/downwell-time-field-mask.h: $(REPO)/tools/export-time-stop-masks.csx $(REPO)/tools/verify-time-stop-masks.py $(REPO)/data/data.win
	$(MAKE) -C $(REPO) generated/downwell-time-field-mask.h
$(OBJECTS): $(REPO)/generated/contact-motion.inc
$(OBJECTS): $(REPO)/generated/clear-motion.inc
$(REPO)/generated/clear-motion.inc: $(REPO)/tools/generate-clear-motion.cpp $(REPO)/tools/generate-clear-motion.py $(REPO)/src/core/LevelClearMotion.h $(REPO)/src/core/HudReal.h $(REPO)/data/data.win $(REPO)/data/Downwell.exe $(REPO)/extracted/utmt-0.9.2.0/manifest.json
	$(MAKE) -C $(REPO) generated/clear-motion.inc
$(OBJECTS): $(REPO)/generated/amiga-room-terrain-plan.inc
$(REPO)/generated/amiga-room-terrain-plan.inc: $(REPO)/tools/generate-room-terrain-plan.py $(REPO)/extracted/utmt-0.9.2.0/manifest.json
	$(MAKE) -C $(REPO) generated/amiga-room-terrain-plan.inc
$(REPO)/generated/contact-motion.inc: $(REPO)/tools/generate-contact-motion.cpp $(REPO)/tools/generate-contact-motion.py $(REPO)/src/core/ContactMotionCodec.h $(REPO)/generated/smoke-trig.inc $(REPO)/generated/atan-q16.inc
	$(MAKE) -C $(REPO) generated/contact-motion.inc

$(OBJECTS): $(REPO)/generated/smoke-trig.inc
$(OBJECTS): $(REPO)/generated/sound-durations.inc $(REPO)/generated/sound-gains.inc
$(REPO)/generated/sound-durations.inc $(REPO)/generated/sound-gains.inc: $(REPO)/tools/sound-durations.py
	$(MAKE) -C $(REPO) generated/sound-durations.inc
$(OBJECTS): $(REPO)/generated/atan-q16.inc
$(REPO)/generated/atan-q16.inc: $(REPO)/tools/generate-atan-table.py
	$(MAKE) -C $(REPO) generated/atan-q16.inc

$(REPO)/generated/smoke-trig.inc: $(REPO)/tools/export-smoke-trig.py $(REPO)/data/Downwell.exe
	$(MAKE) -C $(REPO) generated/smoke-trig.inc

$(REPO)/generated/downwell-goal-mask.h: $(REPO)/tools/generate-goal-mask.py $(REPO)/tools/export-goal-mask.csx $(REPO)/data/data.win
	$(MAKE) -C $(REPO) generated/downwell-goal-mask.h

check:
	@set -e; for tool in $(CC) $(AS) $(OBJDUMP) elf2hunk; do \
		command -v $$tool >/dev/null || { echo "missing Amiga tool: $$tool"; exit 1; }; \
	done
	@echo "Amiga 68000 toolchain OK"

.PHONY: FORCE_HUNK_OPTIONS
# Audit even an up-to-date ELF: a rejected link remains on disk, and a
# subsequent make must not publish it merely because linking is now skipped.
$(OUT).exe: $(OUT).elf FORCE_HUNK_OPTIONS | audit
	@mkdir -p $(@D)
	@printf '%s\n' '$(STRIP)' > $(OUT).hunk-options.tmp
	@if [ ! -f $@ ] || [ $< -nt $@ ] || ! cmp -s $(OUT).hunk-options.tmp $(OUT).hunk-options; then \
		elf2hunk $< $@ $(if $(filter 1,$(STRIP)),-s) && \
		mv -f $(OUT).hunk-options.tmp $(OUT).hunk-options && echo "Built $@ (STRIP=$(STRIP))"; \
	else rm -f $(OUT).hunk-options.tmp; fi

$(OUT).elf: $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@
	$(OBJDUMP) --disassemble --no-show-raw-ins -S $@ > $(OUT).s

obj/DownwellAmiga.o: $(AMIGA_SRC)/DownwellAmiga.cpp \
		$(REPO)/generated/amiga-game-planes.h \
		$(REPO)/src/core/PortReplay.h $(AMIGA_SCENE_BANKS) $(REPO)/generated/amiga-hud-text.h $(REPO)/generated/amiga-health-art.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(REPO)/generated/amiga-hud-text.h: $(REPO)/tools/generate-amiga-hud-text.cpp $(REPO)/tools/generate-amiga-hud-text.py $(AMIGA_SRC)/AmigaHudText.h $(AMIGA_SRC)/AmigaRenderer.h $(AMIGA_HUD) $(AMIGA_GAME)
	$(MAKE) -C $(REPO) generated/amiga-hud-text.h

$(REPO)/generated/amiga-health-art.h: $(REPO)/tools/generate-amiga-health-art.cpp $(REPO)/tools/generate-amiga-health-art.py $(AMIGA_SRC)/AmigaRenderer.h $(AMIGA_SRC)/AmigaRenderer.c $(REPO)/src/core/HudNumbers.h $(AMIGA_HUD)
	$(MAKE) -C $(REPO) generated/amiga-health-art.h

$(REPO)/generated/amiga-health-source.inc: $(REPO)/generated/amiga-health-art.h
	@test -f $@ || (cd $(REPO) && python3 tools/generate-amiga-health-art.py)

.PHONY: verify-health-link
verify-health-link: $(REPO)/generated/amiga-health-source.inc $(REPO)/generated/amiga-ammo-factored.h
	python3 $(REPO)/tests/amiga-health-link-test.py --assembler "$$(command -v $(AS))" --linker "$$( $(CC) -print-prog-name=ld )"

obj/AmigaInput.o: $(AMIGA_SRC)/AmigaInput.cpp $(AMIGA_SRC)/AmigaInput.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaKeyboard.o: $(AMIGA_SRC)/AmigaKeyboard.cpp $(AMIGA_SRC)/AmigaKeyboard.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaWindAudio.o: $(AMIGA_SRC)/AmigaWindAudio.cpp $(AMIGA_SRC)/AmigaWindAudio.h $(REPO)/generated/amiga-wind.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -I$(REPO)/generated -c -o $@ $<

obj/DownwellAmiga.o: $(REPO)/generated/amiga-encounter-masks.h
$(REPO)/generated/amiga-encounter-masks.h: $(REPO)/tools/generate-encounter-masks.py $(REPO)/generated/downwell-sprite-masks.dwsm
	cd $(REPO) && python3 tools/generate-encounter-masks.py
out/Downwell.encountermasks: $(REPO)/generated/amiga-encounter-masks.h
	@mkdir -p $(@D)
	cp $(REPO)/generated/downwell-encounter-masks.dwsm $@

# Isolated real-Paula fixture; does not replace the normal game executable.
.PHONY: wind-audio-probe
wind-audio-probe: out/WindAudioProbe.exe out/Downwell.wind

out/Downwell.wind: $(REPO)/generated/amiga-wind.bin
	@mkdir -p $(@D)
	cp $< $@

$(REPO)/generated/amiga-wind.bin: $(REPO)/tools/generate-amiga-wind-bank.py
	$(MAKE) -C $(REPO) generated/amiga-wind.bin

$(REPO)/generated/amiga-wind.h: $(REPO)/generated/amiga-wind.bin
	@test -f $@ || (cd $(REPO) && python3 tools/generate-amiga-wind-bank.py)

obj/WindAudioProbe.o: $(REPO)/tests/platform/amiga/WindAudioProbe.cpp $(AMIGA_SRC)/AmigaWindAudio.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -DDOWNWELL_WIND_AUDIO_VERIFY -c -o $@ $<

obj/WindAudioProbeDriver.o: $(AMIGA_SRC)/AmigaWindAudio.cpp $(AMIGA_SRC)/AmigaWindAudio.h $(AMIGA_SRC)/WindBank.h $(REPO)/generated/amiga-wind.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -I$(REPO)/generated -DDOWNWELL_WIND_AUDIO_VERIFY -c -o $@ $<

out/WindAudioProbe.elf: obj/WindAudioProbe.o obj/WindAudioProbeDriver.o obj/AmigaPlatform.o obj/AmigaHardware.o obj/CopperList.o obj/GCCRuntime.o obj/AmigaMemoryCopy.o obj/gcc8_c_support.o obj/gcc8_a_support.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=out/WindAudioProbe.map $^ -o $@

out/WindAudioProbe.exe: out/WindAudioProbe.elf
	elf2hunk $< $@ -s

# WR.1 free-memory probe: the budget after boot on the selected machine.
.PHONY: free-memory-probe
free-memory-probe: out/FreeMemoryProbe.exe

obj/FreeMemoryProbe.o: $(REPO)/tests/platform/amiga/FreeMemoryProbe.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

out/FreeMemoryProbe.elf: obj/FreeMemoryProbe.o obj/GCCRuntime.o obj/AmigaMemoryCopy.o obj/gcc8_c_support.o obj/gcc8_a_support.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=out/FreeMemoryProbe.map $^ -o $@

out/FreeMemoryProbe.exe: out/FreeMemoryProbe.elf
	elf2hunk $< $@ -s

# W2.4 Paula mixer listening probe; does not replace the game executable.
.PHONY: paula-mixer-probe
paula-mixer-probe: out/PaulaMixerProbe.exe out/Downwell.sfx

out/Downwell.sfx: $(REPO)/generated/amiga-sfx.bin
	@mkdir -p $(@D)
	cp $< $@

$(REPO)/generated/amiga-sfx.bin $(REPO)/generated/amiga-sfx.h: $(REPO)/tools/pack-amiga-sfx.py
	$(MAKE) -C $(REPO) generated/amiga-sfx.bin

# W2.5 scene sound banks (AmigaSoundBanks), from the same pack run.
$(REPO)/generated/amiga-sfx-%.bin: $(REPO)/generated/amiga-sfx.bin
	@test -f $@
$(OUT).snd%: $(REPO)/generated/amiga-sfx-%.bin
	@mkdir -p $(@D)
	cp $< $@

obj/AmigaSoundBanks.o: $(AMIGA_SRC)/AmigaSoundBanks.cpp $(AMIGA_SRC)/AmigaSoundBanks.h $(AMIGA_SRC)/AmigaPaulaAudio.h $(AMIGA_SRC)/PaulaMixer.h $(REPO)/generated/amiga-sfx.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -I$(REPO)/generated -c -o $@ $<
obj/DownwellAmiga.o: $(AMIGA_SRC)/AmigaSoundBanks.h $(AMIGA_SRC)/AmigaPaulaAudio.h $(AMIGA_SRC)/PaulaMixer.h

obj/AmigaPaulaAudio.o: $(AMIGA_SRC)/AmigaPaulaAudio.cpp $(AMIGA_SRC)/AmigaPaulaAudio.h $(AMIGA_SRC)/PaulaMixer.h $(REPO)/src/core/SoundRequests.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/PaulaMixerProbe.o: $(REPO)/tests/platform/amiga/PaulaMixerProbe.cpp $(REPO)/tests/platform/amiga/PaulaProbeSchedule.h $(AMIGA_SRC)/AmigaPaulaAudio.h $(AMIGA_SRC)/PaulaMixer.h $(REPO)/generated/amiga-sfx.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -I$(REPO)/generated -I$(REPO)/tests/platform/amiga -c -o $@ $<

out/PaulaMixerProbe.elf: obj/PaulaMixerProbe.o obj/AmigaPaulaAudio.o obj/AmigaPlatform.o obj/AmigaHardware.o obj/CopperList.o obj/GCCRuntime.o obj/AmigaMemoryCopy.o obj/gcc8_c_support.o obj/gcc8_a_support.o
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=out/PaulaMixerProbe.map $^ -o $@

out/PaulaMixerProbe.exe: out/PaulaMixerProbe.elf
	elf2hunk $< $@ -s

obj/AmigaProfile.o: $(AMIGA_SRC)/AmigaProfile.cpp $(AMIGA_SRC)/AmigaProfile.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPlayer.o: $(REPO)/src/core/PlayerMotion.cpp $(REPO)/src/core/PlayerMotion.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPortRuntime.o: $(REPO)/src/core/GameplayWorld.cpp \
		$(REPO)/src/core/GameplayWorld.h $(REPO)/src/core/PlayerMotion.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaSceneRuntime.o: $(AMIGA_SRC)/AmigaSceneRuntime.cpp \
		$(AMIGA_SRC)/AmigaSceneRuntime.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/DownwellCopperLists.o: $(AMIGA_SRC)/DownwellCopperLists.cpp \
		$(AMIGA_SRC)/DownwellCopperLists.h \
		$(AMIGA_SRC)/framework/CopperList.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaTerrain.o: $(AMIGA_SRC)/AmigaTerrain.cpp \
		$(AMIGA_SRC)/AmigaTerrain.h $(AMIGA_SRC)/AmigaPortRuntime.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaGameplayGeneration.o: $(AMIGA_SRC)/AmigaGameplayGeneration.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMenu.o: $(AMIGA_SRC)/AmigaMenu.cpp $(AMIGA_SRC)/AmigaMenu.h \
		$(REPO)/src/core/RoomPack.h $(REPO)/src/core/ObjectPack.h \
		$(REPO)/src/core/GameMakerRandom.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMenuScroll.o: $(AMIGA_SRC)/AmigaMenuScroll.cpp \
		$(AMIGA_SRC)/AmigaMenuScroll.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMenuCache.o: $(AMIGA_SRC)/AmigaMenuCache.cpp \
		$(AMIGA_SRC)/AmigaMenuCache.h $(AMIGA_SRC)/AmigaMenu.h \
		$(AMIGA_SRC)/AmigaMenuScroll.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaStyleRenderer.o: $(AMIGA_SRC)/AmigaStyleRenderer.cpp \
		$(AMIGA_SRC)/AmigaStyleRenderer.h $(AMIGA_SRC)/AmigaSceneRuntime.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaUpgradeScene.o: $(AMIGA_SRC)/AmigaUpgradeScene.cpp $(AMIGA_SRC)/AmigaUpgradeScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaStyleScene.o: $(AMIGA_SRC)/AmigaStyleScene.cpp $(AMIGA_SRC)/AmigaStyleScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPublisherScene.o: $(AMIGA_SRC)/AmigaPublisherScene.cpp $(AMIGA_SRC)/AmigaPublisherScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaGardenScene.o: $(AMIGA_SRC)/AmigaGardenScene.cpp $(AMIGA_SRC)/AmigaGardenScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaEffectSurface.o: $(AMIGA_SRC)/AmigaEffectSurface.cpp $(AMIGA_SRC)/AmigaEffectSurface.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaGameplayEffects.o: $(AMIGA_SRC)/AmigaGameplayEffects.cpp $(AMIGA_SRC)/AmigaGameplayEffects.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaGameplayRows.o: $(AMIGA_SRC)/AmigaGameplayRows.cpp $(AMIGA_SRC)/AmigaGameplayRows.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaGameplayScene.o: $(AMIGA_SRC)/AmigaGameplayScene.cpp $(AMIGA_SRC)/AmigaGameplayScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
$(REPO)/generated/amiga-damage-text.h: $(REPO)/tools/generate-amiga-damage-text.py $(AMIGA_GAME)
	$(MAKE) -C $(REPO) generated/amiga-damage-text.h

obj/AmigaGameplayActors.o: $(AMIGA_SRC)/AmigaGameplayActors.cpp $(AMIGA_SRC)/AmigaGameplayActors.h $(AMIGA_SRC)/AmigaDamageText.h $(REPO)/generated/amiga-damage-text.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaGameplaySpriteAssignment.o: $(AMIGA_SRC)/AmigaGameplaySpriteAssignment.cpp $(AMIGA_SRC)/AmigaGameplayActors.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaGameplayDrawList.o: $(AMIGA_SRC)/AmigaGameplayDrawList.cpp $(AMIGA_SRC)/AmigaGameplayDrawList.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaTerrainRaster.o: $(AMIGA_SRC)/AmigaTerrainRaster.cpp $(AMIGA_SRC)/AmigaTerrainRaster.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaTerrainPainter.o: $(AMIGA_SRC)/AmigaTerrainPainter.cpp $(AMIGA_SRC)/AmigaTerrainPainter.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaCreditsScene.o: $(AMIGA_SRC)/AmigaCreditsScene.cpp $(AMIGA_SRC)/AmigaCreditsScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<
obj/AmigaControlsScene.o: $(AMIGA_SRC)/AmigaControlsScene.cpp $(AMIGA_SRC)/AmigaControlsScene.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaSelectionLayer.o: $(AMIGA_SRC)/AmigaSelectionLayer.cpp $(AMIGA_SRC)/AmigaSelectionLayer.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaSplashCache.o: $(AMIGA_SRC)/AmigaSplashCache.cpp \
		$(AMIGA_SRC)/AmigaSplashCache.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaHardware.o: $(AMIGA_SRC)/framework/AmigaHardware.cpp \
		$(AMIGA_SRC)/framework/AmigaHardware.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Bitmap.o: $(AMIGA_SRC)/framework/Bitmap.cpp \
		$(AMIGA_SRC)/framework/Bitmap.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Palette.o: $(AMIGA_SRC)/framework/Palette.cpp \
		$(AMIGA_SRC)/framework/Palette.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Sprite.o: $(AMIGA_SRC)/framework/Sprite.cpp \
		$(AMIGA_SRC)/framework/Sprite.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Util.o: $(AMIGA_SRC)/framework/Util.cpp \
		$(AMIGA_SRC)/framework/Util.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -Wno-narrowing -Wno-overflow -c -o $@ $<

obj/GCCRuntime.o: $(AMIGA_SRC)/framework/GCCRuntime.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaPlatform.o: $(AMIGA_SRC)/framework/AmigaPlatform.cpp \
		$(AMIGA_SRC)/framework/AmigaPlatform.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaMemory.o: $(AMIGA_SRC)/framework/AmigaMemory.cpp \
		$(AMIGA_SRC)/framework/AmigaMemory.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/CopperList.o: $(AMIGA_SRC)/framework/CopperList.cpp \
		$(AMIGA_SRC)/framework/CopperList.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/RoomPack.o: $(REPO)/src/core/RoomPack.cpp $(REPO)/src/core/RoomPack.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/ObjectPack.o: $(REPO)/src/core/ObjectPack.cpp $(REPO)/src/core/ObjectPack.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/BuilderPatternPack.o: $(REPO)/src/core/BuilderPatternPack.cpp \
		$(REPO)/src/core/BuilderPatternPack.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AreaOneBuilder.o: $(REPO)/src/core/AreaOneBuilder.cpp \
		$(REPO)/src/core/AreaOneBuilder.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/GameMakerRandom.o: $(REPO)/src/core/GameMakerRandom.cpp \
		$(REPO)/src/core/GameMakerRandom.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/Replay.o: $(REPO)/src/core/Replay.cpp $(REPO)/src/core/Replay.h \
		$(REPO)/src/core/Input.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/AmigaRenderer.o: $(AMIGA_SRC)/AmigaRenderer.c $(AMIGA_SRC)/AmigaRenderer.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

obj/AmigaBlitter.o: $(AMIGA_SRC)/AmigaBlitter.cpp \
		$(AMIGA_SRC)/AmigaPlaneBank.h \
		$(AMIGA_SRC)/framework/AmigaHardware.h \
		$(AMIGA_SRC)/framework/Bitmap.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(AMIGA_HUD):
	$(MAKE) -C $(REPO) generate-amiga-hud

$(AMIGA_GAME): $(REPO)/tools/generate-amiga-hud.py $(REPO)/tools/amiga_scene_contract.py
	$(MAKE) -C $(REPO) generate-amiga-game

$(AMIGA_RESULTS): $(REPO)/tools/generate-amiga-hud.py
	$(MAKE) -C $(REPO) generate-amiga-results

$(AMIGA_SHOP): $(REPO)/tools/generate-amiga-hud.py $(REPO)/data/language.ini
	$(MAKE) -C $(REPO) generate-amiga-shop
$(AMIGA_PAUSE): $(REPO)/tools/generate-amiga-hud.py
	$(MAKE) -C $(REPO) generate-amiga-pause

$(AMIGA_CLEAR): $(REPO)/tools/generate-amiga-hud.py $(REPO)/tools/amiga_scene_contract.py $(REPO)/data/language.ini
	$(MAKE) -C $(REPO) generate-amiga-clear

$(ROOM_PACK): $(REPO)/tools/generate-room-pack.py \
	$(REPO)/tools/export-room-layer-metadata.csx $(REPO)/tools/export-room-layer-metadata.sh \
	$(REPO)/tools/verify-room-layer-metadata.py $(REPO)/src/core/RoomPack.h \
	$(REPO)/extracted/utmt-0.9.2.0/manifest.json $(REPO)/data/data.win
	$(MAKE) -C $(REPO) generate-room-pack

$(OBJECT_PACK):
	$(MAKE) -C $(REPO) generate-object-pack

$(COLLISION_PACK):
	$(MAKE) -C $(REPO) generate-sprite-pack

$(COLLISION_MASK_PACK):
	$(MAKE) -C $(REPO) generate-collision-masks

$(BUILDER_PACK): $(REPO)/tools/generate-builder-pattern-pack.py
	$(MAKE) -C $(REPO) generate-builder-pattern-pack

$(AMIGA_EXIT_MASK): $(AMIGA_GAME)
	@test -f $@ || $(MAKE) -C $(REPO) generate-amiga-game

$(AMIGA_ENTRY_MASK): $(AMIGA_GAME)
	@test -f $@ || $(MAKE) -C $(REPO) generate-amiga-game

$(AMIGA_SELECTION_MASK): $(AMIGA_GAME)
	@test -f $@ || $(MAKE) -C $(REPO) generate-amiga-game

$(AMIGA_BOOT_BANK) $(AMIGA_FRONT_BANK) $(AMIGA_SCENE_BANKS) $(REPO)/generated/amiga-cavern.dwap: $(AMIGA_GAME)
	@test -f $@ || $(MAKE) -C $(REPO) generate-amiga-game

$(AMIGA_EFFECT_STAMPS): $(AMIGA_GAME) $(REPO)/tools/generate-effect-stamps.py
	cd $(REPO) && python3 tools/generate-effect-stamps.py

obj/AmigaAssets.o: obj/startup-options $(AMIGA_SRC)/AmigaAssets.s $(AMIGA_HUD) $(REPO)/generated/amiga-health-source.inc $(REPO)/generated/amiga-health-art.h $(AMIGA_GAME) $(AMIGA_EFFECT_STAMPS) $(AMIGA_EXIT_MASK) $(AMIGA_ENTRY_MASK) $(AMIGA_SELECTION_MASK) \
		$(AMIGA_RESULTS) $(AMIGA_SHOP) $(AMIGA_CLEAR) $(ROOM_PACK) $(OBJECT_PACK) $(COLLISION_PACK) $(REFERENCE_COLLISION_MASK_INPUT) $(BUILDER_PACK)
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $(AMIGA_SRC)/AmigaAssets.s

$(REPO)/generated/amiga-terrain-phase6.bin: $(AMIGA_GAME) $(REPO)/tools/generate-terrain-phase.py
	python3 $(REPO)/tools/generate-terrain-phase.py

$(REPO)/generated/amiga-initial-terrain.bin: $(AMIGA_GAME) $(REPO)/generated/downwell-rooms.dwrm $(REPO)/generated/amiga-terrain-phase6.bin $(REPO)/tools/generate-initial-terrain.py $(REPO)/tools/prepare-initial-terrain.cpp $(AMIGA_SRC)/AmigaTerrainPainter.cpp $(AMIGA_SRC)/AmigaTerrain.cpp $(AMIGA_SRC)/AmigaTerrain.h $(AMIGA_SRC)/AmigaRenderer.c $(AMIGA_SRC)/AmigaRenderer.h $(AMIGA_SRC)/AmigaGameplayRows.h $(REPO)/src/core/RoomPack.cpp $(REPO)/src/core/RoomPack.h $(AMIGA_SRC)/AmigaTerrainRunSurface.h $(AMIGA_SRC)/AmigaInitialTerrain.h
	$(MAKE) -C $(REPO) generate-initial-terrain

obj/AmigaTerrainPrepared.o: $(AMIGA_SRC)/AmigaTerrainPrepared.s $(REPO)/generated/amiga-terrain-phase6.bin $(REPO)/generated/amiga-initial-terrain.bin
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

obj/AmigaPatternTransfer.o: $(AMIGA_SRC)/AmigaPatternTransfer.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

$(REPO)/generated/amiga-field-images.bin: $(REPO)/tools/generate-field-images.cpp $(REPO)/tools/generate-field-images.py $(REPO)/generated/downwell-field-spans.h $(wildcard $(REPO)/src/core/*.h)
	$(MAKE) -C $(REPO) generated/amiga-field-images.bin
$(REPO)/generated/downwell-field-columns.h: $(REPO)/tools/generate-field-columns.cpp $(REPO)/tools/generate-field-columns.py $(wildcard $(REPO)/src/core/*.h)
	$(MAKE) -C $(REPO) generated/downwell-field-columns.h
ifeq ($(PREPARED_FIELDS),1)
obj/DownwellAmiga.o obj/AmigaBlitter.o: $(REPO)/generated/downwell-field-columns.h
endif
obj/AmigaFieldPrepared.o: $(AMIGA_SRC)/AmigaFieldPrepared.s $(REPO)/generated/amiga-field-images.bin
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

obj/gcc8_c_support.o: $(SUPPORT)/gcc8_c_support.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Dmemcpy=downwellToolchainMemcpy -Dmemset=downwellToolchainMemset -c -o $@ $<

obj/AmigaMemoryCopy.o: $(AMIGA_SRC)/AmigaMemoryCopy.cpp $(AMIGA_SRC)/AmigaMemoryCopy.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/gcc8_a_support.o: $(SUPPORT)/gcc8_a_support.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

# WR.2 shipping size gate: `make size-gate STRIP=1` on the default
# configuration. Ceilings sit just above the current build; lower them as
# WR shrinks it. Verify, probe and profile builds are not shipping builds.
SIZE_MAX_EXE ?= 1150000
SIZE_MAX_TEXT ?= 690000
SIZE_MAX_DATA ?= 425000
SIZE_MAX_BSS ?= 1280000
.PHONY: size-gate
size-gate: $(OUT).exe
	@if [ '$(STRIP)' != 1 ]; then echo "*** size-gate measures the stripped executable: use STRIP=1"; exit 1; fi
	@if [ -n '$(filter %_VERIFY -DDOWNWELL_PROBES -DDOWNWELL_FPSCOUNT -DDOWNWELL_PROFILE_%,$(CFLAGS))' ]; then \
		echo "*** size-gate needs the shipping configuration, not $(filter %_VERIFY -DDOWNWELL_PROBES -DDOWNWELL_FPSCOUNT -DDOWNWELL_PROFILE_%,$(CFLAGS))"; exit 1; fi
	OBJDUMP=$(OBJDUMP) python3 $(REPO)/tools/amiga-size-report.py $(OUT).elf --exe $(OUT).exe --top 0 --classes 0 \
		--max-exe $(SIZE_MAX_EXE) --max-text $(SIZE_MAX_TEXT) --max-data $(SIZE_MAX_DATA) --max-bss $(SIZE_MAX_BSS)

audit: $(OUT).elf
	@if [ '$(ENTRY_WIPE_VERIFY)' != 1 ] && $(OBJDUMP) -t $(OUT).elf | grep -q 'amiga_entry_mask'; then \
		echo "*** entry mask must be scene-loaded, not linked"; exit 1; fi
	@if [ -z '$(findstring DOWNWELL_REFERENCE_SCENE_BANKS,$(CPPFLAGS))' ] && $(OBJDUMP) -t $(OUT).elf | grep -q 'amiga_hud_pack'; then \
		echo "*** HUD source must be scene-loaded, not linked"; exit 1; fi
	@if $(OBJDUMP) -t $(OUT).elf | grep -q 'amiga_selection_mask'; then \
		echo "*** selection mask must be scene-loaded, not linked"; exit 1; fi
	@if [ '$(EXIT_WIPE_VERIFY)' != 1 ] && $(OBJDUMP) -t $(OUT).elf | grep -q 'amiga_exit_mask'; then \
		echo "*** exit mask must be scene-loaded, not linked"; exit 1; fi
	@if $(OBJDUMP) -t $(OUT).elf | grep -q 'amiga_builder_pack'; then \
		echo "*** builder patterns must be run-loaded, not linked"; exit 1; fi
	@if $(OBJDUMP) -t $(OUT).elf | grep -qE 'fieldBandPixels|fieldColumnWords'; then \
		echo "*** gameplay field upload pixels must be scene-loaded, not linked"; exit 1; \
	else echo "Field image source linkage audit: clean"; fi
	@python3 $(REPO)/tools/audit-amiga-scene-loads.py $(OUT).elf $(if $(filter 1,$(REPLAY_VERIFY) $(PLANE_VERIFY)),--reference-oracle)
	@if $(OBJDUMP) -t $(OUT).elf | grep -qE 'amiga_game_pack'; then \
		echo "*** gameplay CPU art must be scene-loaded, not linked"; exit 1; \
	else echo "Gameplay CPU art linkage audit: clean"; fi
	@if [ -z '$(REFERENCE_COLLISION_MASK_INPUT)' ] && $(OBJDUMP) -t $(OUT).elf | grep -q 'amiga_collision_masks'; then \
		echo "*** reference collision masks linked without an admitted query probe"; exit 1; \
	else echo "Collision-mask ownership linkage audit: clean"; fi
	@if [ -z '$(findstring DOWNWELL_REFERENCE_SCENE_BANKS,$(CPPFLAGS))' ] && $(OBJDUMP) -t $(OUT).elf | grep -qE 'amiga_(results|shop|clear)_pack'; then \
		echo "*** transient scene art linked into shipping executable"; exit 1; \
	else echo "Transient scene-bank linkage audit: clean"; fi
	@if $(OBJDUMP) -t $(OUT).elf | grep -qE \
		'__(ashl|ashr|lshr|mul|u?div|u?mod)di3'; then \
		echo "*** unexpected 64-bit software arithmetic in native executable"; exit 1; \
	else echo "68000 64-bit helper audit: clean"; fi
	@if $(OBJDUMP) -d --no-show-raw-insn $(OUT).elf | grep -qiE \
		'^[[:space:][:xdigit:]]+:[[:space:][:xdigit:]]+[[:space:]]+(bf[a-z]+|bkpt|callm|cas2?|chk2|cmp2|div[su]l|extb|move16|movec|moves|pack|rt[dm]|trap[a-z]{2}|unpk)([.[:space:]]|$$)'; then \
		echo "*** instruction newer than 68000 in native executable"; exit 1; \
	else echo "68000 instruction audit: clean"; fi

.PHONY: verify-box-effects-compile
verify-box-effects-compile:
	@mkdir -p obj
	$(CC) $(CPPFLAGS) -c -o obj/BoxWorldEffectsCompile.o $(REPO)/tests/core/BoxWorldEffectsCompile.cpp

.PHONY: verify-builder-traversal-compile
verify-builder-traversal-compile:
	@mkdir -p obj
	$(CC) $(CPPFLAGS) -c -o obj/BuilderStringTraversalCompile.o $(REPO)/tests/core/BuilderStringTraversalCompile.cpp

.PHONY: verify-blood-compile
verify-blood-compile:
	@mkdir -p obj
	$(CC) $(CPPFLAGS) -c -o obj/BloodInstancesCompile.o $(REPO)/tests/core/BloodInstancesCompile.cpp

.PHONY: verify-time-shard-registration-compile
verify-time-shard-registration-compile:
	@mkdir -p obj
	$(CC) $(CPPFLAGS) -DDOWNWELL_NATIVE_QUERY_PROBE -c -o obj/TimeShardRegistrationCompile.o $(REPO)/tests/core/GameplayTimeShardRegistrationTest.cpp

# Keep .run: it owns the live FS-UAE pid used by run.sh to replace the
# previous project emulator safely.  Removing it leaves a stale window.
clean:
	$(RM) -r obj out

-include $(OBJECTS:.o=.d)
-include obj/WindAudioProbe.d obj/WindAudioProbeDriver.d

$(AMIGA_HOT_OBJECTS): CPPFLAGS += $(AMIGA_HOT_OPTIMIZATION)
