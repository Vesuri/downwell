// W2.4 Paula mixer: the portable plan and sequencer on synthetic banks, and
// optionally with the real pack:
//   paula-mixer-test                          unit checks only
//   paula-mixer-test PACK WAV                 + the probe schedule's expected
//                                             output (44.1 kHz stereo WAV)
//   paula-mixer-test PACK --route LOG...      + replay route logs through the
//                                             plan and report steals and drops
#include "platform/amiga/PaulaMixer.h"
#include "PaulaModel.h"
#include "PaulaProbeSchedule.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using downwell::PaulaBank;
using downwell::PaulaChannelCommand;
using downwell::PaulaSequencer;
using downwell::PaulaVoicePlan;
using downwell::SoundRequest;
using downwell::MotionBinary64;

namespace {
void require(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; std::exit(1); }
}
struct Spec { unsigned id, period, volume; bool loop; unsigned gainQ15, bytes; };
std::vector<std::uint8_t> pack(const std::vector<Spec>& specs) {
    // Pack v2: 32-byte header, 16-byte entries, the id index (padded even), data.
    unsigned indexSize = 0;
    for (const Spec& s : specs) indexSize = s.id + 1 > indexSize ? s.id + 1 : indexSize;
    const std::size_t indexAt = 32 + 16 * specs.size();
    std::vector<std::uint8_t> out(indexAt + indexSize + (indexSize & 1));
    auto put16 = [&](std::size_t at, unsigned v) { out[at] = std::uint8_t(v >> 8); out[at + 1] = std::uint8_t(v); };
    auto put32 = [&](std::size_t at, std::uint32_t v) { put16(at, v >> 16); put16(at + 2, v & 0xffff); };
    std::vector<std::uint8_t> data;
    for (std::size_t i = 0; i < specs.size(); ++i) {
        const Spec& s = specs[i];
        const std::size_t e = 32 + 16 * i;
        put16(e, s.id); put16(e + 2, s.period); out[e + 4] = std::uint8_t(s.volume); out[e + 5] = s.loop;
        put16(e + 6, s.gainQ15); put32(e + 8, std::uint32_t(data.size())); put32(e + 12, s.bytes);
        out[indexAt + s.id] = std::uint8_t(i + 1);
        for (unsigned b = 0; b < s.bytes; ++b) data.push_back(std::uint8_t(b & 1 ? 0x40 : 0xc0));
    }
    out[0] = 'D'; out[1] = 'W'; out[2] = 'S'; out[3] = 'X'; put16(4, 2); put16(6, unsigned(specs.size()));
    put32(8, std::uint32_t(out.size() + data.size())); put32(12, std::uint32_t(out.size())); put16(16, indexSize);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}
downwell::AudioLevel real(double d) { return downwell::AudioLevel::raw(std::int32_t(std::lround(d * 65536.0))); }
SoundRequest play(int asset, int voice, unsigned priority, double gain = 1, double pitch = 1, int category = 8) {
    SoundRequest r; r.kind = SoundRequest::play; r.asset = std::int16_t(asset); r.voice = voice;
    r.priority = std::uint8_t(priority); r.category = std::int8_t(category); r.gainValue = real(gain); r.pitchValue = real(pitch);
    return r;
}
SoundRequest simple(SoundRequest::Kind kind, int id = -1) { SoundRequest r; r.kind = kind; r.voice = id; return r; }
SoundRequest gain(int id, double g, unsigned fadeMs = 0) {
    SoundRequest r; r.kind = SoundRequest::gain; r.voice = id; r.gainValue = real(g); r.fadeMs = std::uint16_t(fadeMs); return r;
}
SoundRequest pitch(int id, double p) { SoundRequest r; r.kind = SoundRequest::pitch; r.voice = id; r.pitchValue = real(p); return r; }

constexpr int V = downwell::SoundVoices::firstVoice;

