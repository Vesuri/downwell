#include "core/WallEndStep.h"
#include <cassert>
#include <vector>
#include <cstdio>
#include <cstring>
using namespace downwell;
struct Ops {
    unsigned neighbors=0,diagonals=0,places=0,points=0,flowers=0,tiles=0,destroys=0,deactivations=0;
    unsigned fail=0,calls=0;
    std::vector<int> trace;
    bool call(int tag){trace.push_back(tag);return ++calls!=fail;}
    bool outOfView(){return call(1);}
    bool place(int x,int y,int object,bool& hit){
        const int dx[]={0,16,0,-16},dy[]={-16,0,16,0};
        assert(places<4&&x==dx[places]&&y==dy[places]&&object==60);
        hit=(neighbors>>places++)&1;return call(2);
    }
    bool point(int x,int y,int object,bool& hit){
        const int dx[]={-16,16,-16,16},dy[]={-16,-16,16,16};
        assert(points<4&&x==dx[points]&&y==dy[points]&&object==60);
        hit=(diagonals>>points++)&1;return call(3);
    }
    bool image(int frame){assert(frame>=0&&frame<=15);return call(100+frame);}
    bool tile(int bg,int sx,int sy,int w,int h,int dx,int dy,int depth){
        assert(bg==739&&sx==48&&sy==48&&w==16&&h==16&&dx==-8&&dy==-8&&depth==100);
        ++tiles;return call(4);
    }
    bool destroy(){++destroys;return call(5);}
    bool deactivate(){++deactivations;return call(6);}
    bool create(int object,int dx,int dy){assert(object==379&&dx==0&&dy==-16);++flowers;return call(7);}
};
int main(int argc,char** argv){
    {
        WallEndStepState s;s.checked=2;s.imageIndex=14;
        assert(wallRoomCreation(676,s)&&s.pillar==-1&&s.checked==2&&s.imageIndex==14);
        assert(wallRoomCreation(677,s)&&s.pillar==1);
        assert(wallRoomCreation(678,s)&&s.pillar==-1);
        assert(wallRoomCreation(-1,s)&&s.pillar==-1);
        assert(!wallRoomCreation(679,s)&&s.pillar==-1);
    }
    const bool dump=argc==2&&std::strcmp(argv[1],"--oracle")==0;
    if(argc!=1&&!dump)return 2;
    const int right[]={15,7,2,7,7,7,6,7,15,15,15,15,15,15,15,15};
    const int left[]={15,13,15,15,13,13,15,15,8,13,15,15,12,13,15,15};
    unsigned cases=0;
    for(int area=0;area<6;++area)for(int pillar=-1;pillar<=1;++pillar)
    for(unsigned neighbors=0;neighbors<16;++neighbors)for(unsigned diagonals=0;diagonals<16;++diagonals){
        WallEndStepState state;state.pillar=pillar;
        Ops ops;ops.neighbors=neighbors;ops.diagonals=diagonals;
        GameMakerRandom random(cases),oracle(cases);
        const int raw=pillar<0?left[neighbors]:pillar>0?right[neighbors]:int(neighbors);
        const bool enclosed=raw==15&&diagonals==15;
        const bool choice=area==4&&(raw==0||raw==2||raw==6||raw==8||raw==10||raw==12||raw==14);
        const int choiceIndex=choice?int(oracle.chooseIndexSmall(3)):-1;
        const bool flower=choiceIndex==2;
        const int final=area==0?((raw==0||raw==2||raw==8||raw==10||raw==14)?14:
            (raw==6||raw==12?raw:15)):raw;
        assert(wallEndStep(state,area,random,ops));
        assert(state.checked==1&&state.neighbors==raw&&state.imageIndex==final&&state.checkThing==60&&
            state.surrounded==enclosed&&state.lights==(raw==15?(15u^diagonals):0)&&
            state.checkDistance==(raw==15?16:0)&&ops.places==4&&ops.points==(raw==15?4u:0u)&&
            ops.flowers==unsigned(flower)&&!ops.tiles&&!ops.destroys);
        assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        const auto emit=[&](unsigned phase,const Ops& o){
            if(!dump)return;
            std::printf("[%d,%d,%u,%u,%d,%u,[%d,%d,%d,%d,%d,%u,%u],%u,[",
                area,pillar,neighbors,diagonals,choiceIndex,phase,state.checked,state.neighbors,state.imageIndex,
                state.checkThing,state.checkDistance,unsigned(state.surrounded),state.lights,unsigned(random.drawCount()));
            for(unsigned i=0;i<o.trace.size();++i)std::printf("%s%d",i?",":"",o.trace[i]);
            std::puts("]]");
        };
        emit(0,ops);
        // A request made after inspection is processed on the next End Step.
        state.deactivate=true;Ops next;const auto draws=random.drawCount();
        assert(wallEndStep(state,area,random,next));
        std::vector<int> expected;if(area==5)expected.push_back(1);
        if(enclosed){expected.push_back(4);expected.push_back(5);}expected.push_back(6);
        assert(next.trace==expected&&state.checked==(enclosed?1:2)&&random.drawCount()==draws);
        emit(1,next);
        // Every failing callback leaves exactly the successful execution's
        // call prefix; a failed query does not install its answer into state.
        for(unsigned fail=1;fail<=ops.calls;++fail){
            WallEndStepState partial;partial.pillar=pillar;
            Ops rejected;rejected.neighbors=neighbors;rejected.diagonals=diagonals;rejected.fail=fail;
            GameMakerRandom r(cases);
            assert(!wallEndStep(partial,area,r,rejected));
            assert(rejected.trace==std::vector<int>(ops.trace.begin(),ops.trace.begin()+fail));
            assert(r.drawCount()==unsigned(rejected.trace.back()==7));
            if(fail==(area==5?2u:1u))assert(partial.neighbors==0);
        }
        ++cases;
    }
    for(unsigned fail=1;fail<=4;++fail){
        WallEndStepState state;state.checked=1;state.surrounded=true;state.deactivate=true;
        Ops ops;ops.fail=fail;GameMakerRandom random;
        assert(!wallEndStep(state,5,random,ops)&&ops.calls==fail&&random.drawCount()==0);
        const std::vector<int> expected{1,4,5,6};
        assert(ops.trace==std::vector<int>(expected.begin(),expected.begin()+fail));
    }
    for(bool enclosed:{false,true}){
        WallEndStepState state;state.checked=1;state.surrounded=enclosed;
        Ops ops;GameMakerRandom random;
        assert(wallEndStep(state,1,random,ops));
        assert(ops.trace==(enclosed?std::vector<int>{4,5}:std::vector<int>{}));
        if(!enclosed){
            state.deactivate=true;Ops third;
            assert(wallEndStep(state,1,random,third)&&third.trace.empty()&&state.checked==2);
        }
    }
    if(!dump)std::printf("Wall End Step: %u source-branch combinations, next-event ordering and callback failures passed\n",cases);
}
