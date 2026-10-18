# Diagnostic production-session admission; not a shipping backend selector.
CPPFLAGS += -UDOWNWELL_EXTERNAL_COLLISION_BACKEND -DDOWNWELL_SCENE_COLLISION_BACKEND
# Candidate fixtures use the actual scene-loaded mask lease, not the unrelated
# all-resource query probe retained by reference-owner pixel fixtures.
ASFLAGS := $(subst --defsym DOWNWELL_REFERENCE_COLLISION_MASKS=1,,$(ASFLAGS))
REFERENCE_COLLISION_MASK_INPUT :=
