#include "core/RunnerSpatialIndex.h"
#include "core/RunnerLineBounds.h"
#ifdef DOWNWELL_NATIVE_LINE_PROBE
extern "C" bool spatialLineProbe(const downwell::RunnerLineArguments& args,
    downwell::RunnerSpatialSplit::Box box,bool& hit){
    downwell::RunnerSpatialLine line;return line.assign(args)&&line.overlaps(box,hit);
}
extern "C" bool candidateLineProbe(const downwell::RunnerLineArguments& args,
    downwell::RunnerSpatialSplit::Box box,bool& hit){
    return downwell::RunnerLineBounds::test(args,box,true,false,hit);
}
#else
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <cstdio>
using namespace downwell;
static RunnerImageValue real(float n){RunnerImageValue r;std::memcpy(&r.bits,&n,4);return r;}
static float scalar(RunnerImageValue r){float n;std::memcpy(&n,&r.bits,4);return n;}
static bool reference(const RunnerSpatialLine& line,RunnerSpatialSplit::Box box,bool& hit){
    float sx=scalar(line.x),sy=scalar(line.y);
    float ex=sx+scalar(line.dx),ey=sy+scalar(line.dy);
    const float left=scalar(box.left),right=scalar(box.right),top=scalar(box.top),bottom=scalar(box.bottom)+1.f;
    if(sx>=right+1.f||left>ex||std::fmin(sy,ey)>=bottom||top>std::fmax(sy,ey)){hit=false;return true;}
    if(left>sx){
        float a=left-sx,b=ey-sy,c=ex-sx;
        a=a*b;if(c==0)return false;a=a/c;sy=sy+a;sx=left;
    }
    if(ex>right){
        float a=right-ex,b=ey-sy,c=ex-sx;
        a=a*b;a=a/c;ey=ey+a;
    }
    assert(std::isfinite(sy)); // ey can be NaN or infinity in the right strip.
    hit=!(top>sy&&top>ey)&&!(sy>=bottom&&ey>=bottom);return true;
}
static bool candidateReference(RunnerLineArguments args,RunnerSpatialSplit::Box box,bool compatibility){
    float sx=scalar(args.x1),sy=scalar(args.y1),ex=scalar(args.x2),ey=scalar(args.y2);
    const float extension=compatibility?1.f:-0.00001f;
    const float left=scalar(box.left),top=scalar(box.top),right=scalar(box.right)+extension,bottom=scalar(box.bottom)+extension;
    if(std::fmin(sx,ex)>=right||left>std::fmax(sx,ex)||std::fmin(sy,ey)>=bottom||top>std::fmax(sy,ey))return false;
    if(sx>ex){std::swap(sx,ex);std::swap(sy,ey);}
    if(left>sx){float a=left-sx,b=ey-sy,c=ex-sx;a=a*b;a=a/c;sy=sy+a;sx=left;}
    if(ex>right){float a=right-ex,b=ey-sy,c=ex-sx;a=a*b;a=a/c;ey=ey+a;}
    assert(std::isfinite(sy)&&std::isfinite(ey));
    return !(top>sy&&top>ey)&&!(sy>=bottom&&ey>=bottom);
}
int main(){
    unsigned seed=42,comparisons=0,unsupported=0;
    const auto coordinate=[&](){seed=seed*1664525u+1013904223u;return float(int(seed%4096)-2048)/8.f;};
    for(unsigned i=0;i<100000;++i){
        const float ax=coordinate(),ay=coordinate(),bx=i%7?coordinate():ax,by=coordinate();
        RunnerLineArguments args{real(ax),real(ay),real(bx),real(by)};
        RunnerSpatialLine line;assert(line.assign(args));
        const bool forward=ax<bx;
        assert(scalar(line.x)==(forward?ax:bx)&&scalar(line.y)==(forward?ay:by));
        assert(scalar(line.dx)==(forward?bx-ax:ax-bx)&&scalar(line.dy)==(forward?by-ay:ay-by));
        const float left=coordinate(),top=coordinate();
        RunnerSpatialSplit::Box box{real(left),real(top),real(left+32),real(top+32)};
        bool expected=false,actual=false;
        const bool ok=reference(line,box,expected);
        assert(line.overlaps(box,actual)==ok);
        if(ok){assert(actual==expected);++comparisons;}else ++unsupported;
        for(bool compatibility:{false,true}){
            assert(RunnerLineBounds::test(args,box,compatibility,false,actual));
            assert(actual==candidateReference(args,box,compatibility));
            assert(RunnerLineBounds::test(args,box,compatibility,true,actual)&&!actual);
        }
    }
    // Degenerate vertical line in the source's right+1 broad-phase strip
    // reaches division by zero. Preserve masked-SSE unordered/infinite gates.
    RunnerSpatialLine edge;assert(edge.assign({real(16.5f),real(0),real(16.5f),real(8)}));
    bool edgeHit=false;assert(edge.overlaps({real(0),real(0),real(16),real(16)},edgeHit)&&edgeHit);
    unsigned edgeCases=0,edgeHits=0,edgeMisses=0;
    for(float start:{-32.f,-1.f,0.f,1.f,16.f,17.f,32.f})
    for(float end:{-32.f,-1.f,0.f,1.f,16.f,17.f,32.f})
    for(float x:{16.f,16.25f,16.5f,16.75f,17.f}){
        assert(edge.assign({real(x),real(start),real(x),real(end)}));
        const RunnerSpatialSplit::Box box{real(0),real(0),real(16),real(16)};
        bool expected=false,actual=false;
        assert(reference(edge,box,expected)&&edge.overlaps(box,actual)&&actual==expected);
        ++edgeCases;if(actual)++edgeHits;else ++edgeMisses;
    }
    assert(edgeHits&&edgeMisses);
    bool candidate=false;
    assert(RunnerLineBounds::test({real(16.5f),real(0),real(16.5f),real(8)},
        {real(0),real(0),real(16),real(16)},true,false,candidate)&&candidate);
    assert(RunnerLineBounds::test({real(16.5f),real(0),real(16.5f),real(8)},
        {real(0),real(0),real(16),real(16)},false,false,candidate)&&!candidate);
    RunnerSpatialLine vertical;assert(vertical.assign({real(16),real(0),real(16),real(8)}));
    bool hit=false;assert(vertical.overlaps({real(0),real(0),real(16),real(16)},hit)&&hit);
    assert(vertical.assign({real(17),real(0),real(17),real(8)}));
    assert(vertical.overlaps({real(0),real(0),real(16),real(16)},hit)&&!hit);
    RunnerSpatialIndex<128> tree;
    for(unsigned i=0;i<40;++i){
        const float left=float((i*17)%40)*16;
        assert(tree.insert({real(left),real(0),real(left+15),real(15)},i));
    }
    assert(tree.height()>0);
    std::vector<unsigned> order,lineOrder;
    assert(tree.query({real(-100),real(-100),real(1000),real(100)},[&](unsigned i){order.push_back(i);return true;}));
    const RunnerLineArguments args{real(-10),real(4),real(700),real(4)};
    assert(tree.queryLine(args,[&](unsigned i){lineOrder.push_back(i);return true;}));
    assert(order==lineOrder&&order.size()==40);
    unsigned callbacks=0;
    assert(tree.queryLine(args,[&](unsigned i){assert(i==order[0]);++callbacks;return false;}));
    assert(callbacks==1);
    auto invalid=args;invalid.x1.bits=0x7f800000;
    assert(!tree.queryLine(invalid,[&](unsigned){assert(false);return true;}));
    assert(tree.valid()); // Invalid queries do not corrupt the index.
    RunnerSpatialIndex<8> edgeTree;
    assert(edgeTree.insert({real(0),real(0),real(16),real(16)},123));
    callbacks=0;
    assert(edgeTree.queryLine({real(16.5f),real(8),real(16.5f),real(8)},
        [&](unsigned i){assert(i==123);++callbacks;return true;})&&callbacks==1);
    assert(unsupported==0);
    std::printf("Spatial line: %u scalar comparisons, %u unsupported cases, split-tree order and early stop passed\n",comparisons,unsupported);
    std::printf("Right-edge exceptional arithmetic: %u vertical/point/reversed cases (%u hits, %u misses) passed\n",edgeCases,edgeHits,edgeMisses);
    std::puts("Instance line bounds: 200000 scalar compatibility comparisons and exclusion gates passed");
}
#endif