void testBank() {
    auto bytes = pack({{3, 424, 24, false, 32768, 4000}, {18, 424, 64, false, 16384, 6000}, {186, 424, 40, true, 32768, 8366}});
    PaulaBank bank;
    require(bank.bind(bytes.data(), bytes.size()) && bank.count() == 3, "a well-formed pack binds");
    PaulaBank::Sound s;
    require(bank.find(18, s) && s.period == 424 && s.volume == 64 && s.assetGainQ15 == 16384 && s.bytes == 6000 && s.offset == 4000,
            "find reads the entry");
    require(bank.find(186, s) && s.loop && !bank.find(4, s) && !bank.find(-1, s), "find misses absent ids");
    auto bad = bytes; bad[3] = 'Y';
    require(!bank.bind(bad.data(), bad.size()), "a wrong magic is rejected");
    bad = bytes; bad[32 + 15] = 1; // odd length
    require(!bank.bind(bad.data(), bad.size()), "an odd sample length is rejected");
    require(!bank.bind(bytes.data(), bytes.size() - 2), "a short file is rejected");
    require(PaulaVoicePlan::q16(real(1)) == 65536 && PaulaVoicePlan::q16(real(0.5)) == 32768 &&
            PaulaVoicePlan::q16(real(1.8)) == 117965 && PaulaVoicePlan::q16(real(0)) == 0 &&
            PaulaVoicePlan::q16(real(-1)) == 0 && PaulaVoicePlan::q16(real(1000)) == 0xffffff,
            "levels clamp to unsigned Q16");
}

