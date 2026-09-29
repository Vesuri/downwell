#ifndef DOWNWELL_AMIGA_DAMAGE_TEXT_H
#define DOWNWELL_AMIGA_DAMAGE_TEXT_H
#include "AmigaRenderer.h"
#include "core/DamageNumberText.h"

// All four black outline passes precede every white glyph in the source.
// Their union can therefore be published once, without generic asset lookup
// or repeated masked writes. No pixels are scaled or converted to planar.
struct AmigaDamageText {
    static void draw(AmigaRenderer& renderer, const AmigaDamageGlyphs& glyphs,
                     const downwell::DamageNumberText& label, int x, int y) {
        if (!label.length || label.length > 12) return;
        UWORD rows[12][8] = {};
        const unsigned words = (label.length+1)/2+2;
        for (unsigned i=0;i<label.length;++i) {
            const unsigned digit = label.text[i]=='-' ? 10 : unsigned(label.text[i]-'0');
            if (digit>10) return;
            const unsigned shift = (i&1) ? 0 : 8;
            for (unsigned row=0;row<10;++row)
                rows[row+1][1+i/2] |= UWORD(glyphs.rows[digit][row]) << shift;
        }
        const int left=x-int(label.length)*4-16;
        for (unsigned row=0;row<12;++row) {
            for (unsigned word=0;word<words;++word) {
                const UWORD white=rows[row][word];
                UWORD mask=white | UWORD(white<<1) | UWORD(white>>1);
                if (row) mask |= rows[row-1][word];
                if (row+1<12) mask |= rows[row+1][word];
                if (word) mask |= UWORD(rows[row][word-1]<<15);
                if (word+1<words) mask |= UWORD(rows[row][word+1]>>15);
                if (mask) amigaRendererDrawPlanarWord(&renderer,left+int(word)*16,
                                                       y+int(row)-1,mask,0,white);
            }
        }
    }
};
#endif
