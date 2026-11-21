#ifndef DOWNWELL_TESTS_SOUND_REQUEST_LOG_H
#define DOWNWELL_TESTS_SOUND_REQUEST_LOG_H
// W2.3 host route logs: drain the core's SoundRequests after every step into
// a text log (one request per line) and keep each sound's request count and
// peak concurrency (live voices of that asset in the source-duration model).
//
// Log line: tick kind asset voice priority loop cut category gain pitch fade,
// where kind is play/stop/stopall/pauseall/resumeall/gain/pitch/pause/resume
// (voice is the voice or asset id the request names), category is
// the OLchannel index (-1 for soundPlay), gain/pitch are decimal and fade is
// a gain request's fade time in milliseconds. W2.6 bank lines, written
// before the requests they cover when the Amiga's sound bank set changes,
// are "tick bank resident scene" (each a pack name or none): the Paula route
// replay binds those packs, as AmigaSoundBanks does at that point.
//
// The summary (asset count peak, one line per sound, then a total line) is
// what route tests compare with the committed expectation in
// tests/data/sound-routes/<route>.summary; DOWNWELL_WRITE_SOUND_ROUTES=1
// rewrites the expectations instead (tools/sound-inventory.py reads them).
#include "core/SoundRequests.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace downwell_test {
class SoundRequestLog {
public:
    explicit SoundRequestLog(const std::string& path):out_(path){}
    bool good()const{return bool(out_);}
    void drain(downwell::SoundRequests& requests,const downwell::SoundVoices& voices){
        dropped_=requests.dropped();
        requests.drain([&](const downwell::SoundRequest& r){record(r);});
        std::map<int,unsigned> live;
        voices.forEachLive([&](int asset){++live[asset];});
        unsigned total=0;
        for(const auto& v:live){
            auto& s=sounds_[v.first];if(v.second>s.peak)s.peak=v.second;total+=v.second;
        }
        if(total>peakVoices_)peakVoices_=total;
    }
    // The Amiga bank set for the requests drained next (only changes are
    // written). The caller mirrors where DownwellAmiga.cpp switches banks.
    void banks(std::uint32_t tick,const char* resident,const char* scene){
        if(resident==resident_&&scene==scene_)return;
        resident_=resident;scene_=scene;
        out_<<tick<<" bank "<<resident<<' '<<scene<<'\n';
    }
    unsigned plays(int asset)const{auto i=sounds_.find(asset);return i==sounds_.end()?0:i->second.count;}
    unsigned peak(int asset)const{auto i=sounds_.find(asset);return i==sounds_.end()?0:i->second.peak;}
    unsigned requests()const{return requests_;}
    unsigned kinds(downwell::SoundRequest::Kind k)const{return kinds_[k];}
    unsigned peakVoices()const{return peakVoices_;}
    std::uint32_t dropped()const{return dropped_;}
    std::string summary()const{
        std::ostringstream s;
        for(const auto& v:sounds_)if(v.second.count)s<<v.first<<' '<<v.second.count<<' '<<v.second.peak<<'\n';
        s<<"total "<<requests_<<" plays "<<kinds_[downwell::SoundRequest::play]<<" stops "<<kinds_[downwell::SoundRequest::stop]
         <<" peak "<<peakVoices_<<'\n';
        return s.str();
    }
    // Compare with (or, under DOWNWELL_WRITE_SOUND_ROUTES, rewrite) the
    // committed expectation. Returns false with a reason on mismatch.
    bool matchesExpectation(const std::string& route,std::string& reason)const{
        const std::string path="tests/data/sound-routes/"+route+".summary";
        const std::string actual=summary();
        if(std::getenv("DOWNWELL_WRITE_SOUND_ROUTES")){
            std::ofstream(path)<<actual;return true;
        }
        std::ifstream in(path);
        if(!in){reason=path+" is missing";return false;}
        std::stringstream expected;expected<<in.rdbuf();
        if(expected.str()!=actual){reason=path+" differs from this run";return false;}
        return true;
    }
private:
    static double real(downwell::MotionBinary64 v){
        const std::uint64_t bits=(std::uint64_t(v.high)<<32)|v.low;double d;std::memcpy(&d,&bits,8);return d;
    }
    void record(const downwell::SoundRequest& r){
        static const char* names[]={"play","stop","stopall","pauseall","resumeall","gain","pitch","pause","resume"};
        ++requests_;++kinds_[r.kind];
        if(r.kind==downwell::SoundRequest::play)++sounds_[r.asset].count;
        out_<<r.tick<<' '<<names[r.kind]<<' '<<r.asset<<' '<<r.voice<<' '<<unsigned(r.priority)<<' '
            <<int(r.loop)<<' '<<int(r.cut)<<' '<<int(r.category)<<' '<<real(r.gainValue)<<' '<<real(r.pitchValue)<<' '<<r.fadeMs<<'\n';
    }
    struct Sound {unsigned count=0,peak=0;};
    std::ofstream out_;
    std::map<int,Sound> sounds_;
    unsigned requests_=0,kinds_[9]{},peakVoices_=0;
    std::uint32_t dropped_=0;
    const char* resident_=nullptr;
    const char* scene_=nullptr;
};
}
#endif
