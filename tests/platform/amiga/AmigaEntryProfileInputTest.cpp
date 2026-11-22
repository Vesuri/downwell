#include "core/GameplayEntryProfileInput.h"
#include "platform/amiga/AmigaSceneRuntime.h"
#include <cassert>
#include <cstdio>

int main(){
    static_assert(unsigned(AmigaScene::splash)==0&&unsigned(AmigaScene::menu)==1&&
        unsigned(AmigaScene::styleSelect)==2&&unsigned(AmigaScene::gameplay)==3);
    AmigaSceneRuntime scene;
    downwell::GameplayEntryProfileInput tape;
    unsigned counts[4]{},confirmations=0,wellFrames=0;
    for(unsigned tick=0;tick<1200;++tick){
        const unsigned before=unsigned(scene.scene());
        const auto input=tape.sample(before,scene.menuPlayerX());
        if(before==2&&input.fire){assert(scene.styleYOffset()==0);++confirmations;}
        if(before==1&&scene.menuEnterSequenceActive())++wellFrames;
        ++counts[before];
        scene.step(input);
        if(counts[3]==downwell::GameplayEntryProfileInput::gameplayFrames)break;
    }
    assert(counts[0]&&counts[1]&&counts[2]&&wellFrames&&confirmations==1);
    assert(counts[3]==120&&scene.scene()==AmigaScene::gameplay);
    assert(counts[0]==12&&counts[1]==375&&counts[2]==184&&wellFrames==73);
    std::printf("Natural entry input tape: splash=%u garden=%u style=%u gameplay=%u well=%u confirmations=%u\n",
        counts[0],counts[1],counts[2],counts[3],wellFrames,confirmations);
}
