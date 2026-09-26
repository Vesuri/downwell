#ifndef DOWNWELL_CORE_PLAYER_PRECISE_VERIFICATION_H
#define DOWNWELL_CORE_PLAYER_PRECISE_VERIFICATION_H
#include "PlayerGravityBinary64.h"
#include "PlayerWallResponseBinary64.h"
#include "PlayerBoxContact.h"
namespace downwell {
struct PlayerPreciseVerification {
    unsigned checks=0,failures=0;std::uint32_t digest=2166136261u;
    void check(bool yes){++checks;if(!yes)++failures;}
    void word(std::uint32_t w){digest=(digest^w)*16777619u;}
    void real(MotionBinary64 n){word(n.high);word(n.low);}
};
// Cross-CPU composition fixture, not a full player script or natural route.
// Synthetic jumps/geometry deliberately exercise the checked numeric stages.
[[gnu::noinline]] inline PlayerPreciseVerification verifyPlayerPrecise(){
    using N=MotionBinary64;
    PlayerPreciseVerification result;
    for(unsigned scenario=0;scenario<8;++scenario){
        PlayerWallResponseBinary64 state;PlayerGravityBinary64 gravity;
        state.x=N::fromInteger(240);int facing=1;
        struct Events {
            PlayerPreciseVerification& result;bool stopped=false;
            bool meeting(int target,N x,N y,bool& hit){
                result.word(unsigned(target));result.real(x);result.real(y);
                const auto less=PlayerHorizontalBinary64::less;
                const bool floor=!less(y,N::fromInteger(16));
                const bool walls=less(x,N::fromInteger(232))||less(N::fromInteger(248),x)||
                    less(y,N::fromInteger(-8));
                const bool platform=less(x,N::fromInteger(238))&&!less(y,N::fromInteger(8));
                hit=target==87?platform:target==57?(floor||walls):(floor||walls||platform);
                return true;
            }
            bool breakBox(N x,N y,bool& found){result.real(x);result.real(y);result.word(92);found=true;return true;}
            bool timeStopped(){return stopped;}
            bool comboDone(){result.word(1000);return true;}
            bool recharge(){result.word(1001);return true;}
            N gravity(){return {0x3fc99999u,0x9999999au};}
        } events{result};
        for(unsigned tick=0;tick<64;++tick){
            if(tick%24==0){state.vy=N::fromInteger(-3);state.grounded=false;}
            state.airStatus=!state.grounded;events.stopped=(tick%16)<4;
            result.check(gravity.apply(state.vy,scenario&4?4:1,scenario&1,scenario&2?3:0,
                scenario&4,events.gravity(),N::fromInteger(7)));
            const bool left=(tick/12)%2==0,right=!left;
            result.check(PlayerHorizontalBinary64::apply(state.vx,facing,state.grounded,left,right));
            result.check(state.resolve(left,right,events));
            unsigned writes=0;
            result.check(state.integrateAndPublish(tick%16==0,scenario&1,[&](N x){
                result.check(writes++==0);result.real(x);return true;
            },[&](N y){result.check(writes++==1);result.real(y);return true;}));
            result.real(state.x);result.real(state.y);result.real(state.vx);result.real(state.vy);
            result.real(gravity.finalGravity);result.real(gravity.finalMaximum);
            result.word(state.grounded);result.word(state.hardLand);result.word(state.wallColliding);
            result.word(unsigned(state.collision.x));result.word(unsigned(state.collision.y));result.word(unsigned(facing));
        }
    }
    // Live ceiling-box adapter, including half ties at odd/even room origins.
    auto rounded=[](long value){
        long q=value/1280,r=value%1280;
        if(r>640||(r==640&&(q%2)))++q;
        if(r< -640||(r== -640&&(q%2)))--q;
        return q;
    };
    for(int origin=240;origin<242;++origin)for(long x=-15360;x<=15360;x+=320){
        bool hit=false;
        result.check(PlayerBoxContact::test(x,0,origin,176,origin,176,hit));
        const long left=rounded(x+(origin-4)*1280L);
        const long right=rounded(x+(origin+4)*1280L)-1;
        result.check(hit==(right>=origin-8&&left<=origin+7));
        result.word(hit);
        for(long y=0;y<2;++y){
            int cx=0,cy=0;
            result.check(PlayerBoxContact::terrainCentre(x,y,origin,176,cx,cy));
            result.check(cx==left+4-origin&&cy==0);
            result.word(static_cast<unsigned>(cx));result.word(static_cast<unsigned>(cy));
        }
    }
    return result;
}
}
#endif