void testPlan() {
    // Ids 1..6: 1 s one-shots at period 424 (8,365 bytes), gain 1, volume 64.
    std::vector<Spec> specs;
    for (unsigned id = 1; id <= 6; ++id) specs.push_back({id, 424, 64, false, 32768, 8366});
    specs.push_back({7, 424, 40, true, 32768, 4000});
    specs.push_back({8, 424, 64, false, 16384, 8000}); // asset gain 0.5
    specs.push_back({9, 200, 64, false, 32768, 2000});
    auto bytes = pack(specs);
    PaulaBank bank; require(bank.bind(bytes.data(), bytes.size()), "plan bank");
    PaulaVoicePlan plan(bank);
    PaulaChannelCommand cmd[4]{};
    auto take = [&] { for (auto& c : cmd) c = {}; plan.publish([&](unsigned c, const PaulaChannelCommand& m) { cmd[c].merge(m); }); };

    // Four starts alternate sides: 0 (left), 1 (right), 3 (left), 2 (right).
    for (unsigned i = 0; i < 4; ++i) plan.apply(play(int(i + 1), V + int(i), 10 + 10 * i));
    take();
    const int order[4] = {0, 1, 3, 2};
    for (unsigned i = 0; i < 4; ++i)
        require(plan.voice(order[i]).id == V + int(i) && cmd[order[i]].op == PaulaChannelCommand::start && !cmd[order[i]].cut,
                "start " + std::to_string(i) + " takes channel " + std::to_string(order[i]));
    require(cmd[0].words == 4183 && cmd[0].period == 424 && cmd[0].volume == 64 && !cmd[0].loop, "start command fields");
    // A fifth at priority 25 steals priority 10 (channel 0) and cuts it.
    plan.apply(play(5, V + 4, 25));
    take();
    require(plan.voice(0).id == V + 4 && cmd[0].op == PaulaChannelCommand::start && cmd[0].cut && plan.counters().steals == 1,
            "a higher priority steals the lowest and cuts it");
    // A sixth at priority 5 is dropped; equal priority steals the oldest.
    plan.apply(play(6, V + 5, 5));
    require(plan.counters().drops == 1 && plan.activeVoices() == 4, "a lower priority is dropped");
    plan.apply(play(6, V + 6, 20)); // channel 1 had priority 20 (V+1), the oldest at 20
    take();
    require(plan.voice(1).id == V + 6 && plan.counters().steals == 2, "equal priority steals the oldest");
    // Stop by voice, by asset, and all.
    plan.apply(simple(SoundRequest::stop, V + 4));
    take();
    require(!plan.voice(0).active && cmd[0].op == PaulaChannelCommand::stop, "stop by voice");
    plan.apply(simple(SoundRequest::stop, 6));
    take();
    require(!plan.voice(1).active && cmd[1].op == PaulaChannelCommand::stop, "stop by asset");
    plan.apply(simple(SoundRequest::stop, V + 99));
    take();
    require(cmd[0].op == PaulaChannelCommand::none, "stopping an unknown voice writes nothing");
    plan.apply(simple(SoundRequest::stopAll));
    take();
    require(plan.activeVoices() == 0 && cmd[2].op == PaulaChannelCommand::stop && cmd[3].op == PaulaChannelCommand::stop, "stop all");

    // Expiry: 8,366 bytes at 424 is 49.9 PAL frames; one frame of margin.
    plan.apply(play(1, V + 10, 50));
    take();
    const unsigned c = 0;
    require(plan.voice(c).active, "expiry voice");
    plan.advance(49);
    require(plan.voice(c).active, "a one-shot is still live before its end");
    plan.advance(1);
    require(plan.voice(c).active, "and within the margin");
    plan.advance(1);
    require(!plan.voice(c).active && plan.counters().expired == 1, "then it expires with no command");
    take();
    require(cmd[c].op == PaulaChannelCommand::none, "expiry needs no hardware write");
    // Loops never expire.
    plan.apply(play(7, V + 11, 50));
    take();
    const unsigned lc = PaulaVoicePlan::side(0) == 0 ? 1 : 0; // the next start is on the other side
    require(plan.voice(lc).loop && cmd[lc].loop, "loop start");
    plan.advance(10000);
    require(plan.voice(lc).active, "a loop never expires");

    // Gain relative to the asset gain, fades and master.
    plan.apply(simple(SoundRequest::stopAll)); take();
    plan.apply(play(8, V + 20, 50, 0.5)); take();           // asset gain 0.5 played at 0.5: full bank volume
    int g = 0; for (unsigned i = 0; i < 4; ++i) if (plan.voice(i).id == V + 20) g = int(i);
    require(cmd[g].volume == 64, "the asset gain is the bank volume");
    plan.apply(gain(V + 20, 0.35)); take();                 // footstep x 0.7
    require(plan.voice(g).volume == 45 && cmd[g].volumeDirty && cmd[g].volume == 45, "a script gain scales relative to the asset gain");
    plan.apply(gain(8, 0, 100)); take();                    // by asset, fade over 5 fields
    require(plan.voice(g).fadeFields == 5, "fade fields");
    plan.advance(2); take();
    require(cmd[g].volumeDirty && plan.voice(g).volume < 45 && plan.voice(g).volume > 0, "a fade steps per field");
    plan.advance(3); take();
    require(plan.voice(g).volume == 0 && plan.voice(g).gainQ16 == 0, "a fade reaches its target");
    plan.apply(gain(V + 20, 0.5)); plan.setMasterQ16(32768); take();
    require(plan.voice(g).volume == 32, "master gain halves the volume");
    plan.setMasterQ16(65536); take();

    // Pitch: period = base / pitch, clamped at 124; expiry follows it.
    plan.apply(simple(SoundRequest::stopAll)); take();
    plan.apply(play(1, V + 30, 50, 1, 1.8)); take();
    int p = 0; for (unsigned i = 0; i < 4; ++i) if (plan.voice(i).id == V + 30) p = int(i);
    require(cmd[p].period == 236, "pitch 1.8 on period 424");
    plan.apply(pitch(V + 30, 1)); take();
    require(cmd[p].periodDirty && cmd[p].period == 424, "a later pitch rewrites the period");
    plan.apply(play(9, V + 31, 50, 1, 2.2)); take();
    int q = 0; for (unsigned i = 0; i < 4; ++i) if (plan.voice(i).id == V + 31) q = int(i);
    require(cmd[q].period == 124, "the period clamps at 124");

    // Pause mutes and frees the channel for stealing first; resume restores.
    plan.apply(simple(SoundRequest::stopAll)); take();
    for (unsigned i = 0; i < 4; ++i) plan.apply(play(int(i + 1), V + 40 + int(i), 90)); take();
    plan.apply(simple(SoundRequest::pause, V + 42)); take();
    int paused = 0; for (unsigned i = 0; i < 4; ++i) if (plan.voice(i).id == V + 42) paused = int(i);
    require(cmd[paused].volumeDirty && cmd[paused].volume == 0, "pause mutes");
    plan.apply(play(5, V + 50, 1)); take(); // even the lowest priority takes a paused voice
    require(plan.voice(paused).id == V + 50 && cmd[paused].cut, "a paused voice is stolen first");
    plan.apply(simple(SoundRequest::pauseAll)); take();
    for (unsigned i = 0; i < 4; ++i) require(plan.voice(i).volume == 0, "pause all mutes every voice");
    plan.apply(simple(SoundRequest::resumeAll)); take();
    for (unsigned i = 0; i < 4; ++i) require(plan.voice(i).volume == 64, "resume all restores every voice");

    // Music (category 5) and sounds outside the bank are not played.
    const auto before = plan.counters();
    plan.apply(play(1, V + 60, 100, 1, 1, 5));
    plan.apply(play(400, V + 61, 100));
    require(plan.counters().music == before.music + 1 && plan.counters().missing == before.missing + 1 &&
            plan.counters().plays == before.plays, "music and missing sounds are skipped");
    // A reserved channel is never allocated.
    PaulaVoicePlan reserved(bank); reserved.reserveChannels(1);
    for (unsigned i = 0; i < 4; ++i) reserved.apply(play(int(i + 1), V + 70 + int(i), 50));
    require(!reserved.voice(0).active && reserved.activeVoices() == 3 && reserved.counters().steals == 1,
            "WM's reserved channels stay free: the fourth start steals among the other three");
}

