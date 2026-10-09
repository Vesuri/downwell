#include "core/LevelNumber.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Draw {
    bool surface=true;unsigned views=0,ends=0,failCharacter=0;
    std::vector<int> tokens,xs,ys;
    bool exists(bool& result){result=surface;return true;}
    bool begin(){return true;}
    bool view(unsigned axis,MotionBinary64& value){
        assert(axis==unsigned(views>=2));
        const int positions[]={100,101,200,202};value=MotionBinary64::fromInteger(positions[views++]);return true;
    }
    bool character(unsigned token,MotionBinary64 x,MotionBinary64 y){
        std::int32_t px,py;assert(x.roundPixel(px)&&y.roundPixel(py));
        tokens.push_back(token);xs.push_back(px);ys.push_back(py);return tokens.size()!=failCharacter;
    }
    bool end(){++ends;return true;}
};
int main(){
    auto read=[](unsigned i,unsigned& token){token=100+i;return true;};
    LevelNumber<8> title;
    assert(title.create(3,MotionBinary64::fromInteger(5),read));
    assert(title.shown==1&&title.i==4&&title.alarms.get(0)==4);
    Draw absent;absent.surface=false;
    assert(title.draw(absent)&&!absent.views&&!absent.ends&&title.characters[1].offset==-2);
    Draw first;
    assert(title.draw(first)&&first.tokens==std::vector<int>{101}&&first.xs[0]==77&&first.ys[0]==80&&
        first.views==4&&first.ends==1&&title.characters[1].offset==-1);
    for(unsigned n=0;n<3;++n){Draw next;assert(title.draw(next));}
    assert(title.characters[1].offset==1&&title.characters[2].offset==-2);
    unsigned destroyed=0;
    for(unsigned tick=1;tick<=142;++tick){
        for(unsigned alarm=0;alarm<12;++alarm)assert(title.advanceAlarm(alarm,[&]{++destroyed;return true;}));
        if(tick==4)assert(title.shown==2&&title.characters[1].enabled==1);
        if(tick==8)assert(title.shown==3&&title.characters[2].enabled==1);
        if(tick==12)assert(title.alarms.get(1)==119);
        if(tick==131)assert(title.exit==1&&title.alarms.get(2)==3);
        if(tick==134)assert(title.exit==2&&title.characters[3].enabled==1);
        assert(destroyed==unsigned(tick==142));
    }
    assert(title.destroyRequested);
    LevelNumber<8> exiting;assert(exiting.create(3,MotionBinary64::fromInteger(5),read));
    exiting.exit=1;exiting.characters[1].offset=3;
    Draw exitDraw;assert(exiting.draw(exitDraw)&&exiting.characters[1].offset==4&&
        exitDraw.tokens==std::vector<int>({102,103}));
    LevelNumber<8> failed;assert(failed.create(3,{},read));failed.shown=3;
    Draw rejected;rejected.failCharacter=2;
    assert(!failed.draw(rejected)&&!failed.good&&failed.characters[1].offset==-1&&
        failed.characters[2].offset==-2&&!rejected.ends&&failed.i==2);
    LevelNumber<2> overflow;assert(!overflow.create(3,{},read));
    LevelNumber<8> retirement;assert(retirement.create(1,{},read));
    retirement.exit=1;retirement.alarms.set(2,1);unsigned retireCalls=0;
    assert(!retirement.advanceAlarm(2,[&]{++retireCalls;return false;})&&!retirement.good&&
        !retirement.destroyRequested&&retirement.alarms.get(2)==0&&retireCalls==1);
    assert(!retirement.advanceAlarm(2,[&]{++retireCalls;return true;})&&retireCalls==1);
    LevelNumber<8> birthFailure;
    assert(!birthFailure.create(3,{},[](unsigned i,unsigned& token){token=i;return i!=2;})&&
        birthFailure.characters[1].token==1&&birthFailure.i==2&&birthFailure.alarms.get(0)==-1);
    std::puts("Level-number Create, ordered alarms, Draw-owned offsets, view rereads and failure prefixes passed");
}
