#ifndef DOWNWELL_GAMEPLAY_SHOP_PROFILE_INPUT_H
#define DOWNWELL_GAMEPLAY_SHOP_PROFILE_INPUT_H
#include "Input.h"
namespace downwell {
// Read-only buttons selected on the host. Advance only after a gameplay Step,
// never while the scene backend is preparing a frame or running an upgrade.
class GameplayShopProfileInput {
    const unsigned char* tapes_[2];
    unsigned lengths_[2], cursors_[2]{};
public:
    constexpr GameplayShopProfileInput(const unsigned char* a,unsigned na,const unsigned char* b,unsigned nb)
        :tapes_{a,b},lengths_{na,nb}{}
    unsigned packed(unsigned level,bool transition)const {
        if(transition)return 4;
        if(level<1||level>2||cursors_[level-1]>=lengths_[level-1])return 0;
        return tapes_[level-1][cursors_[level-1]];
    }
    bool advance(unsigned level){
        if(level<1||level>2||cursors_[level-1]>=lengths_[level-1])return false;
        ++cursors_[level-1];return true;
    }
    unsigned frames(unsigned level)const{return level>=1&&level<=2?cursors_[level-1]:0;}
    bool done()const{return cursors_[0]==lengths_[0]&&cursors_[1]==lengths_[1];}
};
}
#endif
