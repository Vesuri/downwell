#include "platform/amiga/WindBank.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
using namespace downwell;
int main(int argc,char** argv){
    assert(argc==2);
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    WindBank bank;WindBank::Sample sample;
    assert(!bank.ready()&&!bank.sample(349,sample));
    assert(bank.bind(bytes.data(),bytes.size())&&bank.size()==bytes.size());
    for(int id=349;id<=355;++id){
        assert(bank.sample(id,sample)&&sample.length);
        assert(sample.bytes>=bytes.data()+104&&sample.bytes+sample.length<=bytes.data()+bytes.size());
    }
    assert(!bank.sample(348,sample)&&!sample.bytes&&!sample.length);
    assert(!bank.sample(356,sample));
    // Corrupt each structural word independently, including every offset/ID.
    for(unsigned offset=0;offset<104;++offset){
        auto changed=bytes;changed[offset]^=0x80;
        assert(!bank.bind(changed.data(),changed.size())&&!bank.ready());
    }
    for(unsigned length=0;length<104;++length)assert(!bank.bind(bytes.data(),length));
    assert(!bank.bind(bytes.data(),bytes.size()-1));
    auto extra=bytes;extra.push_back(0);assert(!bank.bind(extra.data(),extra.size()));
    assert(!bank.bind(nullptr,bytes.size()));
    // A stale successful borrow cannot survive a failed rebind or retirement.
    assert(bank.bind(bytes.data(),bytes.size()));
    assert(!bank.bind(nullptr,0)&&!bank.sample(349,sample));
    assert(bank.bind(bytes.data(),bytes.size()));bank.reset();assert(!bank.ready()&&!bank.size());
    std::puts("wind bank resource/admission/bounds checks passed");
}
