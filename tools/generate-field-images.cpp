// Offline small-field bitmaps. The portable raster is retained as the oracle;
// this does not claim original Windows GPU raster equivalence.
#include "core/GoalFieldOutline.h"
#include "../generated/downwell-field-spans.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) return 20;
    constexpr unsigned width=64, height=56, plane=width*height/8;
    std::map<std::string,unsigned> unique;
    std::vector<std::string> images;
    std::vector<unsigned> indices;
    for (unsigned phase=0; phase<512; ++phase) {
        std::string pixels(width*height, '\0');
        downwell::GoalFieldOutline outline;
        outline.setRasterCircle(0,0,24*256+phase);
        bool valid=true;
        outline.draw([&](int x,int y,int pen) {
            x+=32; y+=28;
            if (x<0 || x>=int(width) || y<0 || y>=int(height)) { valid=false; return; }
            pixels[y*width+x]=char(pen);
        });
        if (!valid) return 20;
        const auto inserted=unique.emplace(pixels,unsigned(images.size()));
        const unsigned id=inserted.first->second;
        if (id!=downwell::fieldSpanIndices[phase]) return 20;
        if (inserted.second) images.push_back(pixels);
        indices.push_back(id);
    }
    if (images.size()>65535) return 20;
    std::string pack="DWFI";
    auto word=[&](unsigned n){pack+=char(n>>8);pack+=char(n);};
    word(1);word(unsigned(images.size()));word(width);word(height);word(32);word(28);
    for (auto id:indices) word(id);
    for (const auto& pixels:images) {
        std::string planar(plane*3,'\0');
        for (unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) {
            const unsigned pen=static_cast<unsigned char>(pixels[y*width+x]);
            const unsigned offset=y*(width/8)+x/8, bit=0x80u>>(x&7);
            if(pen&1)planar[offset]|=char(bit);
            if(pen&2)planar[plane+offset]|=char(bit);
            if(pen)planar[2*plane+offset]|=char(bit);
        }
        // Verify colour and opacity independently for every pixel, including
        // padding. Masked black must stay opaque, not turn into transparency.
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
            const unsigned offset=y*(width/8)+x/8, bit=0x80u>>(x&7);
            const unsigned low=(planar[offset]&bit)?1:0;
            const unsigned high=(planar[plane+offset]&bit)?2:0;
            const unsigned pen=static_cast<unsigned char>(pixels[y*width+x]);
            if((low|high)!=pen || bool(planar[2*plane+offset]&bit)!=bool(pen))return 20;
        }
        pack+=planar;
    }
    std::ofstream output(argv[1],std::ios::binary);
    output.write(pack.data(),pack.size());output.close();
    if(!output)return 20;
    std::cout<<indices.size()<<" radii, "<<images.size()<<" planar images, "<<pack.size()<<" bytes\n";
    return 0;
}
