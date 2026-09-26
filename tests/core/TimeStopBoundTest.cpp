#include "core/TimeStopBound.h"
// UTMT 0.9.2.0, data.win SHA-256
// c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8.
// Assembly/gml_GlobalScript_TimeStopBound.asm SHA-256:
// 5ddcdef2048c195e966d86c297f4a19ea9e0dc7910a58fe8644630b7d2c40b58
// Assembly/gml_GlobalScript_checkTimeStopArea.asm SHA-256:
// 5235a1c7bd7bc8f0670039e050040c0012d79fb4dd285b74d02c40ea4d2f467c
// The supplied field result isolates the script predicate, not place_meeting's
// transformed mask geometry or original-runner encounter behavior.
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t n;std::memcpy(&n,&d,8);return {unsigned(n>>32),unsigned(n)};}
int main(){
    unsigned cases=0;
    for(int width:{0,159,160,319,320,480,1920}){
        for(int i=-1024;i<=8192;++i){
            const double x=i/4.0;
            for(bool stopped:{false,true})for(bool field:{false,true}){
                unsigned queries=0;bool allowed=false;
                assert(TimeStopBound::evaluate(stopped,real(x),real(17.5),width,
                    [&](auto px,auto py,bool& hit){
                        ++queries;assert(px.high==real(x).high && px.low==real(x).low);
                        assert(py.high==real(17.5).high && py.low==real(17.5).low);
                        hit=field;return true;
                    },allowed));
                assert(queries==unsigned(stopped));
                assert(allowed==(!stopped || field || x<160 || x>width-160));++cases;
            }
        }
        for(double boundary:{160.0,double(width-160)})for(double x:{std::nextafter(boundary,-INFINITY),boundary,std::nextafter(boundary,INFINITY)}){
            // Skip subnormals: outside the motion representation's contract.
            if(!real(x).valid())continue;
            bool allowed=false;
            assert(TimeStopBound::evaluate(true,real(x),{},width,[](auto,auto,bool& hit){hit=false;return true;},allowed));
            assert(allowed==(x<160 || x>width-160));++cases;
        }
    }
    bool allowed=false;
    assert(!TimeStopBound::evaluate(true,real(-1000),{},480,[](auto,auto,bool&){return false;},allowed));
    assert(!allowed); // Failure is not hidden by a side exemption.
    assert(TimeStopBound::evaluate(false,{}, {},480,[](auto,auto,bool&){assert(false);return false;},allowed));
    assert(allowed);
    std::printf("TimeStopBound: %u boundary/field cases and query ordering passed\n",cases);
}
