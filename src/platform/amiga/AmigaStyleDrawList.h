#ifndef DOWNWELL_AMIGA_STYLE_DRAW_LIST_H
#define DOWNWELL_AMIGA_STYLE_DRAW_LIST_H
#include "AmigaSelectionLayer.h"
#include "AmigaSceneRuntime.h"

// Source-order carousel and individual labels, as in the full style oracle.
// Common vertical translation belongs to the DMA viewport, not these commands.
struct AmigaStyleDrawList {
    static constexpr int baseView = 160, center = baseView + 142;
    AmigaSelectionLayer::Command commands[10];
    unsigned count = 0;
    bool build(const AmigaSceneRuntime& scenes) {
        count = 0;
        if (scenes.styleCursor() < 0 || scenes.styleCursor() > 4) return false;
        const auto add = [&](unsigned id,unsigned frame,int x,int y,int pen=-1) {
            commands[count++] = {id,frame,x,y,pen};
        };
        if (scenes.styleSelected()) {
            add(65500,scenes.styleRunFrame(),80,center-14+scenes.stylePlayerDescent());
            return true;
        }
        for(int style=0;style<5;++style) {
            const int x=80+style*48-scenes.styleSmoothX();
            if(x<=-16 || x>=176) continue;
            const int distance=x-80;
            add(style==0 ? 65500 : 65501,
                style==0 && style==scenes.styleCursor() ? scenes.styleRunFrame() : 0,
                x,center-14-distance*distance/200);
        }
        if(scenes.styleCursor()<4) add(65502,0,96,center-10);
        if(scenes.styleCursor()>0) add(65503,0,64,center-10);
        // Separate bounds keep actor damage out of the static title and keep
        // a label change from clearing the whole 160x128 panel rectangle.
        add(65000,0,80,center-70);
        add(scenes.styleCursor()==0 ? 65001 : 65003,0,80,center+10);
        if(scenes.styleCursor()==0) add(65002,0,80,center+32,1);
        return true;
    }
};
#endif
