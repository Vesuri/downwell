#include "AmigaKeyboard.h"
#include <proto/exec.h>
#include <exec/interrupts.h>
#include <exec/nodes.h>
#include <resources/cia.h>
#include <proto/cia.h>
#include <hardware/cia.h>

namespace {
Library* base=nullptr;
Interrupt vector{}, *previous=nullptr;
AmigaCancelKeys keys;
bool active=false;
ULONG keyboardHandler(){
    auto* const sdr=reinterpret_cast<volatile UBYTE*>(0xbfec01);
    auto* const cra=reinterpret_cast<volatile UBYTE*>(0xbfee01);
    const UBYTE serial=*sdr;
    // Revs' CIA handshake: >=85 us on a 68000 (RKM Appendix G).
    *cra|=CIACRAF_SPMODE;
    for(volatile UWORD delay=0;delay<200;++delay){}
    *cra&=UBYTE(~CIACRAF_SPMODE);
    keys.receive(serial);
    return 0;
}
}

bool AmigaKeyboard::acquire(){
    if(owned_)return true;
    if(active)return false;
    base=static_cast<Library*>(OpenResource((CONST_STRPTR)CIAANAME));
    if(!base)return false;
    keys.held=false;keys.pressed=false;keys.quit=false;keys.escape=false;keys.pause=false;
    keys.left=false;keys.right=false;keys.fire=false;keys.firePressed=false;
    vector.is_Node.ln_Type=NT_INTERRUPT;
    vector.is_Node.ln_Pri=0;
    vector.is_Node.ln_Name=(char*)"Downwell keyboard";
    vector.is_Data=nullptr;
    vector.is_Code=(void(*)())keyboardHandler;
    // Same ownership model as Revs; restore the displaced OS vector on exit.
    Disable();
    previous=AddICRVector(base,CIAICRB_SP,&vector);
    if(previous){
        RemICRVector(base,CIAICRB_SP,previous);
        if(AddICRVector(base,CIAICRB_SP,&vector)){
            AddICRVector(base,CIAICRB_SP,previous);
            previous=nullptr;Enable();return false;
        }
    }
    owned_=active=true;
    Enable();return true;
}
void AmigaKeyboard::release(){
    if(!owned_)return;
    Disable();
    RemICRVector(base,CIAICRB_SP,&vector);
    if(previous)AddICRVector(base,CIAICRB_SP,previous);
    previous=nullptr;owned_=active=false;
    Enable();
}
bool AmigaKeyboard::cancel(){
    Disable();const bool value=keys.sampleCancel();Enable();return value;
}
void AmigaKeyboard::controls(downwell::Input& input){
    if(!owned_)return;
    Disable();keys.sampleControls(input);Enable();
}
bool AmigaKeyboard::quit()const{return owned_&&keys.quit;}
unsigned AmigaKeyboard::pause(){
    if(!owned_)return 0;
    Disable();const unsigned value=keys.samplePause();Enable();return value;
}
