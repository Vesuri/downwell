#include "platform/amiga/AmigaGameplayScene.h"
#include "platform/amiga/AmigaEntryMask.h"
#include <cassert>
#include <initializer_list>
#include <cstdio>
#include <cstdint>
// Lifecycle doubles. Native gates exercise actual Copper/ring behavior.
class Bitmap {};
class Sprite {};
class Palette {};
static unsigned builds, updates, masks;
static bool buildOK=true, updateOK=true, maskOK=true;
static const Bitmap* bound;
static const UBYTE* lastMask;
CopperList::CopperList(UWORD* words,unsigned capacity)
    : words_(words),capacity_(capacity),count_(0),valid_(words!=nullptr) {}
DownwellGameplayCopperList::DownwellGameplayCopperList(UWORD* words,unsigned capacity)
    : CopperList(words,capacity) {}
bool DownwellGameplayCopperList::build(const Bitmap& game,const Bitmap&,const Sprite&,
    const Sprite*,unsigned,const Sprite&,const Palette&,unsigned) {
    ++builds; bound=&game; return buildOK;
}
bool DownwellGameplayCopperList::update(const Bitmap& game,unsigned) {
    ++updates; bound=&game; return updateOK;
}
bool DownwellGameplayCopperList::updateExitMask(const UBYTE* mask) {
    ++masks; lastMask=mask; return maskOK;
}
int main() {
    UWORD words[2][400]{};
    Bitmap backing, display[2], hud;
    Sprite player, actors[2], overflow;
    Palette palette;
    UBYTE mask[8]{};
    AmigaGameplayScene scene(words[0],words[1],400);
    auto update=[&](unsigned slot) {
        return scene.update(slot,display[slot%2],hud,player,actors,2,overflow,palette,17);
    };
    assert(!scene.mask(0,mask) && masks==0);
    assert(!update(2));
    assert(!scene.build(2,backing,hud,player,actors,2,overflow,palette,0));
    for(unsigned slot=0;slot<2;++slot)
        assert(scene.build(slot,backing,hud,player,actors,2,overflow,palette,0));
    scene.invalidate(); // startup backing/borrowed intro slots are no longer valid
    for(unsigned i=0;i<600;++i) {
        const unsigned slot=i%2;
        assert(update(slot) && bound==&display[slot]);
        assert(scene.mask(slot,i<300 ? mask : nullptr));
        assert(lastMask==(i<300 ? mask : nullptr));
        assert(scene.copper(slot).data()==words[slot]);
    }
    assert(builds==4 && updates==598 && masks==600);
    scene.invalidate(0);
    assert(update(1) && builds==4);
    assert(update(0) && builds==5);
    updateOK=false;
    assert(!update(0));
    assert(!scene.mask(0,mask));
    updateOK=true; buildOK=false;
    assert(!update(0));
    buildOK=true;
    unsigned before=builds;
    assert(update(0) && builds==before+1);
    maskOK=false;
    assert(!scene.mask(0,mask));
    before=builds; maskOK=true;
    assert(update(0) && builds==before+1);
    scene.invalidate();
    before=builds;
    assert(update(0) && update(1) && builds==before+2);
    assert(!scene.mask(2,mask));
    alignas(2) static UBYTE entry[AmigaEntryMask::bytes]{};
    before=builds;
    for(int y=-116;y<=800;++y){
        unsigned offset;assert(AmigaEntryMask::offset(y,offset));
        const auto oldMasks=masks;
        assert(scene.entryMask(unsigned(y+116)%2,entry,sizeof(entry),y));
        assert(lastMask==entry+offset&&masks==oldMasks+1&&builds==before);
        assert(offset+284*AmigaEntryMask::stride<=sizeof(entry));
    }
    const auto oldMasks=masks;const auto* oldMask=lastMask;
    assert(!scene.entryMask(0,entry,sizeof(entry),-117));
    assert(!scene.entryMask(0,entry,sizeof(entry),(-2147483647-1)));
    assert(!scene.entryMask(0,entry,sizeof(entry)-1,0));
    assert(!scene.entryMask(0,nullptr,sizeof(entry),0));
    assert(!scene.entryMask(0,entry+1,sizeof(entry),0));
    assert(!scene.entryMask(2,entry,sizeof(entry),0));
    assert(masks==oldMasks&&lastMask==oldMask&&builds==before);
    assert(scene.entryMask(0,entry,sizeof(entry),2147483647)&&lastMask==entry);
    assert(scene.mask(0,nullptr)&&lastMask==nullptr); // explicit retirement
    // Predict only a view being approached. Zero resolved speed includes
    // pressing against a wall, irrespective of the held input direction.
    const int none = AmigaGameplayScene::noStageColumn;
    for (int x=-236; x<=236; ++x) for (int camera : {-160,0,160})
    for (long speed : {-640L,-1L,0L,1L,640L}) {
        const int target=AmigaGameplayScene::stageColumn(x,speed,camera,false);
        const bool approachRight=camera==0 && x>16 && speed>0;
        const bool approachLeft=camera==0 && x<-16 && speed<0;
        const bool returning=(camera<0 && x>-144 && speed>0) ||
                             (camera>0 && x<144 && speed<0);
        assert(target==(approachRight?160:approachLeft?-160:returning?0:none));
        assert(AmigaGameplayScene::stageColumn(x,speed,camera,true)==none);
    }
    // Model the actual retained row contents independently of interval math.
    // Include reversals, column switches, failed copies and ring wraparound.
    int storedY[320], storedX[320];
    for (unsigned i=0;i<320;++i) storedY[i]=storedX[i]=-10000;
    scene.resetStage();
    assert(!scene.stageCovers(160,0,284));
    std::uint32_t random=42;
    int x=160, y=0;
    unsigned covered=0, failed=0, upward=0;
    for (unsigned tick=0;tick<20000;++tick) {
        random=random*1664525U+1013904223U;
        if (tick%137==0) x=(int((random>>16)%3))*160;
        y+=int((random>>8)%7)-3;
        if (y<0) y=0;
        if (tick%191==0) y+=340;
        const auto span=scene.stageSpan(x,y,284);
        assert(span.count>=0 && span.count<=48);
        if (span.first<scene.stageFirst()) ++upward;
        if (tick%53==0) {
            scene.invalidateStage(); ++failed;
            assert(!scene.stageCovers(x,y,284));
            continue;
        }
        for (int row=span.first;row<span.first+span.count;++row) {
            storedY[row%320]=row; storedX[row%320]=x;
        }
        scene.staged(span,320);
        assert(scene.stageEnd()-scene.stageFirst()<=320);
        if (scene.stageCovers(x,y,284)) {
            ++covered;
            for (int row=y;row<y+284;++row)
                assert(storedY[row%320]==row && storedX[row%320]==x);
        }
    }
    assert(covered>10000 && failed>300 && upward>100);
    scene.resetStage();
    assert(scene.stageFirst()==0 && scene.stageEnd()==0);
    assert(!scene.stageCovers(x,y,284));
    // Cold append work may stop after a prefix, but prepends must adjoin the
    // old interval. Exercise both a bounded suffix and rejection of a gap.
    auto cold = scene.stageSpan(160,100,284,16);
    scene.staged({cold.first,16},320);
    cold = scene.stageSpan(160,52,284,16);
    assert(cold.first==84 && cold.count==16);
    scene.staged(cold,320);
    assert(scene.stageCovers(160,84,32));
    scene.staged({52,16},320);
    assert(!scene.stageCovers(160,52,64));
    scene.resetStage();
    std::printf("Staging: 20000 updates, %u covered views, %u failed copies, %u upward strips\n",
                covered,failed,upward);
    // Independent pixel-row identity/revision oracle for the active backing.
    // Damage outside the view must be repaired before it becomes visible.
    int revisions[4096]{}, storedRevision[320]{};
    y=1024;
    scene.setBackingRange(y,y+284);
    for (int row=y;row<y+284;++row) storedY[row%320]=row;
    unsigned unchanged=0, snaps=0;
    for (unsigned tick=0;tick<20000;++tick) {
        random=random*1664525U+1013904223U;
        if (tick%97==0) { y=512+int((random>>8)%2300); ++snaps; }
        else y+=int((random>>8)%7)-3;
        if (y<32) y=32;
        if (y>3600) y=3600;
        const auto exposed=scene.exposeBacking(y,284,320);
        assert(exposed.count>=0 && exposed.count<=320);
        if (!exposed.count) ++unchanged;
        for (int row=exposed.first;row<exposed.first+exposed.count;++row) {
            storedY[row%320]=row; storedRevision[row%320]=revisions[row];
        }
        const int damaged=y+int((random>>16)%360)-32;
        for (int row=damaged;row<damaged+16;++row) {
            ++revisions[row];
            if (row>=y && row<y+284) storedRevision[row%320]=revisions[row];
        }
        scene.invalidateBackingOutside(damaged,damaged+16,y,284);
        assert(scene.backingFirst()<=y && scene.backingEnd()>=y+284);
        for (int row=y;row<y+284;++row)
            assert(storedY[row%320]==row && storedRevision[row%320]==revisions[row]);
    }
    assert(unchanged>10000 && snaps>200);
    scene.setBackingRange(1000,1300);
    auto snap=scene.exposeBacking(2000,284,320);
    assert(snap.first==1992 && snap.count==300);
    snap=scene.exposeBacking(500,284,320);
    assert(snap.first==492 && snap.count==300);
    std::printf("Backing: 20000 updates, %u unchanged views and %u camera snaps\n",unchanged,snaps);
    std::puts("Gameplay scene: retired-slot rebuilds, 600 patched frames, exit masks and failure recovery");
}