void testMerge() {
    PaulaChannelCommand a; a.op = PaulaChannelCommand::start; a.cut = false; a.volume = 10;
    PaulaChannelCommand v; v.volumeDirty = true; v.volume = 20;
    a.merge(v);
    require(a.op == PaulaChannelCommand::start && a.volume == 20, "a later volume rides with a pending start");
    PaulaChannelCommand s; s.op = PaulaChannelCommand::start; s.cut = false;
    a.merge(s);
    require(a.op == PaulaChannelCommand::start && a.cut, "a start over an unconsumed start cuts");
    PaulaChannelCommand stop; stop.op = PaulaChannelCommand::stop;
    PaulaChannelCommand idle; idle.merge(stop); idle.merge(s);
    require(idle.op == PaulaChannelCommand::start && idle.cut, "a start after a stop cuts");
}

void testSequencer() {
    auto bytes = pack({{1, 424, 64, false, 32768, 8366}, {2, 424, 64, true, 32768, 4000}});
    PaulaBank bank; require(bank.bind(bytes.data(), bytes.size()), "sequencer bank");
    downwell_test::PaulaModel model(bank.data());
    PaulaSequencer seq; PaulaVoicePlan plan(bank);
    PaulaChannelCommand mailbox[4]{};
    seq.begin(model);
    unsigned field = 0;
    auto vblank = [&] {
        plan.publish([&](unsigned c, const PaulaChannelCommand& m) { mailbox[c].merge(m); });
        model.setField(++field); seq.vblank(mailbox, model); plan.advance(1);
        model.render(PaulaVoicePlan::fieldClocks, 44100, nullptr);
    };
    auto writes = [&](unsigned f, unsigned c) {
        std::string w; for (const auto& x : model.writes()) if (x.field == f && x.channel == c) w += x.what; return w;
    };
    require(writes(0, 0) == "vpzD" || writes(0, 0) == "vpz" , "begin idles each channel on silence");
    plan.apply(play(1, V, 50));
    vblank();
    require(writes(1, 0) == "spv" && seq.state(0) == PaulaSequencer::oneShotStarted, "an idle start writes no DMA control");
    model.render(1000, 44100, nullptr);
    require(model.playingSample(0), "the start latches at the silent word's wrap");
    vblank();
    require(writes(2, 0) == "z" && seq.state(0) == PaulaSequencer::playing, "the next VBlank reloads silence");
    // A cut: DMA off in one VBlank, the new sample and DMA on in the next.
    plan.apply(play(1, V + 1, 60)); // the second voice goes right; force a cut on channel 0 by filling
    plan.apply(play(1, V + 2, 60)); plan.apply(play(1, V + 3, 60)); plan.apply(play(1, V + 4, 60));
    vblank();
    require(writes(3, 0) == "vd" && seq.state(0) == PaulaSequencer::stopped, "a cut turns DMA off first");
    vblank();
    require(writes(4, 0) == "spvD" && seq.state(0) == PaulaSequencer::oneShotStarted, "then programs and restarts it");
    vblank();
    require(writes(5, 0) == "z", "and reloads silence");
    // A stop: DMA off, then back to the idle silent loop.
    plan.apply(simple(SoundRequest::stopAll));
    vblank();
    require(writes(6, 0) == "vd", "a stop turns DMA off");
    vblank();
    require(writes(7, 0) == "vpzD" && seq.state(0) == PaulaSequencer::idle, "then idles on silence");
    // A loop keeps its reload on the sample.
    plan.apply(play(2, V + 9, 50));
    vblank(); vblank(); vblank();
    unsigned c = 0; for (unsigned i = 0; i < 4; ++i) if (plan.voice(i).id == V + 9) c = i;
    require(seq.state(c) == PaulaSequencer::playing && writes(9, c).empty() && writes(10, c).empty(),
            "a loop never reloads silence");
    model.render(PaulaVoicePlan::fieldClocks * 20, 44100, nullptr);
    require(model.playingSample(c), "and keeps playing");
}

