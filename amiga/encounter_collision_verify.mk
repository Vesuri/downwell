# Additional makefile for the production collision-session failure fixture.
# The normal backend remains unchanged until its performance gate passes.
CPPFLAGS += -UDOWNWELL_EXTERNAL_COLLISION_BACKEND -DDOWNWELL_SCENE_COLLISION_BACKEND -DDOWNWELL_ENCOUNTER_UPDATE_FAILURE_VERIFY
