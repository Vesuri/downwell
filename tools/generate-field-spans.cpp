// Offline presentation cache, version 2. This preserves the portable raster;
// it does not establish equivalence with the original Windows GPU raster.
#include "core/GoalFieldOutline.h"
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc > 2) return 20;
    constexpr int extent = 132, width = extent * 2 + 1;
    struct Span { int first, end, y, pen; };
    std::map<std::string, unsigned> unique;
    std::vector<Span> spans;
    std::vector<unsigned> offsets{0}, indices;
    unsigned long plots = 0, selectedSpans = 0;
    for (int minimum : {24,128}) for (int phase = 0; phase < 512; ++phase) {
        std::string pixels(width * width, '\0');
        downwell::GoalFieldOutline outline;
        outline.setRasterCircle(0,0,minimum * 256 + phase);
        bool invalid = false;
        outline.draw([&](int x,int y,int pen) {
            if (x < -extent || x > extent || y < -extent || y > extent) {
                invalid = true; return;
            }
            pixels[(y+extent)*width+x+extent] = static_cast<char>(pen);
            ++plots;
        });
        if (invalid) return 20;
        auto insertion = unique.emplace(pixels,static_cast<unsigned>(unique.size()));
        const unsigned index = insertion.first->second;
        indices.push_back(index);
        if (insertion.second) {
            for (int y = -extent; y <= extent; ++y) {
                for (int x = -extent; x <= extent;) {
                    const int pen = pixels[(y+extent)*width+x+extent];
                    const int first = x++;
                    while (x <= extent && pixels[(y+extent)*width+x+extent] == pen) ++x;
                    if (pen) spans.push_back({first,x,y,pen});
                }
            }
            offsets.push_back(static_cast<unsigned>(spans.size()));
        }
        // Verify every radius, including deduplicated entries, against the
        // independent pixel callback path before emitting any output.
        std::string reconstructed(width * width,'\0');
        for (unsigned s = offsets[index]; s < offsets[index+1]; ++s) {
            const auto& span = spans[s];
            for (int x = span.first; x < span.end; ++x)
                reconstructed[(span.y+extent)*width+x+extent] = static_cast<char>(span.pen);
        }
        if (reconstructed != pixels) return 20;
        selectedSpans += offsets[index+1]-offsets[index];
    }
    if (unique.size() > 65535) return 20;
    // Share identical horizontal rows between all shapes. Runtime only walks
    // visible rows; this also avoids visiting vertically clipped geometry.
    std::map<std::vector<int>, unsigned> uniqueRows;
    std::vector<std::vector<int>> rows;
    std::vector<unsigned> rowIndices, shapeOffsets{0};
    std::vector<int> firstRows;
    for (unsigned shape = 0; shape < unique.size(); ++shape) {
        unsigned s = offsets[shape];
        const int first = spans[s].y, last = spans[offsets[shape+1]-1].y;
        firstRows.push_back(first);
        for (int y = first; y <= last; ++y) {
            std::vector<int> row;
            while (s < offsets[shape+1] && spans[s].y == y) {
                row.insert(row.end(),{spans[s].first,spans[s].end,spans[s].pen});
                ++s;
            }
            auto insertion = uniqueRows.emplace(row,static_cast<unsigned>(rows.size()));
            if (insertion.second) rows.push_back(row);
            rowIndices.push_back(insertion.first->second);
        }
        shapeOffsets.push_back(static_cast<unsigned>(rowIndices.size()));
    }
    unsigned runs = 0;
    for (const auto& row : rows) runs += row.size()/3;
    // Pack final row colours, not individual spans, into ready planar words.
    // Align the normal well centre (logical X 190) to avoid runtime shifts in
    // the common view. Other positions use the same words with carry shifts.
    constexpr int wordOrigin=14;
    struct Word {int column;unsigned low,high;};
    std::vector<Word> words;
    std::vector<unsigned> wordOffsets{0};
    for(const auto& row:rows){
        std::map<int,std::array<unsigned,2>> packed;
        for(unsigned i=0;i<row.size();i+=3)for(int x=row[i];x<row[i+1];++x){
            const int pixel=x+wordOrigin;
            int column=pixel/16;if(pixel<0&&pixel%16)--column;
            const unsigned bit=0x8000u>>unsigned(pixel-column*16);
            auto& planes=packed[column];
            for(unsigned p=0;p<2;++p)
                planes[p]=(row[i+2]&(1<<p))?planes[p]|bit:planes[p]&~bit;
        }
        for(const auto& word:packed)words.push_back({word.first,word.second[0],word.second[1]});
        wordOffsets.push_back(unsigned(words.size()));
    }
    if(words.size()>65535)return 20;
    if (rows.size() > 65535 || runs > 65535 || rowIndices.size() > 65535) return 20;
    const auto bytes = indices.size()*2 + shapeOffsets.size()*2 + firstRows.size()*2 +
        rowIndices.size()*2 + (rows.size()+1)*2 + runs*6;
    std::cerr << indices.size() << " radii, " << unique.size() << " unique rasters, "
              << rows.size() << " unique rows, " << runs << " stored runs, " << bytes << " table bytes; "
              << plots << " original pixel callbacks, " << selectedSpans
              << " cached span callbacks over all radii\n";
    std::cerr << words.size() << " stored planar words, "
              << bytes-(rows.size()+1)*2-runs*6+wordOffsets.size()*2+words.size()*6
              << " word-path table bytes (unused span tables are link-discarded)\n";
    if (argc == 1) return 0;
    std::ofstream output(argv[1]);
    output << "// Generated by tools/generate-field-spans.cpp version 2; do not edit.\n"
              "// Portable raster cache, not an original Windows pixel oracle.\n"
              "#ifndef DOWNWELL_GENERATED_FIELD_SPANS_H\n"
              "#define DOWNWELL_GENERATED_FIELD_SPANS_H\nnamespace downwell {\n"
              "struct CachedFieldRun { short first,end; unsigned short pen; };\n"
              "static const unsigned short fieldSpanIndices[1024] = {\n";
    for (auto i : indices) output << i << ",";
    output << "\n};\nstatic const unsigned short fieldShapeOffsets[] = {\n";
    for (auto i : shapeOffsets) output << i << ",";
    output << "\n};\nstatic const short fieldShapeFirstRows[] = {\n";
    for (auto i : firstRows) output << i << ",";
    output << "\n};\nstatic const unsigned short fieldRowIndices[] = {\n";
    for (auto i : rowIndices) output << i << ",";
    output << "\n};\nstatic const unsigned short fieldRowOffsets[] = {\n0,";
    unsigned end = 0;
    for (const auto& row : rows) { end += row.size()/3; output << end << ','; }
    output << "\n};\nstatic const CachedFieldRun fieldRuns[] = {\n";
    for (const auto& row : rows) for (unsigned i = 0; i < row.size(); i += 3)
        output << '{' << row[i] << ',' << row[i+1] << ',' << row[i+2] << "},\n";
    output << "};\nstatic constexpr int fieldWordOrigin=" << wordOrigin << ";\n"
              "struct CachedFieldWord { short column; unsigned short low,high; };\n"
              "static const unsigned short fieldWordOffsets[] = {\n";
    for(auto offset:wordOffsets)output << offset << ',';
    output << "\n};\nstatic const CachedFieldWord fieldWords[] = {\n";
    for(const auto& word:words)output << '{' << word.column << ',' << word.low << ',' << word.high << "},\n";
    output << "};\n}\n#endif\n";
    output.close();
    return output ? 0 : 20;
}