struct Run { unsigned fields = 0; PaulaVoicePlan::Counters counters{}; };
Run runSchedule(const PaulaBank& bank, std::vector<std::int16_t>* audio, std::ostream* events = nullptr) {
    downwell_test::PaulaModel model(bank.data());
    PaulaSequencer seq; PaulaVoicePlan plan(bank);
    downwell_test::PaulaProbeSchedule schedule(bank);
    require(schedule.ready(), "the schedule finds its sounds");
    PaulaChannelCommand mailbox[4]{};
    seq.begin(model);
    // The probe: VBlank, then the main loop's step (submit, advance, publish).
    unsigned field = 0;
    for (;; ++field) {
        model.setField(field);
        seq.vblank(mailbox, model);
        const bool more = schedule.step(field, [&](const SoundRequest& r) {
                                            if (events && r.kind == SoundRequest::play) *events << field << ' ' << r.asset << '\n';
                                            plan.apply(r); },
                                        [&](std::uint32_t m) { plan.setMasterQ16(m); });
        plan.advance(1);
        plan.publish([&](unsigned c, const PaulaChannelCommand& m) { mailbox[c].merge(m); });
        model.renderFrames(PaulaVoicePlan::fieldClocks, 882, audio);
        if (!more) break;
    }
    return {field, plan.counters()};
}

void testSchedule() {
    // A synthetic bank with the ids the schedule names.
    std::vector<Spec> specs;
    for (unsigned id : {3u, 5u, 14u, 16u, 18u, 27u, 29u}) specs.push_back({id, 424, 48, false, 32768, 8000});
    specs.push_back({186, 424, 40, true, 32768, 8366});
    specs.push_back({343, 424, 40, true, 32768, 4184});
    auto bytes = pack(specs);
    PaulaBank bank; require(bank.bind(bytes.data(), bytes.size()), "schedule bank");
    const Run run = runSchedule(bank, nullptr);
    require(run.counters.steals == 1 && run.counters.drops == 1 && run.counters.missing == 0,
            "the schedule steals once and drops once");
}

std::vector<std::uint8_t> readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    require(bool(in), std::string("open ") + path);
    return {std::istreambuf_iterator<char>(in), {}};
}

void writeWav(const char* path, const std::vector<std::int16_t>& s) {
    std::ofstream out(path, std::ios::binary);
    auto u32 = [&](std::uint32_t v) { for (int i = 0; i < 4; ++i) out.put(char(v >> (8 * i))); };
    auto u16 = [&](unsigned v) { out.put(char(v)); out.put(char(v >> 8)); };
    const std::uint32_t bytes = std::uint32_t(s.size() * 2);
    out.write("RIFF", 4); u32(36 + bytes); out.write("WAVEfmt ", 8); u32(16); u16(1); u16(2);
    u32(44100); u32(44100 * 4); u16(4); u16(16); out.write("data", 4); u32(bytes);
    out.write(reinterpret_cast<const char*>(s.data()), std::streamsize(bytes));
    require(bool(out), std::string("write ") + path);
}

