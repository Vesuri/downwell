# Diagnostic only; use after Makefile and scene_collision_candidate.mk.
# Counts change timing. Never compare this executable's cadence to a baseline.
CPPFLAGS += -DDOWNWELL_COLLISION_QUERY_CENSUS
