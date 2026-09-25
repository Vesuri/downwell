#include "AmigaGardenScene.h"
#include "AmigaSceneRuntime.h"
#include "AmigaMenuCache.h"
#include "AmigaProfile.h"
#include "framework/Sprite.h"

AmigaGardenScene::AmigaGardenScene(UBYTE* foreground,UBYTE* background,UBYTE* entry,
    UWORD* first,UWORD* second,unsigned capacity)
    : foreground_(foreground),background_(background),entry_(entry),
      lists_{DownwellMenuCopperList(first,capacity),DownwellMenuCopperList(second,capacity)} {}

bool AmigaGardenScene::prepare(const AmigaMenuDrawList& list,const UBYTE* pack,const UBYTE* end) {
    ready_=false; title_=-1; enter();
    if(!foreground_ || !background_ || !entry_ || !pack || !end ||
        !amigaRenderMenuCache(list,pack,end,foreground_,background_,entry_)) return false;
    pack_=pack; end_=end; ready_=true;
    return true;
}

bool AmigaGardenScene::render(unsigned slot,const AmigaSceneRuntime& scenes,bool entry,
    const Sprite& blank,const Palette& palette,Frame& frame) {
    frame={};
    if(!ready_ || slot>=2) return false;
    DW_PROFILE_PHASE(profileTitle);
    if(title_!=scenes.menuTitleFrame()) {
        if(!amigaRenderMenuTitle(UWORD(scenes.menuTitleFrame()),pack_,end_,foreground_,entry_)) return false;
        title_=scenes.menuTitleFrame();
    }
    DW_PROFILE_PHASE(profileSprite);
    UWORD* player=blank.data();
    if(scenes.menuPlayerVisible()) {
        player=amigaMenuHardwareSprite(pack_,end_,UWORD(scenes.menuPlayerSprite()),
            UWORD(scenes.menuPlayerFrame()),scenes.menuPlayerFacing()<0,slot,
            downwellPlayfieldX+scenes.menuPlayerX()-(scenes.menuCameraX()-80),
            scenes.menuPlayerY()-(scenes.menuCameraY()-142));
        if(!player) return false;
    }
    DW_PROFILE_PHASE(profileScroll);
    const auto scroll=entry ? amigaMenuEntryScrollState(scenes.menuCameraX()-80,scenes.menuCameraY()-142)
                            : amigaMenuScrollState(scenes.menuCameraX()-80,scenes.menuCameraY()-142);
    const auto* foreground=entry ? entry_ : foreground_;
    DW_PROFILE_PHASE(profileCopper);
    if(!built_[slot] && !lists_[slot].buildScroll(foreground,background_,blank,blank,palette,scroll,entry)) return false;
    if(!lists_[slot].updateScroll(foreground,background_,player,scroll,entry)) return false;
    built_[slot]=true;
    frame={player,scroll};
    return true;
}
