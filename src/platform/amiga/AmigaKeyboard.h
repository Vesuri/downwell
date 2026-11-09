#ifndef DOWNWELL_AMIGA_KEYBOARD_H
#define DOWNWELL_AMIGA_KEYBOARD_H
#include <cstdint>

// CIA serial decoding is separate from interrupt ownership for host verification.
struct AmigaCancelKeys {
    volatile bool held=false, pressed=false, quit=false, escape=false, pause=false;
    void receive(std::uint8_t serial) {
        const auto inverse=std::uint8_t(~serial);
        const auto code=std::uint8_t((inverse>>1)|(inverse<<7));
        const bool down=(code&0x80)==0;
        if((code&0x7f)==0x32) { // X
            if(down&&!held)pressed=true;
            held=down;
        }
        if(code==0x45){quit=true;escape=true;} // Escape, never Cancel
        if(code==0x19)pause=true; // P
    }
    // Caller excludes the serial interrupt while acknowledging the latch.
    bool sampleCancel(){const bool value=held||pressed;pressed=false;return value;}
    // W1.10: pressed edges for the pause menu; gameplay consumes Escape
    // as pause instead of quitting.
    // Bit 0: Escape, bit 1: P.
    unsigned samplePause(){const unsigned value=(escape?1u:0u)|(pause?2u:0u);escape=pause=false;quit=false;return value;}
};

class AmigaKeyboard {
public:
    ~AmigaKeyboard(){release();}
    bool acquire();
    void release();
    bool cancel();
    bool quit()const;
    // Escape (bit 0) or P (bit 1) since the last call; clears the quit latch.
    unsigned pause();
private:
    bool owned_=false;
};
#endif