// Route logs (tests/core/SoundRequestLog.h): replay the requests field by
// field (two host ticks per 50 Hz field is not the game's pacing; one tick
// per field is, at the A500's one update per field). The log's bank lines
// bind the scene packs AmigaSoundBanks holds at that point (a slot change
// stops the slot's voices first, as unbind does); every play must find its
// sound in them.
struct ScenePacks {
    std::map<std::string, std::vector<std::uint8_t>> bytes;
    explicit ScenePacks(const std::string& full) {
        const std::string stem = full.substr(0, full.size() - 4); // ".bin"
        for (const char* name : {"run", "front", "results", "clear"}) bytes[name] = readFile((stem + "-" + name + ".bin").c_str());
    }
    PaulaBank bank(const std::string& name) const {
        PaulaBank b;
        if (name == "none") return b;
        auto i = bytes.find(name);
        require(i != bytes.end() && b.bind(i->second.data(), i->second.size()), "route bank " + name);
        return b;
    }
};

void replayRoute(const ScenePacks& packs, const char* path) {
    std::ifstream in(path);
    require(bool(in), std::string("open ") + path);
    PaulaBank resident, scene;
    PaulaVoicePlan plan(resident, &scene);
    std::string line, residentName = "none", sceneName = "none";
    unsigned tick = 0, peak = 0, bankLines = 0; bool first = true;
    std::map<int, unsigned> missing;
    static const char* names[] = {"play", "stop", "stopall", "pauseall", "resumeall", "gain", "pitch", "pause", "resume"};
    while (std::getline(in, line)) {
        std::istringstream f(line);
        unsigned t; std::string kind; f >> t >> kind;
        if (first) { tick = t; first = false; }
        // The main loop publishes once per frame, then the next frame advances.
        if (t > tick) { plan.publish([](unsigned, const PaulaChannelCommand&) {}); plan.advance(t - tick); tick = t; }
        if (kind == "bank") {
            std::string r, s; f >> r >> s; ++bankLines;
            if (r != residentName) { plan.stopSlot(0); resident = packs.bank(r); residentName = r; }
            if (s != sceneName) { plan.stopSlot(1); scene = packs.bank(s); sceneName = s; }
            continue;
        }
        int asset, voice; unsigned priority, loop, cut; int category; double g, p; unsigned fade = 0;
        f >> asset >> voice >> priority >> loop >> cut >> category >> g >> p >> fade;
        SoundRequest r;
        unsigned k = 0; while (k < 9 && kind != names[k]) ++k;
        require(k < 9, "route log kind " + kind);
        r.kind = SoundRequest::Kind(k); r.asset = std::int16_t(asset); r.voice = voice; r.priority = std::uint8_t(priority);
        r.loop = loop; r.cut = cut; r.category = std::int8_t(category); r.gainValue = real(g); r.pitchValue = real(p);
        r.fadeMs = std::uint16_t(fade);
        const auto before = plan.counters().missing;
        plan.apply(r);
        if (plan.counters().missing != before) ++missing[asset];
        if (plan.activeVoices() > peak) peak = plan.activeVoices();
    }
    plan.publish([](unsigned, const PaulaChannelCommand&) {});
    const auto& c = plan.counters();
    std::cout << "Route " << path << ": plays " << c.plays << " steals " << c.steals << " drops " << c.drops
              << " missing " << c.missing << " music " << c.music << " expired " << c.expired
              << " stops " << c.stops << " peak " << peak << " voices\n";
    for (const auto& m : missing) std::cout << "  missing sound " << m.first << " x" << m.second << "\n";
    require(bankLines > 0, std::string(path) + ": the log names its bank sets");
    require(c.missing == 0, std::string(path) + ": every requested sound is in a loaded bank");
}
}

