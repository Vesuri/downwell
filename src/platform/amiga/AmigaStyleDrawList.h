#ifndef DOWNWELL_AMIGA_STYLE_DRAW_LIST_H
#define DOWNWELL_AMIGA_STYLE_DRAW_LIST_H
#include "AmigaSelectionLayer.h"
#include "AmigaSceneRuntime.h"

// Same panel/carousel order and integer placement as AmigaStyleRenderer.
// Common vertical translation belongs to the DMA viewport, not these commands.
struct AmigaStyleDrawList {
    static constexpr int baseView = 160, center = baseView + 142;
    AmigaSelectionLayer::Command commands[8];
    unsigned count = 0;
    bool build(const AmigaSceneRuntime& scenes) {
        count = 0;
        if (scenes.styleCursor() < 0 || scenes.styleCursor() > 4) return false;
        const auto add = [&](unsigned id,unsigned frame,int x,int y) {
            commands[count++] = {id,frame,x,y,-1};
        };
        if (scenes.styleSelected()) {
            add(65500,scenes.styleRunFrame(),80,center-14+scenes.stylePlayerDescent());
            return true;
        }
        add(scenes.styleCursor()==0 ? 65410 : 65411,0,0,center-80);
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
        return true;
    }
};
#endif
