#include "platform/amiga/AmigaClearBank.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main(int argc, char** argv) {
    require(argc == 2, "clear pack argument required");
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<std::uint8_t> source{std::istreambuf_iterator<char>(file), {}};
    require(source.size() > 20, "read generated clear pack");
    const auto original = source;
    std::vector<std::uint8_t> storage(32140 + 32, 0xa5);
    auto* target = storage.data() + 16;
    unsigned maximum = 0, maximumOutlined = 0;
    for (unsigned sample = 0; sample < 256; ++sample) {
        unsigned offers[4];
        for (unsigned i = 0; i < 4; ++i) {
            const unsigned id = (sample * (i + 1) + i * 7) % 24;
            offers[i] = id == 23 ? 100 : id;
        }
        const unsigned area = 1 + sample % 4;
        const unsigned level = 1 + sample % 3;
        unsigned char owned[23];
        for (unsigned i = 0; i < 23; ++i) owned[i] = (sample + i) % 2;
        const unsigned bytes = AmigaClearBank::build(source.data(), source.size(), offers, 4, target, 32140, area, level, owned);
        require(bytes && bytes <= 13148, "selected offers, stage title and borders fit source-verified bound");
        if (bytes > maximum) maximum = bytes;
        require(std::memcmp(target + 12, source.data() + 12, 8) == 0, "input provenance prefix preserved");
        const unsigned count = AmigaClearBank::longword(target + 8);
        require(count >= 9 && count <= 18, "duplicates deduplicated by resource ID/frame");
        unsigned borders = 0;
        unsigned titles = 0;
        for (unsigned i = 0; i < count; ++i) {
            const auto* copied = target + 20 + i * 24;
            const auto key = AmigaClearBank::longword(copied);
            if (AmigaClearBank::word(copied) >= 64500 && AmigaClearBank::word(copied) < 64512) {
                require(AmigaClearBank::word(copied) == downwell::levelClearStageLabel(area, level),
                    "bank includes only this stage's composed title");
                ++titles;
            }
            require(i == 0 || AmigaClearBank::longword(copied - 24) < key,
                "selected directory is strictly ordered for renderer binary search");
            if (AmigaClearBank::word(copied + 2) != 0) {
                const unsigned id = AmigaClearBank::word(copied);
                require((id == downwell::levelBorderSprite(area) || id == downwell::levelBorderSprite(area + 1)) &&
                    (AmigaClearBank::word(copied + 2) == 7 || AmigaClearBank::word(copied + 2) == 13),
                    "only current/incoming border frames are banked");
                ++borders;
            }
            const std::uint8_t* originalRecord = nullptr;
            for (unsigned j = 0; j < AmigaClearBank::longword(source.data() + 8); ++j) {
                const auto* candidate = source.data() + 20 + j * 24;
                if (AmigaClearBank::longword(candidate) == AmigaClearBank::longword(copied)) originalRecord = candidate;
            }
            require(originalRecord && std::memcmp(copied, originalRecord, 16) == 0 &&
                AmigaClearBank::longword(copied + 20) == AmigaClearBank::longword(originalRecord + 20),
                "resource ID, frame, origins, dimensions and flags preserved");
            const unsigned offset = AmigaClearBank::longword(copied + 16);
            const unsigned length = AmigaClearBank::longword(copied + 20);
            require(offset <= bytes && length <= bytes - offset &&
                std::memcmp(target + offset, source.data() + AmigaClearBank::longword(originalRecord + 16), length) == 0,
                "all selected planar payload bytes preserved");
        }
        require(borders == 4, "both border frames from both selected areas survive sorting");
        require(titles == 1, "exactly one stage title is banked");
        for (unsigned id : offers) {
            const unsigned wanted = downwell::levelUpgradeNameLabel(id, id < 23 ? owned[id] : 0);
            unsigned found = 0;
            for (unsigned i = 0; i < count; ++i)
                found += AmigaClearBank::word(target + 20 + i * 24) == wanted;
            require(found == 1, "exact owned-name variant banked once");
        }
        for (unsigned i = 0; i < 16; ++i)
            require(storage[i] == 0xa5 && storage[storage.size() - 1 - i] == 0xa5, "adjacent staging/HUD guards intact");
        const unsigned outlinedBytes = AmigaClearBank::build(source.data(), source.size(), offers, 4,
            target, 31756, area, level, owned, true);
        require(outlinedBytes && outlinedBytes <= 31756, "outlined bank fits existing retired storage");
        if (outlinedBytes > maximumOutlined) maximumOutlined = outlinedBytes;
        const unsigned outlinedCount = AmigaClearBank::longword(target+8);
        require(outlinedCount <= 26, "outlined directory fits bounded scratch storage");
        for (unsigned id : offers) {
            const unsigned labels[] = {downwell::levelUpgradeNameLabel(id,id<23?owned[id]:0),64300+id};
            for (unsigned label : labels) for (unsigned frame = 0; frame < 2; ++frame) {
                unsigned found = 0;
                for (unsigned i = 0; i < outlinedCount; ++i) {
                    const auto* entry = target+20+i*24;
                    found += AmigaClearBank::word(entry)==label && AmigaClearBank::word(entry+2)==frame;
                }
                require(found==1,"both oracle and precomposed text frames are banked once");
            }
        }
    }
    const auto before = storage;
    const auto payload = [&](unsigned id, unsigned frame) {
        for (unsigned i = 0; i < AmigaClearBank::longword(source.data()+8); ++i) {
            const auto* record = source.data()+20+i*24;
            if (AmigaClearBank::word(record)==id && AmigaClearBank::word(record+2)==frame)
                return AmigaClearBank::longword(record+20);
        }
        require(false,"bound calculation requires every offered resource");
        return std::uint32_t(0);
    };
    unsigned maximumBase = 0, maximumBundle = 0;
    for (unsigned area=1;area<=4;++area) for (unsigned level=1;level<=3;++level) {
        unsigned bytes=20+26*24+payload(downwell::levelClearStageLabel(area,level),0)+payload(64001,0);
        for (unsigned a=area;a<=area+1;++a)
            bytes+=payload(downwell::levelBorderSprite(a),7)+payload(downwell::levelBorderSprite(a),13);
        if(bytes>maximumBase) maximumBase=bytes;
    }
    for (unsigned option=0;option<24;++option) for (unsigned owned=0;owned<(option==23?1u:2u);++owned) {
        const unsigned id=option==23?100:option, name=downwell::levelUpgradeNameLabel(id,owned);
        unsigned bytes=payload(downwell::levelUpgradeSprite(id),0);
        for(unsigned frame=0;frame<2;++frame) bytes+=payload(name,frame)+payload(64300+id,frame);
        if(bytes>maximumBundle) maximumBundle=bytes;
    }
    require(maximumBase+4*maximumBundle<=31756,"all four-offer combinations fit, without relying on sampled offers");
    std::printf("Conservative all-offer outlined bank bound: %u bytes\n",maximumBase+4*maximumBundle);
    unsigned preparedBase=0, preparedBundle=0;
    for(unsigned area=1;area<=4;++area) for(unsigned level=1;level<=3;++level) {
        unsigned bytes=20+34*24;
        const unsigned titles[]={downwell::levelClearStageLabel(area,level),64001};
        for(unsigned id:titles) {
            bytes+=payload(id,0);
            for(unsigned frame=2;frame<=7;++frame) bytes+=payload(id,frame);
        }
        for(unsigned a=area;a<=area+1;++a)
            bytes+=payload(downwell::levelBorderSprite(a),7)+payload(downwell::levelBorderSprite(a),13)+
                payload(downwell::levelBorderSprite(a),263)+payload(downwell::levelBorderSprite(a),269);
        if(bytes>preparedBase) preparedBase=bytes;
    }
    for(unsigned option=0;option<24;++option) for(unsigned owned=0;owned<(option==23?1u:2u);++owned) {
        const unsigned id=option==23?100:option;
        const unsigned bytes=payload(downwell::levelUpgradeSprite(id),0)+
            payload(downwell::levelUpgradeNameLabel(id,owned),1)+payload(64300+id,1);
        if(bytes>preparedBundle) preparedBundle=bytes;
    }
    require(preparedBase+4*preparedBundle<=31756,"all prepared heading/four-offer banks fit the existing Chip region");
    std::printf("Conservative prepared-heading bank bound: %u bytes\n",preparedBase+4*preparedBundle);
    unsigned maximumPrepared=0;
    for(unsigned sample=0;sample<256;++sample) {
        unsigned offers[4]; unsigned char owned[23];
        for(unsigned i=0;i<4;++i) {
            const unsigned id=(sample*(i+1)+i*7)%24;
            offers[i]=id==23 ? 100 : id;
        }
        for(unsigned i=0;i<23;++i) owned[i]=(sample+i)%2;
        const unsigned area=1+sample%4, level=1+sample%3;
        std::vector<std::uint8_t> prepared(31756+32,0x69);
        auto* bank=prepared.data()+16;
        const unsigned bytes=AmigaClearBank::build(source.data(),source.size(),offers,4,
            bank,31756,area,level,owned,true,true,true);
        require(bytes && bytes<=preparedBase+4*preparedBundle,"prepared selection respects the all-offer bound");
        if(bytes>maximumPrepared) maximumPrepared=bytes;
        const unsigned count=AmigaClearBank::longword(bank+8);
        require(count<=34,"prepared selection fits its bounded record array");
        unsigned titles=0;
        for(unsigned i=0;i<count;++i) {
            const auto* record=bank+20+i*24;
            const unsigned id=AmigaClearBank::word(record), frame=AmigaClearBank::word(record+2);
            const bool title=id==64001 || id==downwell::levelClearStageLabel(area,level);
            if(title) { require(frame==0 || (frame>=2 && frame<=7),"heading uses only admitted prepared frames"); ++titles; }
            else if(id>=64000) require(frame==1,"production retains only precomposed description/name outlines");
            const auto key=AmigaClearBank::longword(record);
            require(!i || AmigaClearBank::longword(record-24)<key,"prepared directory remains strictly sorted");
            const std::uint8_t* originalRecord=nullptr;
            for(unsigned j=0;j<AmigaClearBank::longword(source.data()+8);++j)
                if(AmigaClearBank::longword(source.data()+20+j*24)==key) originalRecord=source.data()+20+j*24;
            require(originalRecord && std::memcmp(record,originalRecord,16)==0,"prepared geometry matches immutable source record");
            const unsigned offset=AmigaClearBank::longword(record+16), length=AmigaClearBank::longword(record+20);
            require(length==AmigaClearBank::longword(originalRecord+20) && offset>=20+count*24 &&
                offset<=bytes && length<=bytes-offset && std::memcmp(bank+offset,
                source.data()+AmigaClearBank::longword(originalRecord+16),length)==0,"prepared pixels match source payload exactly");
        }
        require(titles==14,"both headings have all seven resident shapes");
        for(unsigned i=0;i<16;++i) require(prepared[i]==0x69 && prepared[31756+16+i]==0x69,"prepared bank preserves allocation guards");
    }
    std::printf("256 prepared heading banks verified; largest sampled bank %u bytes\n",maximumPrepared);
    unsigned char invalidOwned[23] = {};
    invalidOwned[0] = 2;
    const unsigned ownedOffer[] = {0};
    require(!AmigaClearBank::build(source.data(), source.size(), ownedOffer, 1, target, 32140, 1, 1, invalidOwned) &&
        storage == before, "unsupported ownership level fails instead of displaying an incorrect name");
    unsigned bad[] = {99}, fallback[] = {100, 100, 100, 100};
    require(!AmigaClearBank::build(source.data(), source.size(), bad, 1, target, 32140) &&
        !AmigaClearBank::build(source.data(), 19, fallback, 4, target, 32140) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 1) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140, 0) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140, 5) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140, 1, 4) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 5, target, 32140) && storage == before,
        "rejected requests leave existing bank untouched");
    require(AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140) &&
        AmigaClearBank::longword(target + 8) == 9, "four fallback offers share one bundle alongside four borders");
    require(source == original, "source pack untouched by bank construction");
    const auto validBank = storage;
    auto unordered = source;
    // Duplicate and reversed directory keys must fail before touching a bank.
    std::memcpy(unordered.data()+44,unordered.data()+20,4);
    require(!AmigaClearBank::build(unordered.data(),unordered.size(),fallback,4,target,32140) && storage==validBank,
        "duplicate directory key rejected transactionally");
    unordered=source;
    for(unsigned i=0;i<24;++i) {
        const auto byte=unordered[20+i]; unordered[20+i]=unordered[44+i]; unordered[44+i]=byte;
    }
    require(!AmigaClearBank::build(unordered.data(),unordered.size(),fallback,4,target,32140) && storage==validBank,
        "unordered directory rejected transactionally");
    std::vector<std::uint8_t> expectedCopy(31756);
    const unsigned copyBytes = AmigaClearBank::build(source.data(),source.size(),fallback,4,
        expectedCopy.data(),expectedCopy.size(),1,1,nullptr,true);
    require(copyBytes != 0,"copy alignment oracle builds");
    for(unsigned srcOffset=0;srcOffset<4;++srcOffset) for(unsigned dstOffset=0;dstOffset<4;++dstOffset) {
        std::vector<std::uint8_t> shifted(source.size()+4,0xa5), copied(31756+8,0xa5);
        std::memcpy(shifted.data()+srcOffset,source.data(),source.size());
        const auto unchangedSource=shifted;
        auto* destination=copied.data()+4+dstOffset;
        const unsigned bytes=AmigaClearBank::build(shifted.data()+srcOffset,source.size(),fallback,4,
            destination,31756,1,1,nullptr,true);
        require(bytes==copyBytes && std::memcmp(destination,expectedCopy.data(),bytes)==0,
            "all source/destination byte alignments preserve bank payload");
        for(unsigned i=0;i<4+dstOffset;++i) require(copied[i]==0xa5,"prefix guard intact");
        for(unsigned i=4+dstOffset+bytes;i<copied.size();++i) require(copied[i]==0xa5,"suffix guard intact");
        require(shifted==unchangedSource,"copy source untouched");
    }
    for (unsigned j = 0; j < AmigaClearBank::longword(source.data() + 8); ++j) {
        auto* record = source.data() + 20 + j * 24;
        if (AmigaClearBank::word(record) == 64500) {
            AmigaClearBank::putLong(record + 16, 0xffffffffu);
            break;
        }
    }
    require(!AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140) && storage == validBank,
        "out-of-range payload offset is rejected before any destination write");
    std::printf("256 clear banks verified; largest sampled bank %u bytes\n", maximum);
    std::printf("256 outlined banks verified; largest sampled bank %u bytes\n",maximumOutlined);
}
