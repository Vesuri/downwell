#ifndef DOWNWELL_AMIGA_ROW_DEBT_H
#define DOWNWELL_AMIGA_ROW_DEBT_H
#include <cstdint>

// One bit per physical row; no second per-group count to maintain. Publication
// and acknowledgement operate on word masks, enumeration returns exact runs.
template<unsigned Rows> class AmigaRowDebt {
    static_assert(Rows%16==0,"Whole row-debt words");
    std::uint16_t words_[Rows/16]{};
    static unsigned firstBit(unsigned bits){
        static constexpr unsigned char nibble[16]={4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0};
        unsigned n=0;
        if(!(bits&255)){bits>>=8;n=8;}
        if(!(bits&15)){bits>>=4;n+=4;}
        return n+nibble[bits&15];
    }
public:
    void reset(){for(auto& word:words_)word=0xffff;}
    bool contains(unsigned row)const{return (words_[row/16]&(1u<<(row&15)))!=0;}
    void assign(unsigned first,unsigned end,bool owed){
        while(first<end){
            const unsigned word=first/16;
            const unsigned stop=end<(word+1)*16?end:(word+1)*16;
            const unsigned beginBit=first&15,length=stop-first;
            const unsigned mask=(0xffffu>>(16-length))<<beginBit;
            if(owed)words_[word]|=mask;else words_[word]&=~mask;
            first=stop;
        }
    }
    unsigned next(unsigned first,unsigned end,bool owed)const{
        while(first<end){
            const unsigned word=first/16,offset=first&15;
            unsigned bits=(owed?unsigned(words_[word]):unsigned(words_[word]^0xffffu));
            bits&=0xffffu<<offset;
            if(bits){const unsigned found=word*16+firstBit(bits);return found<end?found:end;}
            first=(word+1)*16;
        }
        return end;
    }
};
#endif
