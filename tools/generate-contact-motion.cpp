#include "../src/core/ContactMotionCodec.h"
#include "../src/core/TableAtan.h"
#include "../src/core/RunnerPointDirection.h"
#include "../src/core/SmokeTrig.h"
#include <array>
#include <map>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace downwell;
static double host(MotionBinary64 value){
    const std::uint64_t bits=(std::uint64_t(value.high)<<32)|value.low;
    double result;std::memcpy(&result,&bits,8);return result;
}
static MotionBinary64 binary(double value){
    std::uint64_t bits;std::memcpy(&bits,&value,8);return {unsigned(bits>>32),unsigned(bits)};
}
int main(){
    std::int64_t coefficients[403][4];
    for(unsigned segment=0;segment<403;++segment){
        long double samples[4];
        for(unsigned j=0;j<4;++j){MotionBinary64 result;
            if(!smokeCosine(binary((segment+j/4.0)/64),result))return 1;
            samples[j]=host(result);
        }
        const auto d1=samples[1]-samples[0],d2=samples[2]-2*samples[1]+samples[0],
            d3=samples[3]-3*samples[2]+3*samples[1]-samples[0];
        const long double polynomial[]={samples[0],4*(d1-d2/2+d3/3),8*(d2-d3),64*d3/6};
        for(unsigned j=0;j<4;++j)coefficients[segment][j]=std::llround(polynomial[j]*70368744177664.L);
    }
    struct Sample {RunnerImageValue radians;MotionBinary64 speed;};
    std::vector<Sample> samples;
    std::map<unsigned,unsigned> exceptions;
    for(unsigned q=0;q<=table_atan_detail::pi;++q)for(unsigned sign=0;sign<2;++sign){
        const auto source=RunnerImageValue{table_atan_detail::rounded(q<<14)|(sign<<31)}.promoted();
        MotionBinary64 direction,cosine,expected,predicted;
        if(!RunnerPointDirection::evaluate({}, {},MotionBinary64::fromInteger(1),MotionBinary64::fromInteger(1),direction,
            [&](auto,auto,auto& result){result=source;return true;}))return 2;
        RunnerImageValue angle,radians,speed;
        if(!angle.assign(direction)||!SmokeDirection::radians(angle,radians)||!smokeCosine(radians.promoted(),cosine)||
            !SmokeDirection::component({0x40000000u},cosine,false,expected)||!speed.assign(expected)||
            !ContactMotionCodec::decode(radians,coefficients,nullptr,0,predicted))return 3;
        if(expected.high!=predicted.high||expected.low!=predicted.low)exceptions[radians.bits]=speed.bits;
        samples.push_back({radians,expected});
    }
    std::vector<ContactMotionCodec::Correction> corrections;
    for(const auto& entry:exceptions)corrections.push_back({entry.first,entry.second});
    // Certify the final decoder, including exception lookup and binary64
    // promotion, not just the predictor's host arithmetic.
    for(const auto& sample:samples){MotionBinary64 actual;
        if(!ContactMotionCodec::decode(sample.radians,coefficients,corrections.data(),corrections.size(),actual)||
            actual.high!=sample.speed.high||actual.low!=sample.speed.low)return 4;
    }
    std::fprintf(stderr,"Certified %zu signed Q16 inputs; %zu corrections; %zu payload bytes\n",
        samples.size(),corrections.size(),sizeof(coefficients)+corrections.size()*sizeof(corrections[0]));
    std::puts("// Generated and exhaustively certified; do not commit this derived payload.\nstatic const std::int64_t coefficients[403][4]={");
    for(const auto& row:coefficients)std::printf("{%lldLL,%lldLL,%lldLL,%lldLL},\n",
        (long long)row[0],(long long)row[1],(long long)row[2],(long long)row[3]);
    std::puts("};\nstatic const ContactMotionCodec::Correction corrections[]={");
    for(const auto& c:corrections)std::printf("{0x%08xu,0x%08xu},\n",c.radians,c.speed);
    std::puts("};");
}
