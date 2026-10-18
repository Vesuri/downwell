# Additional makefile for the production collision-session failure fixture.
# The normal backend remains unchanged until its performance gate passes.
include scene_collision_candidate.mk
CPPFLAGS += -DDOWNWELL_ENCOUNTER_UPDATE_FAILURE_VERIFY
