#include "core/BuilderCommonDispatch.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
using namespace downwell;
static void require(bool ok){if(!ok)std::abort();}
struct Ops {
    std::vector<std::string> calls;
    unsigned choice=0, failAt=0;
    bool right=false;
    bool record(std::string text){calls.push_back(text);return !failAt||calls.size()!=failAt;}
    int choose(const int* values,unsigned count){
        std::string text="choose";for(unsigned i=0;i<count;++i)text+=':'+std::to_string(values[i]);
        calls.push_back(text);return values[choice%count];
    }
    template<class Post> bool create(int object,int x,int y,Post post){
        if(!record("create:"+std::to_string(object)+":"+std::to_string(x)+":"+std::to_string(y)))return false;
        if(!record("Create"))return false;
        return post(1234);
    }
    bool write(int child,BuilderChildField field,int value){
        require(child==1234);
        return record("write:"+std::to_string(int(field))+":"+std::to_string(value));
    }
    bool deactivate(int child){require(child==1234);return record("deactivate");}
    bool rightOfMiddle(int child,int width,bool& result){
        require(child==1234&&width==480);result=right;return record("read-x");
    }
};
using Trace=std::vector<std::string>;
static void creation(Trace& t,int object,int x=240){
    t.push_back("create:"+std::to_string(object)+":"+std::to_string(x)+":720");t.push_back("Create");
}
int main(){
    for(auto item:{std::pair<char,int>{'-',93},{'W',103},{'N',329},{'$',277},{'\\',285},
                   {'v',376},{'+',109},{'O',82},{'=',306},{'_',62},{'|',60}}){
        BuilderCommonDispatch state;Ops ops;Trace expected;creation(expected,item.second);
        require(state.cell(item.first,240,720,480,4,0,ops)&&ops.calls==expected);
    }
    for(char c:std::string("123B"))for(unsigned choice=0;choice<2;++choice){
        BuilderCommonDispatch state;Ops ops;ops.choice=choice;Trace expected;
        if(c=='2'||c=='3')expected.push_back("choose:0:1");
        if(c=='1'||c=='B'||choice){
            creation(expected,c=='1'||c=='2'?60:92);
            expected.push_back(c=='1'||c=='2'?"write:0:1":"deactivate");
        }
        require(state.cell(c,240,720,480,4,0,ops)&&ops.calls==expected);
    }
    for(unsigned choice=0;choice<4;++choice){
        BuilderCommonDispatch state;Ops ops;ops.choice=choice;Trace expected{"choose:0:0:0:1"};
        if(choice==3){expected.push_back("choose:254");creation(expected,254);}
        require(state.cell('?',240,720,480,4,0,ops)&&ops.calls==expected);
    }
    for(char c:std::string("{}"))for(int x:{239,240,241}){
        Trace expected;creation(expected,60,x<240?-16:336);
        expected.push_back(c=='{'?"write:1:594":"write:1:595");
        expected.insert(expected.end(),{"write:2:15","write:3:15","write:4:1"});
        for(unsigned fail=0;fail<=expected.size();++fail){
            BuilderCommonDispatch state;Ops ops;ops.failAt=fail;
            require(state.cell(c,x,720,480,4,0,ops)==!fail);
            require(state.sideroomRightmost==(x<240?-16:336));
            require(ops.calls==Trace(expected.begin(),fail?expected.begin()+fail:expected.end()));
            if(fail){const auto before=ops.calls;require(!state.cell('|',240,720,480,4,0,ops)&&ops.calls==before);}
        }
    }
    for(bool right:{false,true}){
        BuilderCommonDispatch state;Ops ops;ops.right=right;Trace expected;creation(expected,60);
        expected.push_back("read-x");expected.push_back(right?"write:5:1":"write:5:-1");
        require(state.pillar(240,720,480,ops)&&ops.calls==expected);
    }
    for(int previous:{0,109,293})for(int health:{2,3})for(unsigned style:{0u,1u})for(unsigned choice=0;choice<6;++choice){
        BuilderCommonDispatch state;state.chunkOrCrate=previous;Ops ops;ops.choice=choice;Trace expected;
        int reward=previous==109?293:109;
        if(!previous){expected.push_back("choose:293:109");reward=choice%2?109:293;}
        if(health<=2&&reward!=109){expected.push_back("choose:0:1:1:1:1:1");if(choice)reward=109;}
        if(style==1)reward=109;
        creation(expected,reward);
        require(state.cell('@',240,720,480,health,style,ops)&&ops.calls==expected&&state.chunkOrCrate==reward);
    }
    std::cout<<"Common builder Create/write/choice traces passed\n";
}
