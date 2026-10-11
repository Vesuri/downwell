#include "platform/amiga/AmigaFieldBandBank.h"
#include "../../../generated/downwell-field-columns.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
int main(int argc,char** argv){
    assert(argc==2);std::ifstream file(argv[1],std::ios::binary);
    std::vector<unsigned char> source{std::istreambuf_iterator<char>(file),{}};
    assert(source.size()==downwell::fieldBandBytes);
    for(unsigned i=0;i<source.size();i+=2)
        assert(unsigned(source[i])*256+source[i+1]==downwell::fieldBandPixels[i/2]);
    std::vector<unsigned char> target(source.size()+4,0x69);
    AmigaFieldBandBank bank;unsigned reads=0;bool fail=false,shortRead=false,corrupt=false;
    const auto read=[&](unsigned char* p,unsigned bytes){
        ++reads;if(fail)return false;
        if(shortRead){for(unsigned i=0;i<bytes/2;++i)p[i]=source[i];return false;}
        for(unsigned i=0;i<bytes;++i)p[i]=source[i];
        if(corrupt)p[bytes/2]^=1;return true;
    };
    const auto load=[&](){return bank.load(target.data(),target.size()-2,
        source.size(),downwell::fieldBandChecksum,read);};
    assert(load()&&reads==1);assert(load()&&reads==1);
    assert(target[source.size()]==0&&target[source.size()+1]==0);
    assert(target[source.size()+2]==0x69&&target[source.size()+3]==0x69);
    bank.invalidate();fail=true;assert(!load()&&reads==2);
    fail=false;corrupt=true;assert(!load()&&reads==3);
    corrupt=false;assert(load()&&reads==4);
    bank.invalidate();assert(load()&&reads==5);
    // Another scene may overwrite the shared range. A partial read must not
    // publish that mixture or mark it resident; retry must read it afresh.
    bank.invalidate();std::fill(target.begin(),target.end(),0x69);
    shortRead=true;assert(!load()&&reads==6);
    shortRead=false;assert(load()&&reads==7);
    assert(std::equal(source.begin(),source.end(),target.begin()));
    assert(target[source.size()+2]==0x69&&target[source.size()+3]==0x69);
    assert(!bank.load(target.data(),source.size(),source.size(),downwell::fieldBandChecksum,read));
    std::puts("Field bank: exact offline words, reuse, invalidation, failed/corrupt reads and guard word passed");
}
