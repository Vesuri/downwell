#include "AmigaCreditsScene.h"
#include "framework/Bitmap.h"
bool AmigaCreditsScene::prepare(const Bitmap& frame,const Sprite& blank,const Palette& palette) {
    frame_ = nullptr;
    if (!frame.data || !list_.buildWide(frame,blank,palette)) return false;
    frame_ = static_cast<const UBYTE*>(frame.data);
    return true;
}
