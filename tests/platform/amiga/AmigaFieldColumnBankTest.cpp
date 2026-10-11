#include "platform/amiga/AmigaFieldColumnBank.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
int main(int argc,char** argv){
    assert(argc==2);std::ifstream file(argv[1],std::ios::binary);
    std::vector<unsigned char> source{std::istreambuf_iterator<char>(file),{}};
    assert(source.size()==downwell::fieldColumnBytes);
    std::vector<downwell::FieldColumnWord> storage(source.size()/8);
    auto* bytes=reinterpret_cast<unsigned char*>(storage.data());
    AmigaFieldColumnBank bank;unsigned reads=0;bool fail=false,corrupt=false;
    auto load=[&](){return bank.load(bytes,source.size(),[&](unsigned char* p,unsigned n){
        ++reads;std::memcpy(p,source.data(),fail?n/2:n);
        if(corrupt)p[n/2]^=1;return !fail;
    });};
    assert(!bank.data());assert(load()&&reads==1);
    assert(std::memcmp(bank.data(),downwell::fieldColumnWords,source.size())==0);
    assert(load()&&reads==1); // no additional preparation while scene owns it
    bank.invalidate();assert(!bank.data());fail=true;
    assert(!load()&&!bank.data());fail=false;corrupt=true;
    assert(!load()&&!bank.data());corrupt=false;
    assert(load()&&reads==4);
    assert(std::memcmp(bank.data(),downwell::fieldColumnWords,source.size())==0);
    bank.invalidate();assert(load()&&reads==5); // scene re-entry
    assert(!bank.load(bytes,source.size()-8,[](auto*,unsigned){assert(false);return true;}));
    bank.invalidate();assert(!bank.load(nullptr,source.size(),[](auto*,unsigned){return true;}));
    assert(!bank.data());
    std::puts("Field columns: exact offline commands, absent/reuse/re-entry and failed/corrupt reads passed");
}
