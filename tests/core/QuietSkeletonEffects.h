#ifndef DOWNWELL_TEST_QUIET_SKELETON_EFFECTS_H
#define DOWNWELL_TEST_QUIET_SKELETON_EFFECTS_H
#include "core/MotionBinary64.h"
// Controlled living/hit-stun fixtures. Unexpected throw/death paths fail;
// these are not no-op production replacements for unfinished effect owners.
struct QuietSkeletonEffects {
    unsigned& hurts;
    bool hurtSound(int id){++hurts;return id==133;}
    bool throwBone(int){return false;}
    bool enemyDeath(){return false;}
    bool blood(int,int){return false;}
    bool smoke(downwell::MotionBinary64,downwell::MotionBinary64,int,int){return false;}
    bool flash(downwell::MotionBinary64,downwell::MotionBinary64,int,int,int){return false;}
    bool currency(int){return false;}
    bool deathSound(int){return false;}
    bool deadBody(int,downwell::MotionBinary64,int){return false;}
    bool debris(int,int){return false;}
};
template<class Throw> struct ThrowingSkeletonEffects:QuietSkeletonEffects {
    Throw launch;
    ThrowingSkeletonEffects(unsigned& hurts,Throw launch):QuietSkeletonEffects{hurts},launch(launch){}
    bool throwBone(int facing){return launch(facing);}
};
#endif
