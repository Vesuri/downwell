#include "core/RedBallStep.h"
using S=downwell::RedBallStep;using N=downwell::MotionBinary64;
#ifdef DOWNWELL_FREESTANDING
struct Ops {
    bool shade();bool drawSelf();bool imageIndex(N&);
    bool drawSpriteExt(int,N,N,N,int,int,int,unsigned,int);
};
bool nativeRedBallDraw(S& state,Ops& ops){return state.draw(ops);}
#else
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static N real(double value){std::uint64_t bits;std::memcpy(&bits,&value,8);return {unsigned(bits>>32),unsigned(bits)};}
static bool same(N a,N b){return a.high==b.high&&a.low==b.low;}
static S initial(){S s;s.initialized=true;s.angle16=real(22.5);s.x={0x43800000u};s.y={0x43000000u};return s;}
struct Ops {
    S& s;unsigned reject=0;std::vector<int> calls;
    int sprite=-1,mirror=0;bool changeHeading=false;
    Ops(S& state,unsigned failure=0):s(state),reject(failure){}
    bool accept(int code){calls.push_back(code);return calls.size()!=reject;}
    bool shade(){if(changeHeading)s.playerDirection=real(90);return accept(1);}
    bool drawSelf(){return accept(2);}
    bool imageIndex(N& frame){assert(s.eyeInitialized);frame=real(2.5);return accept(3);}
    bool drawSpriteExt(int eye,N frame,N x,N y,int xs,int ys,int angle,unsigned color,int alpha){
        assert(same(frame,real(2.5))&&same(x,real(256))&&same(y,real(123))&&ys==1&&!angle&&color==0xffffffu&&alpha==1);
        sprite=eye;mirror=xs;return accept(4);
    }
};
int main(){
    const int sprites[]={182,184,185,184,182,183,181,183,182};
    const int mirrors[]={1,1,1,-1,-1,-1,-1,1,1};
    const int shots[]={0,45,90,135,180,225,270,315,0};
    for(unsigned eighth=0;eighth<359*8;++eighth){
        S s=initial();const double angle=eighth/8.;s.playerDirection=real(angle);Ops ops{s};
        const unsigned row=angle<22.5?0:unsigned((angle-22.5)/45)+1;
        assert(s.draw(ops)&&ops.sprite==sprites[row]&&ops.mirror==mirrors[row]&&s.shotDirection==shots[row]);
        assert((ops.calls==std::vector<int>{1,2,3,4}));
    }
    // VM comparisons include epsilon: a heading just below a sector boundary
    // belongs to the next sector, unlike floor(angle / 45).
    for(unsigned row=1;row<9;++row){
        S s=initial();s.playerDirection=real(22.5+(row-1)*45-0.000001);Ops ops{s};
        assert(s.draw(ops)&&ops.sprite==sprites[row]&&ops.mirror==mirrors[row]&&s.shotDirection==shots[row]);
    }
    for(double angle:{-1.,358.999999,359.,359.5,360.}){
        S s=initial();s.playerDirection=real(angle);Ops first{s};
        assert(!s.draw(first)&&!s.eyeInitialized&&(first.calls==std::vector<int>{1,2}));
        s.playerDirection=real(90);Ops select{s};assert(s.draw(select)&&s.eye==185);
        s.playerDirection=real(angle);Ops retained{s};
        assert(s.draw(retained)&&retained.sprite==185&&s.shotDirection==90);
    }
    for(unsigned failure=1;failure<=4;++failure){
        S s=initial();s.playerDirection=real(225);Ops ops{s,failure};
        assert(!s.draw(ops)&&ops.calls.size()==failure&&s.eyeInitialized==(failure>=3));
        if(failure>=3)assert(s.eye==183&&s.eyeX==-1&&s.shotDirection==225);
    }
    {
        S s=initial();s.playerDirection=real(0);Ops ops{s};ops.changeHeading=true;
        assert(s.draw(ops)&&ops.sprite==185); // Selection follows shade/body.
    }
    std::puts("Red-ball Draw: sector/epsilon boundaries, retained eye fields, body-before-eye and failure prefixes passed");
}
#endif
