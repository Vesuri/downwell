#include "platform/amiga/AmigaKeyboard.h"
#include <exec/interrupts.h>
#include <exec/types.h>
#include <cassert>
#include <cstring>
#include <cstdio>
struct Library{};
static Library resource;
static Interrupt original{},*installed=&original;
static bool present=true,deny=false;
static unsigned disabled=0,removed=0;
void* OpenResource(CONST_STRPTR name){
    assert(!std::strcmp(name,"ciaa.resource"));return present?&resource:nullptr;
}
void Disable(){++disabled;}
void Enable(){assert(disabled);--disabled;}
Interrupt* AddICRVector(Library* base,unsigned bit,Interrupt* vector){
    assert(base==&resource&&bit==3&&disabled);
    if(installed)return installed;
    if(deny&&vector!=&original)return &original;
    installed=vector;return nullptr;
}
void RemICRVector(Library* base,unsigned bit,Interrupt* vector){
    assert(base==&resource&&bit==3&&installed==vector&&disabled);
    installed=nullptr;++removed;
}
static std::uint8_t serial(unsigned code){return std::uint8_t(~((code<<1)|(code>>7)));}
int main(){
    present=false;{AmigaKeyboard k;assert(!k.acquire());}
    present=true;deny=true;{AmigaKeyboard k;assert(!k.acquire());}
    assert(installed==&original&&!disabled);deny=false;
    {
        AmigaKeyboard k;assert(k.acquire()&&k.acquire());
        assert(installed!=&original&&installed&&installed->is_Code);
        {AmigaKeyboard second;assert(!second.acquire());}
        assert(!k.cancel()&&!k.quit());
        k.release();k.release();assert(installed==&original);
        assert(k.acquire());
    }
    assert(installed==&original&&!disabled&&removed==5);
    AmigaCancelKeys keys;
    keys.receive(serial(0x32));assert(keys.sampleCancel()&&keys.sampleCancel());
    keys.receive(serial(0xb2));assert(!keys.sampleCancel());
    keys.receive(serial(0x32));keys.receive(serial(0xb2));
    assert(keys.sampleCancel()&&!keys.sampleCancel());
    keys.receive(serial(0x19));assert(!keys.sampleCancel()&&!keys.quit); // P
    keys.receive(serial(0xc5));assert(!keys.quit);
    keys.receive(serial(0x45));assert(keys.quit&&!keys.sampleCancel());
    std::puts("CIA keyboard ownership, rollback, restoration, X taps/holds and Escape passed");
}
