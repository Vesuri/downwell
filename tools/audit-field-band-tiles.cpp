// Geometry-only DMA audit. This does not predict elapsed time: more rectangles
// trade fewer fetched words for extra setup, waits and edge-mask handling.
#include "../generated/downwell-field-columns.h"
#include <algorithm>
#include <iostream>

int main(){
    using namespace downwell;
    for(unsigned pieces:{2u,3u,4u,6u,11u}){
        unsigned oldWords=0,newWords=0,oldCalls=0,newCalls=0;
        for(const auto& image:fieldBandImages){
            const auto* mask=fieldBandPixels+image.wordOffset+11*image.height;
            for(unsigned sx=0;sx<=16;++sx){
                oldWords+=11*image.height;++oldCalls;
                for(unsigned piece=0;piece<pieces;++piece){
                    const unsigned first=11*piece/pieces,last=11*(piece+1)/pieces;
                    const unsigned left=std::max(sx,first*16),right=std::min(sx+160,last*16);
                    if(left>=right)continue;
                    unsigned top=image.height,bottom=0;
                    // Fixed, offline tile bounds. Deliberately do not tighten
                    // these again for each viewport clipping phase.
                    for(unsigned y=0;y<image.height;++y)for(unsigned x=first;x<last;++x)
                        if(mask[y*11+x]){top=std::min(top,y);bottom=std::max(bottom,y+1);}
                    if(top>=bottom)continue;
                    const unsigned destX=110+left-sx,width=right-left;
                    const unsigned words=std::max(((left&15)+width+15)/16,((destX&15)+width+15)/16);
                    newWords+=words*(bottom-top);++newCalls;
                }
            }
        }
        std::cout<<pieces<<" pieces: words "<<oldWords<<" -> "<<newWords
            <<", transfers "<<oldCalls<<" -> "<<newCalls
            <<", words saved per extra transfer "
            <<(double(oldWords)-double(newWords))/double(newCalls-oldCalls)<<'\n';
    }
}
