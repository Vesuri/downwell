#ifndef DOWNWELL_AMIGA_DAMAGE_TEXT_H
#define DOWNWELL_AMIGA_DAMAGE_TEXT_H
#include "AmigaRenderer.h"
#include "core/InstalledTables.h"
#include "core/DamageNumberText.h"

struct AmigaPreparedDamageText {
    unsigned first,words;
    UWORD mask[12][3],white[12][3];
};
namespace downwell_damage_bank {
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/amiga-damage-text.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/amiga-damage-text.h"
#endif
}

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

// Backend-owned text composition: no change to simulation records or Draw
// ordering. Eight cached values cover the eight live damage-number slots.
// Invalidate on glyph-bank replacement; misses never consume game RNG.
class AmigaDamageTextCache {
    bool preparedBank_=false;
    struct Entry {
        int value=0;
        bool valid=false;
        unsigned first=0,words=0,length=0;
        UWORD white[12][8]={},mask[12][8]={};
    } entries_[8];
    unsigned next_=0;
    static void prepare(Entry& entry,const AmigaDamageGlyphs& glyphs,int value) {
        downwell::DamageNumberText label;label.formatInteger(value);
        entry=Entry{};
        entry.value=value;entry.length=label.length;
        const unsigned words=(label.length+1)/2+2;
        for(unsigned i=0;i<label.length;++i) {
            const unsigned digit=label.text[i]=='-'?10:unsigned(label.text[i]-'0');
            for(unsigned row=0;row<10;++row)
                entry.white[row+1][1+i/2]|=UWORD(glyphs.rows[digit][row])<<((i&1)?0:8);
        }
        entry.first=words;
        unsigned end=0;
        for(unsigned row=0;row<12;++row) for(unsigned word=0;word<words;++word) {
            const UWORD white=entry.white[row][word];
            UWORD bits=white|UWORD(white<<1)|UWORD(white>>1);
            if(row)bits|=entry.white[row-1][word];
            if(row+1<12)bits|=entry.white[row+1][word];
            if(word)bits|=UWORD(entry.white[row][word-1]<<15);
            if(word+1<words)bits|=UWORD(entry.white[row][word+1]>>15);
            entry.mask[row][word]=bits;
            if(bits) {
                if(word<entry.first)entry.first=word;
                if(word+1>end)end=word+1;
            }
        }
        if(end)entry.words=end-entry.first;
        entry.valid=true;
    }
public:
    void invalidate() {for(auto& entry:entries_)entry.valid=false;next_=0;preparedBank_=false;}
    // Admit the immutable bank once when assets are bound, never during Draw.
    // Different glyphs still use the retained compositor, including tests and
    // future font changes. The caller must invalidate before replacing glyphs.
    bool bindGlyphs(const AmigaDamageGlyphs& glyphs) {
        invalidate();
#ifndef DOWNWELL_REFERENCE_DAMAGE_BANK
        for(unsigned digit=0;digit<11;++digit)for(unsigned row=0;row<10;++row)
            if(glyphs.rows[digit][row]!=DOWNWELL_TABLE_OBJECT(downwell_damage_bank::preparedDamageGlyphs).rows[digit][row])return false;
        preparedBank_=true;
#endif
        return preparedBank_;
    }
    void draw(AmigaRenderer& renderer,const AmigaDamageGlyphs& glyphs,int value,int x,int y) {
        if(preparedBank_&&value>=0&&value<10){
            const auto& entry=downwell_damage_bank::preparedDamageText[value];
            if(entry.words)amigaRendererDrawTextWords(&renderer,x-24+int(entry.first)*16,y-1,
                &entry.mask[0][entry.first],&entry.white[0][entry.first],entry.words,12,3);
            return;
        }
        Entry* found=nullptr;
        for(auto& entry:entries_)if(entry.valid && entry.value==value) {found=&entry;break;}
        if(!found) {
            found=&entries_[next_];next_=(next_+1)%8;
            prepare(*found,glyphs,value);
        }
        if(!found->words)return;
        amigaRendererDrawTextWords(&renderer,x-int(found->length)*4-16+int(found->first)*16,y-1,
            &found->mask[0][found->first],&found->white[0][found->first],found->words,12,8);
    }
};
#endif
