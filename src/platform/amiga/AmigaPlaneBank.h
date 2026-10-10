#pragma once
#include <cstdint>

// Immutable offline DMA atlas. CPU effects retain the original contiguous DWAP
// in ordinary memory. Only the blitter resolves these independent plane views.
class AmigaPlaneBank {
    const unsigned char* source_=nullptr;
    const unsigned char* bank_=nullptr;
    unsigned sourceBytes_=0,count_=0;
    static unsigned u32(const unsigned char* p){
        return (unsigned(p[0])<<24)|(unsigned(p[1])<<16)|(unsigned(p[2])<<8)|p[3];
    }
public:
    bool bind(const unsigned char* source,unsigned sourceBytes,const unsigned char* bank,
              unsigned bytes,const unsigned char* hash){
        source_=bank_=nullptr;count_=0;
        if(!source||!bank||!hash||bytes<50||u32(bank)!=0x4457504cu||u32(bank+4)!=0x00010030u||
           u32(bank+12)!=sourceBytes)return false;
        for(unsigned i=0;i<32;++i)if(bank[16+i]!=hash[i])return false;
        const unsigned count=u32(bank+8);
        if(!count||count>(bytes-50)/20)return false;
        const unsigned start=48+count*20;
        unsigned previous=0,previousPlane=0;
        for(unsigned i=0;i<count;++i){
            const auto* r=bank+48+i*20;const unsigned offset=u32(r),plane=u32(r+4);
            if((i&&(offset<previous||(offset==previous&&plane<=previousPlane)))||(offset&1)||!plane||(plane&1)||offset>sourceBytes||
               plane>(sourceBytes-offset)/3)return false;
            previous=offset;previousPlane=plane;
            for(unsigned p=0;p<3;++p){const unsigned at=u32(r+8+p*4);
                if((at&1)||at<start||at>bytes-2||plane>bytes-2-at)return false;
            }
        }
        source_=source;sourceBytes_=sourceBytes;bank_=bank;count_=count;return true;
    }
    bool contains(const unsigned char* source)const{
        const auto at=reinterpret_cast<unsigned long>(source),first=reinterpret_cast<unsigned long>(source_);
        return source_&&at>=first&&at-first<sourceBytes_;
    }
    bool find(const unsigned char* source,unsigned planeBytes,const unsigned char*& low,
              const unsigned char*& high,const unsigned char*& mask)const{
        if(!contains(source))return false;
        const unsigned wanted=unsigned(source-source_);unsigned first=0,last=count_;
        while(first<last){const unsigned middle=first+(last-first)/2;
            const auto* r=bank_+48+middle*20;const unsigned offset=u32(r),plane=u32(r+4);
            if(offset<wanted||(offset==wanted&&plane<planeBytes))first=middle+1;
            else if(offset>wanted||(offset==wanted&&plane>planeBytes))last=middle;
            else{
                if(u32(r+4)!=planeBytes)return false;
                low=bank_+u32(r+8);high=bank_+u32(r+12);mask=bank_+u32(r+16);return true;
            }
        }
        return false;
    }
};
