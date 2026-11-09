#ifndef DOWNWELL_AMIGA_PAUSE_OVERLAY_H
#define DOWNWELL_AMIGA_PAUSE_OVERLAY_H
#include "AmigaRenderer.h"
#include "core/PauseMenu.h"

// W1.10: objPauseMenu and its sub-menus on the fixed PF2 layer, above the
// frozen world and hardware sprites. PF2 pens: 1 white, 2 red, 3 opaque
// black, 0 transparent. Text uses the pause bank's font-2 masks at
// 64800 + code (eight-pixel advances, top-left origins); the PALETTE
// screen's runner and gem come from the resident game bank. Each change is
// composed by the CPU in a scratch copy of the HUD layer and then copied
// over the shown rows, so the well never passes through an empty
// (transparent) state while the beam is drawing it.
class AmigaPauseOverlay {
public:
    struct Packs { const UBYTE *pause, *pauseEnd, *game, *gameEnd; };
    static constexpr unsigned glyphBase = 64800;

    static constexpr unsigned scratchBytes = downwellLayerBytes;
    // Seed the scratch layer from the shown HUD (side columns included).
    // The caller retires blits first.
    static void begin(const AmigaRenderer* renderer, UBYTE* scratch) {
        const ULONG* from = reinterpret_cast<const ULONG*>(renderer->hud);
        ULONG* to = reinterpret_cast<ULONG*>(scratch);
        for (unsigned i = 0; i < scratchBytes / 4; ++i) to[i] = from[i];
    }
    // Recompose [top, bottom) of the well. The caller retires blits first.
    static void compose(AmigaRenderer* renderer, UBYTE* scratch, const Packs& packs,
                        downwell::PauseMenu& menu, const downwell::PauseMenu::View& view,
                        int top, int bottom) {
        AmigaRenderer off = *renderer;
        off.hud = scratch;
        off.blitterEnabled = 0;
        // SetClipRect selects the world layer: retarget the HUD after it.
        amigaRendererSetClipRect(&off, downwellPlayfieldX, top, 160, bottom - top);
        off.target = off.hud;
        amigaRendererFillRect(&off, downwellPlayfieldX, top, 160, bottom - top, 0);
        Sink sink{&off, packs};
        (void)menu.draw(sink, view);
        // Rows are whole: the side columns carry the shown HUD unchanged.
        for (unsigned plane = 0; plane < 2; ++plane) {
            const unsigned first = plane * downwellPlaneBytes + unsigned(top) * downwellBytesPerPlaneLine;
            const ULONG* from = reinterpret_cast<const ULONG*>(scratch + first);
            ULONG* to = reinterpret_cast<ULONG*>(renderer->hud + first);
            for (unsigned i = 0; i < unsigned(bottom - top) * downwellBytesPerPlaneLine / 4; ++i) to[i] = from[i];
        }
    }
    // Resume: give the well back to the HUD's own overlays (empty PF2).
    static void clear(AmigaRenderer* renderer) {
        UBYTE* saved = renderer->target;
        renderer->target = renderer->hud;
        amigaRendererClearClipRect(renderer);
        amigaRendererFillRect(renderer, downwellPlayfieldX, 0, 160, 284, 0);
        renderer->target = saved;
    }
    // Everything the composed screen depends on except the palette
    // screen's runner animation.
    struct Key {
        int screen = -1, cursor = 0, powan = 0, whole = 0, whole2 = 0, powanx = 0, confirm = 0;
        unsigned shader = 0, gain = 0; bool noBgm = false, timer = false;
        bool operator==(const Key& o) const {
            return screen == o.screen && cursor == o.cursor && powan == o.powan && whole == o.whole &&
                whole2 == o.whole2 && powanx == o.powanx && confirm == o.confirm && shader == o.shader &&
                gain == o.gain && noBgm == o.noBgm && timer == o.timer;
        }
        bool operator!=(const Key& o) const { return !(*this == o); }
    };
    static Key key(const downwell::PauseMenu& m, const downwell::SaveFile& s) {
        Key k;
        k.screen = int(m.screen); k.cursor = m.cursor; k.powan = m.powan(); k.whole = m.wholePowan;
        k.whole2 = m.wholePowan2; k.powanx = m.powanx; k.confirm = int(m.confirmType);
        k.shader = s.shader; k.gain = s.masterGainTenths; k.noBgm = s.noBgm; k.timer = s.showTimer;
        return k;
    }

private:
    struct Sink {
        AmigaRenderer* renderer;
        Packs packs;
        void dither(int frame, int x, int y) {
            amigaRendererDrawAsset(renderer, packs.pause, packs.pauseEnd, 733, UWORD(frame),
                                   downwellPlayfieldX + x, y, 0);
        }
        void blackRect(int x, int y, int w, int h) {
            amigaRendererFillRect(renderer, downwellPlayfieldX + x, y, w, h, 3);
        }
        void progress(int x, int y) {
            amigaRendererDrawAsset(renderer, packs.pause, packs.pauseEnd, 402, 0,
                                   downwellPlayfieldX + x, y, 0);
        }
        void line(int x, int y, int w, bool white) {
            if (w > 0) amigaRendererFillRect(renderer, downwellPlayfieldX + x, y, w, 1, white ? 1 : 2);
        }
        void sprite(int id, int frame, int x, int y) {
            int bx, by, bw, bh;
            if (!packs.game || !amigaRendererAssetBounds(packs.game, packs.gameEnd, UWORD(id), UWORD(frame),
                    0, 0, 0, &bx, &by, &bw, &bh)) return;
            amigaRendererDrawAsset(renderer, packs.game, packs.gameEnd, UWORD(id), UWORD(frame),
                                   downwellPlayfieldX + x, y, 0);
        }
        void glyphs(const char* s, int x, int y, int halign, unsigned pen) {
            while (*s) {
                const char* end = s;
                while (*end && *end != '#') ++end;
                const int width = 8 * int(end - s);
                int left = halign < 0 ? x : halign == 0 ? x - width / 2 : x - width;
                for (const char* c = s; c < end; ++c, left += 8)
                    if (*c > ' ' && *c < 127)
                        amigaRendererDrawAssetPen(renderer, packs.pause, packs.pauseEnd,
                            UWORD(glyphBase + unsigned(*c)), 0, downwellPlayfieldX + left, y, 0, pen);
                s = *end ? end + 1 : end;
                y += 12;
            }
        }
        void text(const char* s, int x, int y, int halign, downwell::PauseMenu::Kind kind) {
            using K = downwell::PauseMenu::Kind;
            if (!s) return;
            if (kind == K::borderBlack || kind == K::borderRed) {
                glyphs(s, x + 1, y, halign, 3); glyphs(s, x - 1, y, halign, 3);
                glyphs(s, x, y + 1, halign, 3); glyphs(s, x, y - 1, halign, 3);
            }
            glyphs(s, x, y, halign, kind == K::red || kind == K::borderRed ? 2 : 1);
        }
    };
};
#endif
