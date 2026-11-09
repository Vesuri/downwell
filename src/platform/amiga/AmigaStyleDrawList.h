#ifndef DOWNWELL_AMIGA_STYLE_DRAW_LIST_H
#define DOWNWELL_AMIGA_STYLE_DRAW_LIST_H
#include "AmigaSelectionLayer.h"
#include "AmigaSceneRuntime.h"

// Source-order carousel and individual labels, as in the full style oracle.
// Common vertical translation belongs to the DMA viewport, not these commands.
// PlayMenu Draw: unlocked styles show styleRun[i] (65500 for style 0,
// 65504.. for styles 1-4), locked ones sprLocked; the label is the cursor
// style's name and description, or LOCKED!. With hard mode unlocked the
// skull sits left of style 0 and HARD MODE sits above the title.
struct AmigaStyleDrawList {
    static constexpr int baseView = 160, center = baseView + 142, hardApart = 42;
    static unsigned runId(int style) { return style == 0 ? 65500u : 65503u + unsigned(style); }
    AmigaSelectionLayer::Command commands[12];
    unsigned count = 0;
    bool build(const AmigaSceneRuntime& scenes) {
        count = 0;
        const int cursor = scenes.styleCursor();
        if (cursor < 0 || cursor > 4) return false;
        const auto add = [&](unsigned id,unsigned frame,int x,int y,int pen=-1) {
            commands[count++] = {id,frame,x,y,pen};
        };
        if (scenes.styleSelected()) {
            add(runId(cursor),scenes.styleRunFrame(),80,center-14+scenes.stylePlayerDescent());
            return true;
        }
        for(int style=0;style<5;++style) {
            const int x=80+style*48-scenes.styleSmoothX();
            if(x<=-16 || x>=176) continue;
            const int distance=x-80;
            const bool unlocked=scenes.styleUnlocked(style);
            add(unlocked ? runId(style) : 65501,
                unlocked && style==cursor ? scenes.styleRunFrame() : 0,
                x,center-14-distance*distance/200);
        }
        if(scenes.hardUnlocked()) {
            const int x=80-hardApart-scenes.styleSmoothX();
            if(x>-16 && x<176) add(409,scenes.hardMode()?1:0,x,center-14);
        }
        if(cursor<4) add(65502,0,96,center-10);
        if(cursor>0) add(65503,0,64,center-10);
        // Separate bounds keep actor damage out of the static title and keep
        // a label change from clearing the whole 160x128 panel rectangle.
        if(scenes.hardMode()) add(65030,0,80,center-70+1-14);
        add(65000,0,80,center-70);
        if(scenes.styleUnlocked(cursor)) {
            add(cursor==0 ? 65001 : 65020+unsigned(cursor),0,80,center+10);
            add(cursor==0 ? 65002 : 65025+unsigned(cursor),0,80,center+32,1);
        } else add(65003,0,80,center+10);
        return true;
    }
};
#endif
