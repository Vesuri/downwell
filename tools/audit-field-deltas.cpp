// Offline representation audit, not a runtime renderer. Count the work left
// after retaining pixels common to successive prepared large-circle rasters.
#include "../generated/downwell-field-columns.h"
#include <array>
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
using Raster=std::map<std::pair<int,int>,std::array<unsigned,2>>;
int main(){
    using namespace downwell;
    std::vector<Raster> frames;
    for(const auto& frame:fieldColumnFrames){
        Raster raster;
        for(int c=-8;c<10;++c)
            for(unsigned i=fieldColumnOffsets[frame[c+8]];i<fieldColumnOffsets[frame[c+8]+1];++i){
                const auto& w=fieldColumnWords[i];raster[{w.row,c}]={w.low,w.high};
            }
        frames.push_back(raster);
    }
    unsigned total=0,peak=0,adjacent=0,adjacentPeak=0;
    for(unsigned a=0;a<frames.size();++a)for(unsigned b=0;b<frames.size();++b){
        if(a==b)continue;
        auto keys=frames[a];for(const auto& p:frames[b])keys.emplace(p.first,p.second);
        unsigned changed=0,visible=0;
        for(const auto& p:keys){
            const auto x=frames[a].find(p.first),y=frames[b].find(p.first);
            const std::array<unsigned,2> blank{};
            const auto old=x==frames[a].end()?blank:x->second;
            const auto next=y==frames[b].end()?blank:y->second;
            if(old==next)continue;
            ++changed;
            // At centre 190 canonical centre 14 translates by eleven words.
            const int c=p.first.second+11;
            if(c>=6&&c<=16)++visible;
        }
        total+=changed;peak=std::max(peak,visible);
        if(b==a+1){adjacent+=changed;adjacentPeak=std::max(adjacentPeak,visible);}
    }
    std::cout<<frames.size()<<" frames; all directed transitions: "<<total
        <<" changed words, "<<total*10<<" bytes at 10-byte XOR command; peak visible="<<peak<<'\n';
    std::cout<<"Adjacent forward transitions: "<<adjacent<<" changed words, "<<adjacent*10
        <<" bytes; peak visible="<<adjacentPeak<<"; reverse reuses same commands\n";
}
