// Host-only representation audit. Counts work and storage; it does not predict native timing.
#include "../generated/downwell-field-columns.h"
#include <map>
#include <array>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
using namespace downwell;
using Pens=std::array<unsigned,2>;
using Raster=std::map<int,Pens>;
static void transitions(){
    const unsigned n=sizeof(fieldColumnFrames)/sizeof(fieldColumnFrames[0]);
    std::vector<unsigned> frequency(n);
    for(auto frame:fieldColumnRadii)++frequency[frame];
    std::cout<<"Uniform independent Q8 radius pairs; identical raster pairs excluded. Full vertical extent.\n";
    for(int centre:{110,190,269}){
        std::vector<Raster> rasters(n);
        unsigned long long full=0,changed=0,unionCount=0,keptColumns=0,totalColumns=0;
        for(unsigned f=0;f<n;++f){
            const int base=(centre-14)/16,shift=(centre-14)%16;
            for(int col=-8;col<=9;++col){
                unsigned id=fieldColumnFrames[f][col+8];
                for(unsigned i=fieldColumnOffsets[id];i<fieldColumnOffsets[id+1];++i){
                    auto w=fieldColumnWords[i];
                    for(int bit=0;bit<16;++bit){
                        int x=(base+col)*16+bit+shift;
                        if(x<110||x>=270)continue;
                        unsigned b=0x8000u>>bit;
                        if(!((w.low|w.high)&b))continue;
                        auto& p=rasters[f][(w.row+131)*24+x/16];
                        unsigned out=0x8000u>>(x%16);
                        for(unsigned plane=0;plane<2;++plane)
                            p[plane]|=((plane?w.high:w.low)&b)?out:0;
                    }
                }
            }
        }
        for(unsigned a=0;a<n;++a)for(unsigned b=0;b<n;++b){
            if(a==b)continue; // whole-raster matches already skip work
            const unsigned weight=frequency[a]*frequency[b];
            full+=(rasters[a].size()+rasters[b].size())*weight;
            auto keys=rasters[a];for(auto& [key,p]:rasters[b])keys.emplace(key,Pens{});
            for(auto& [key,p]:keys){
                auto ia=rasters[a].find(key),ib=rasters[b].find(key);
                const Pens pa=ia==rasters[a].end()?Pens{}:ia->second;
                const Pens pb=ib==rasters[b].end()?Pens{}:ib->second;
                unionCount+=weight;if(pa!=pb)changed+=weight;
            }
            for(int col=-8;col<=9;++col){
                const int x=centre-14+col*16;
                if(x+16<=110||x>=270)continue;
                const auto ca=fieldColumnFrames[a][col+8],cb=fieldColumnFrames[b][col+8];
                const auto count=(fieldColumnOffsets[ca+1]-fieldColumnOffsets[ca])+
                    (fieldColumnOffsets[cb+1]-fieldColumnOffsets[cb]);
                totalColumns+=count*weight;if(ca==cb)keptColumns+=count*weight;
            }
        }
        std::cout<<"centre="<<centre<<" changed/full visits="<<100.0*changed/full
                 <<"% union/full="<<100.0*unionCount/full
                 <<"% whole-column eliminable="<<100.0*keptColumns/totalColumns<<"%\n";
    }
}

static void layouts(){
    using Plane=std::array<std::array<unsigned short,18>,264>;
    const unsigned n=sizeof(fieldColumnFrames)/sizeof(fieldColumnFrames[0]);
    std::set<std::vector<unsigned short>> fragments;
    unsigned fragmentBytes=0,fragmentCount=0;
    for(unsigned f=0;f<n;++f)for(unsigned col=0;col<18;++col){
        const auto id=fieldColumnFrames[f][col];
        auto i=fieldColumnOffsets[id];
        while(i<fieldColumnOffsets[id+1]){
            auto first=i++;
            while(i<fieldColumnOffsets[id+1]&&fieldColumnWords[i].row==fieldColumnWords[i-1].row+1)++i;
            std::vector<unsigned short> image;
            for(unsigned plane=0;plane<3;++plane)for(auto j=first;j<i;++j){
                const auto& w=fieldColumnWords[j];image.push_back(plane==0?w.low:plane==1?w.high:w.low|w.high);
            }
            if(fragments.insert(image).second)fragmentBytes+=image.size()*2;
            ++fragmentCount;
        }
    }
    std::cout<<"translated single-column fragments="<<fragments.size()<<" bank="<<fragmentBytes
             <<" avg pieces="<<double(fragmentCount)/n<<"\n";
    for(int bandHeight:{16,32,48,64,96,132,264}){
        std::set<std::vector<unsigned short>> images;
        unsigned long bytes=0,pieces=0,drawWords=0;
        for(unsigned f=0;f<n;++f){
            Plane low{},high{};
            for(unsigned col=0;col<18;++col){
                auto id=fieldColumnFrames[f][col];
                for(auto i=fieldColumnOffsets[id];i<fieldColumnOffsets[id+1];++i){
                    const auto& w=fieldColumnWords[i];low[w.row+132][col]=w.low;high[w.row+132][col]=w.high;
                }
            }
            for(int y=0;y<264;y+=bandHeight){
                const int h=std::min(bandHeight,264-y);
                for(int start=0;start<18;){
                    auto used=[&](int col){for(int j=y;j<y+h;++j)if(low[j][col]|high[j][col])return true;return false;};
                    while(start<18&&!used(start))++start;
                    if(start==18)break;
                    int end=start+1;while(end<18&&used(end))++end;
                    std::vector<unsigned short> image{static_cast<unsigned short>(end-start),static_cast<unsigned short>(h)};
                    for(unsigned p=0;p<3;++p)for(int j=y;j<y+h;++j)for(int x=start;x<end;++x)
                        image.push_back(p==0?low[j][x]:p==1?high[j][x]:low[j][x]|high[j][x]);
                    if(images.insert(image).second)bytes+=image.size()*2;
                    ++pieces;drawWords+=(end-start)*h;start=end;
                }
            }
        }
        std::cout<<"band="<<bandHeight<<" unique="<<images.size()<<" bank="<<bytes
                 <<" avg pieces="<<double(pieces)/n<<" avg rectangular words="<<double(drawWords)/n<<"\n";
    }
}

int main(){transitions();layouts();}
