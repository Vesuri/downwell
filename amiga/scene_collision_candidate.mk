# Diagnostic production-session admission; not a shipping backend selector.
CPPFLAGS += -UDOWNWELL_EXTERNAL_COLLISION_BACKEND -DDOWNWELL_SCENE_COLLISION_BACKEND
# The owner binds later-area families: not a world-one build.
CPPFLAGS := $(filter-out -DDOWNWELL_WORLD_ONE_ONLY,$(CPPFLAGS))
# Its collision owner binds source terrain records (see the Makefile).
ifeq ($(REFERENCE_TERRAIN_RECORDS),0)
ifeq ($(findstring -DDOWNWELL_TERRAIN_SOURCE_RECORDS,$(CPPFLAGS)),)
CPPFLAGS += -DDOWNWELL_TERRAIN_SOURCE_RECORDS
endif
endif
# Candidate fixtures use the actual scene-loaded mask lease, not the unrelated
# all-resource query probe retained by reference-owner pixel fixtures.
ASFLAGS := $(subst --defsym DOWNWELL_REFERENCE_COLLISION_MASKS=1,,$(ASFLAGS))
REFERENCE_COLLISION_MASK_INPUT :=
