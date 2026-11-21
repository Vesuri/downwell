#ifndef DOWNWELL_AMIGA_SOUND_BANKS_H
#define DOWNWELL_AMIGA_SOUND_BANKS_H
// W2.5: the scene-local sound banks and the Paula mixer that plays them.
//
// tools/pack-amiga-sfx.py splits the bank into four DWSX packs, each sound
// in exactly one:
// - run (Downwell.sndrun): gameplay, shops and pause, plus what they share
//   with the clear screen and results. Resident from gameplay entry until
//   the run leaves for the front end, so it stays across Retry, pause,
//   shops, the clear screen and results.
// - front (Downwell.sndfront): title, garden, style select and menus; only
//   while no run bank is resident (the cavern atlas and the run bank leave
//   no room for it).
// - results (Downwell.sndresults) and clear (Downwell.sndclear): those
//   screens' own sounds, loaded over the run bank while the screen shows.
//
// Each pack is its own Chip allocation, loaded whole. A pack is loaded only
// when Chip keeps a margin after it (a 512 KiB machine stays silent rather
// than starve the game); a failed load leaves that bank silent and counts in
// gDownwellSoundBankFailures, never failing the game. Changing a slot first
// silences its voices (AmigaPaulaAudio::unbind), then frees the memory.
#include "AmigaPaulaAudio.h"
#include <exec/types.h>
class AmigaPlatform;

namespace downwell {
class AmigaSoundBanks {
public:
    enum Bank : UBYTE { none=0, run, front, results, clear };
    // Chip kept free beyond a pack, in the largest block.
    static constexpr ULONG chipMargin=16384;
    AmigaSoundBanks()=default;
    ~AmigaSoundBanks(){release();}
    AmigaSoundBanks(const AmigaSoundBanks&)=delete;
    AmigaSoundBanks& operator=(const AmigaSoundBanks&)=delete;
    // After takeover. False leaves the game silent (no admission).
    bool acquire(AmigaPlatform& platform);
    // Before platform.restore(): silences Paula and frees every pack.
    void release();
    // The resident slot holds run or none; the scene slot front, results,
    // clear or none. Same bank: nothing happens.
    void resident(Bank bank){set(AmigaPaulaAudio::resident,bank);}
    void scene(Bank bank){set(AmigaPaulaAudio::scene,bank);}
    Bank bank(AmigaPaulaAudio::Slot slot)const{return slots_[slot].bank;}
    AmigaPaulaAudio& audio(){return audio_;}
private:
    struct Slot { Bank bank=none; UBYTE* memory=nullptr; ULONG bytes=0; };
    void set(AmigaPaulaAudio::Slot slot,Bank bank);
    void free(AmigaPaulaAudio::Slot slot);
    void publishState();
    AmigaPlatform* platform_=nullptr;
    AmigaPaulaAudio audio_;
    Slot slots_[2];
};
}
#endif
