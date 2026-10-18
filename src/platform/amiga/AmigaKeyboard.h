#ifndef DOWNWELL_AMIGA_KEYBOARD_H
#define DOWNWELL_AMIGA_KEYBOARD_H
#include <cstdint>

// CIA serial decoding is separate from interrupt ownership for host verification.
struct AmigaCancelKeys {
    volatile bool held=false, pressed=false, quit=false;
    void receive(std::uint8_t serial) {
        const auto inverse=std::uint8_t(~serial);
        const auto code=std::uint8_t((inverse>>1)|(inverse<<7));
        const bool down=(code&0x80)==0;
        if((code&0x7f)==0x32) { // X
            if(down&&!held)pressed=true;
            held=down;
        }
        if(code==0x45)quit=true; // Escape, never Cancel
    }
    // Caller excludes the serial interrupt while acknowledging the latch.
    bool sampleCancel(){const bool value=held||pressed;pressed=false;return value;}
};

class AmigaKeyboard {
public:
    ~AmigaKeyboard(){release();}
    bool acquire();
    void release();
    bool cancel();
    bool quit()const;
private:
    bool owned_=false;
};
#endif