// W2.5 bank slots: a scene bank over the resident one, and its retirement.
void testSlots() {
    auto residentBytes = pack({{1, 424, 64, false, 32768, 8366}, {2, 424, 64, false, 32768, 8366}});
    auto sceneBytes = pack({{2, 424, 40, false, 32768, 8366}, {3, 424, 64, true, 32768, 4000}});
    PaulaBank resident, scene;
    require(resident.bind(residentBytes.data(), residentBytes.size()), "resident bank");
    downwell_test::PaulaModel model(resident.data(), nullptr);
    PaulaSequencer seq; PaulaVoicePlan plan(resident, &scene);
    PaulaChannelCommand mailbox[4]{};
    seq.begin(model);
    unsigned field = 0;
    auto vblank = [&] {
        plan.publish([&](unsigned c, const PaulaChannelCommand& m) { mailbox[c].merge(m); });
        model.setField(++field); seq.vblank(mailbox, model); plan.advance(1);
        model.render(PaulaVoicePlan::fieldClocks, 44100, nullptr);
    };
    auto fetching = [&](unsigned slot) { unsigned n = 0; for (unsigned c = 0; c < 4; ++c) n += model.fetching(c, slot); return n; };
    require(plan.idle(), "a new plan is idle");
    // No scene bank bound: ids 1 and 2 come from the resident bank, 3 is missing.
    plan.apply(play(3, V, 50));
    require(plan.idle(), "a missing sound leaves the plan idle");
    require(plan.counters().missing == 1 && !plan.activeVoices(), "an unbound scene bank finds nothing");
    require(scene.bind(sceneBytes.data(), sceneBytes.size()), "scene bank");
    model = downwell_test::PaulaModel(resident.data(), scene.data());
    seq.begin(model);
    plan.apply(play(1, V + 1, 50));
    plan.apply(play(2, V + 2, 50));
    plan.apply(play(3, V + 3, 50));
    require(!plan.idle(), "claimed voices are not idle");
    vblank();
    unsigned slot0 = 0, slot1 = 0;
    for (unsigned c = 0; c < 4; ++c) {
        const auto& v = plan.voice(c);
        if (!v.active) continue;
        if (v.asset == 1) require(v.slot == 0, "id 1 plays from the resident bank");
        if (v.asset == 2) require(v.slot == 1 && v.volume == 40, "the scene bank's id 2 wins");
        if (v.asset == 3) require(v.slot == 1 && v.loop, "id 3 plays from the scene bank");
        (v.slot ? slot1 : slot0) += 1;
    }
    require(slot0 == 1 && slot1 == 2, "three voices over two banks");
    model.render(1000, 44100, nullptr);
    require(fetching(1) == 2 && fetching(0) == 1, "both banks sound");
    // Retire the scene bank: its voices stop, the resident one plays on.
    plan.stopSlot(1);
    require(plan.activeVoices() == 1, "stopSlot leaves the resident voice");
    vblank();
    require(fetching(1) == 0 && fetching(0) == 1, "one VBlank later nothing fetches the scene bank");
    // A fresh scene play stopped before it reached the hardware costs nothing.
    plan.apply(play(3, V + 4, 50));
    plan.stopSlot(1);
    vblank(); vblank();
    require(fetching(1) == 0, "a fresh scene voice never starts");
    scene = PaulaBank{};
    plan.apply(play(3, V + 5, 50));
    require(plan.counters().missing == 2, "an unbound bank's sounds are missing again");
}

int main(int argc, char** argv) {
    testBank();
    testPlan();
    testMerge();
    testSequencer();
    testSlots();
    testSchedule();
    std::cout << "Paula mixer unit checks passed\n";
    if (argc < 2) return 0;
    if (argc >= 3 && std::strcmp(argv[2], "--route") == 0) {
        const ScenePacks packs(argv[1]);
        for (int i = 3; i < argc; ++i) replayRoute(packs, argv[i]);
        return 0;
    }
    const auto bytes = readFile(argv[1]);
    PaulaBank bank; require(bank.bind(bytes.data(), bytes.size()), "the real pack binds");
    std::vector<std::int16_t> audio;
    std::ofstream events(argc >= 3 ? std::string(argv[2]) + ".events" : std::string("/dev/null"));
    const Run run = runSchedule(bank, &audio, &events);
    require(run.counters.steals == 1 && run.counters.drops == 1 && run.counters.missing == 0,
            "the real schedule steals once and drops once");
    if (argc >= 3) writeWav(argv[2], audio);
    std::cout << "Paula probe schedule: " << run.fields << " fields, " << run.counters.plays << " plays\n";
    return 0;
}
