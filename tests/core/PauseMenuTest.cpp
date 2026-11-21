#include "core/PauseMenu.h"
#include "downwell-menu-text.h"
#include "downwell-palettes.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace downwell;
using Menu = PauseMenu;
using Screen = Menu::Screen;
using Action = Menu::Action;

static Menu::Input press(char key) {
    Menu::Input in;
    if (key == 'L') { in.leftPressed = in.left = true; }
    if (key == 'R') { in.rightPressed = in.right = true; }
    if (key == 'F') in.confirm = true;
    if (key == 'C') in.cancel = true;
    if (key == 'P') in.pause = true;
    return in;
}

static void saveFile() {
    SaveFile defaults = SaveFile::parse("", 0);
    assert(defaults == SaveFile{});
    assert(!defaults.noBgm && !defaults.hardUnlocked && !defaults.hardMode && !defaults.showTimer);
    assert(defaults.masterGainTenths == 10 && defaults.fastestGame == -1);
    // GameMaker's own text, CRLF, quotes, unknown keys and sections.
    const char* pc =
        "[stats]\r\ngems=\"3412.000000\"\r\nstyle=\"2.000000\"\r\nshader=\"5.000000\"\r\n"
        "nobgm=\"1.000000\"\r\nhardUnlocked=\"1.000000\"\r\njp=\"0.000000\"\r\n"
        "recordFurthestReached=\"113.000000\"\r\nrecordRunGem=\"812.000000\"\r\n"
        "recordMaxCombo=\"41.000000\"\r\nrecordFastestGame=\"75123.000000\"\r\n"
        "[option]\r\nmasterGain=\"0.29999999\"\r\nhardMode=\"1.000000\"\r\nshowtimer=\"-1.000000\"\r\n"
        "fullscreenSet=\"1.000000\"\r\n[other]\r\ngems=\"9\"\r\n";
    SaveFile s = SaveFile::parse(pc, unsigned(std::strlen(pc)));
    assert(s.totalGems == 3412 && s.style == 2 && s.shader == 5 && s.noBgm && s.hardUnlocked);
    assert(s.furthestReached == 113 && s.bestRunGems == 812 && s.bestCombo == 41);
    assert(s.fastestGame == 75123 && s.fastestGameHard == -1);
    assert(s.masterGainTenths == 3 && s.hardMode && !s.showTimer);
    char out[SaveFile::formatCapacity];
    const unsigned n = s.format(out, sizeof out);
    assert(n > 0 && n < sizeof out);
    assert(SaveFile::parse(out, n) == s);
    assert(std::strstr(out, "gems=\"3412.000000\"") && std::strstr(out, "masterGain=\"0.300000\""));
    assert(std::strstr(out, "nobgm=\"1.000000\"") && std::strstr(out, "showtimer=\"-1.000000\""));
    assert(s.format(out, 40) == 0);
    for (unsigned g = 0; g <= 10; ++g) {
        SaveFile v; v.masterGainTenths = g;
        const unsigned m = v.format(out, sizeof out);
        assert(SaveFile::parse(out, m).masterGainTenths == g);
    }
    // Out-of-range style is clamped, negatives fall back to zero.
    const char* odd = "[stats]\ngems=-5\nstyle=9\n";
    s = SaveFile::parse(odd, unsigned(std::strlen(odd)));
    assert(s.totalGems == 0 && s.style == 4);
}

static void unlocks() {
    UnlockProgress u = UnlockProgress::from(0);
    assert(u.progress == 0 && u.styleUnlock == 0 && u.paletteUnlock == 0 && !u.maxed);
    u = UnlockProgress::from(999); assert(u.progress == 0);
    u = UnlockProgress::from(1000); assert(u.progress == 1 && u.paletteUnlock == 1 && u.styleUnlock == 0);
    u = UnlockProgress::from(2999); assert(u.progress == 2 && u.paletteUnlock == 2);
    u = UnlockProgress::from(3000); assert(u.progress == 3 && u.styleUnlock == 1 && u.paletteUnlock == 2);
    u = UnlockProgress::from(12500); assert(u.progress == 8 && u.styleUnlock == 2 && u.paletteUnlock == 6);
    u = UnlockProgress::from(35000); assert(u.progress == 14 && u.styleUnlock == 4 && u.paletteUnlock == 10);
    u = UnlockProgress::from(500000);
    assert(u.progress == 41 && u.maxed && u.styleUnlock == 4 && u.paletteUnlock == 37);
    assert(UnlockProgress::pauseGoal(0) == 0 && UnlockProgress::pauseGoal(1000) == 1);
    assert(UnlockProgress::pauseBarLength(500) == 48 && UnlockProgress::pauseBarLength(1500) == 48);
    assert(UnlockProgress::pauseBarLength(0) == 0 && UnlockProgress::pauseBarLength(999) == 95);
}

static void navigation() {
    SaveFile save; const UnlockProgress unlock = UnlockProgress::from(5000); // 4 palettes
    Menu m; m.openPause(false);
    assert(m.open() && m.screen == Screen::pause && m.sound == 326 && m.maxMenu == 6);
    // Wrap left from RESUME to QUIT and back.
    assert(m.step(press('L'), save, unlock) == Action::none && m.cursor == 6 && m.sound == 324);
    assert(m.step(press('R'), save, unlock) == Action::none && m.cursor == 0);
    // Animation: powan +0.4 until >= 1, wholePowan 4 -> 0, wholePowan2 2 -> 0.
    Menu a; a.openPause(false);
    a.step({}, save, unlock);
    assert(a.powanTenths == 4 && a.wholePowan == 3 && a.wholePowan2 == 1);
    for (int i = 0; i < 5; ++i) a.step({}, save, unlock);
    assert(a.powanTenths == 12 && a.wholePowan == 0 && a.wholePowan2 == 0);
    // RETRY asks; NO returns to RETRY, YES restarts.
    m.step(press('R'), save, unlock);
    m.step(press('F'), save, unlock);
    assert(m.screen == Screen::confirm && m.confirmType == Menu::Confirm::retry && m.cursor == 0);
    m.step(press('R'), save, unlock);
    assert(m.step(press('F'), save, unlock) == Action::none && m.screen == Screen::pause && m.cursor == 1);
    m.step(press('F'), save, unlock);
    assert(m.step(press('F'), save, unlock) == Action::retry && !m.open());
    // At the surface RETRY is QUICK START and needs no confirmation.
    m.openPause(true); m.step(press('R'), save, unlock);
    assert(m.step(press('F'), save, unlock) == Action::retry);
    m.openPause(true); m.step(press('R'), save, unlock); m.step(press('R'), save, unlock);
    assert(m.step(press('F'), save, unlock) == Action::styleSelect);
    // Cancel backs out of the confirm to its item, then resumes.
    m.openPause(false); m.cursor = 2; m.step(press('F'), save, unlock);
    assert(m.screen == Screen::confirm && m.confirmType == Menu::Confirm::styleSelect);
    m.step(press('C'), save, unlock);
    assert(m.screen == Screen::pause && m.cursor == 2);
    assert(m.step(press('C'), save, unlock) == Action::resume && !m.open());
    // QUIT confirms; pause input closes any screen.
    m.openPause(false); m.cursor = 6; m.step(press('F'), save, unlock);
    assert(m.confirmType == Menu::Confirm::quit);
    assert(m.step(press('F'), save, unlock) == Action::quit);
    m.openPause(false); m.cursor = 5; m.step(press('F'), save, unlock);
    assert(m.screen == Screen::options);
    assert(m.step(press('P'), save, unlock) == Action::resume && !m.open());
    // RESUME.
    m.openPause(false);
    assert(m.step(press('F'), save, unlock) == Action::resume && m.sound == 322);
}

static void paletteMenu() {
    SaveFile save; const UnlockProgress unlock = UnlockProgress::from(5000);
    assert(unlock.paletteUnlock == 4);
    Menu m; m.openPause(false); m.cursor = 3; m.step(press('F'), save, unlock);
    assert(m.screen == Screen::palette);
    m.step(press('L'), save, unlock);
    assert(save.shader == 4 && m.saveChanged && m.sound == 338 && m.powanx == -4);
    m.step(press('R'), save, unlock); assert(save.shader == 0);
    m.step(press('R'), save, unlock); assert(save.shader == 1);
    // Holding right repeats on the 30th update and then every eighth.
    Menu::Input held; held.right = true;
    unsigned changes = 0;
    for (int i = 0; i < 46; ++i) { m.step(held, save, unlock); changes += m.saveChanged; }
    assert(changes == 3);
    m.step({}, save, unlock); assert(m.buttonHold == 0);
    m.step(press('F'), save, unlock);
    assert(m.screen == Screen::pause && m.cursor == 3 && m.sound == 318);
}

static void options() {
    SaveFile save; const UnlockProgress unlock;
    Menu m; m.openPause(false); m.cursor = 5; m.step(press('F'), save, unlock);
    assert(m.screen == Screen::options && m.maxMenu == 3);
    m.step(press('R'), save, unlock); m.step(press('F'), save, unlock);
    assert(save.noBgm && m.saveChanged);
    m.step(press('R'), save, unlock); m.step(press('F'), save, unlock);
    assert(save.showTimer);
    m.step(press('R'), save, unlock);
    for (unsigned expected : {9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u, 10u}) {
        m.step(press('F'), save, unlock);
        assert(save.masterGainTenths == expected);
    }
    m.step(press('R'), save, unlock); assert(m.cursor == 0);
    m.step(press('F'), save, unlock);
    assert(m.screen == Screen::pause && m.cursor == 5);
    m.cursor = 4; m.step(press('F'), save, unlock);
    assert(m.screen == Screen::stats);
    m.step(press('F'), save, unlock);
    assert(m.screen == Screen::pause && m.cursor == 4);
}

struct Recorder {
    std::vector<std::string> texts;
    unsigned dithers = 0, rects = 0, lines = 0, progressCount = 0, sprites = 0;
    int whiteLine = -1;
    void dither(int frame, int, int) { assert(frame >= 0 && frame <= 7); ++dithers; }
    void blackRect(int x, int, int w, int h) { assert(x == 0 && w == 161 && h == 81); ++rects; }
    void text(const char* s, int, int, int halign, Menu::Kind) {
        assert(s && halign >= -1 && halign <= 1); texts.push_back(s);
    }
    void line(int, int, int w, bool white) { ++lines; if (white && whiteLine < 0) whiteLine = w; }
    void sprite(int id, int frame, int, int) { assert((id == 27 && frame < 8) || (id == 134 && frame < 4)); ++sprites; }
    bool has(const char* s) const { for (auto& t : texts) if (t == s) return true; return false; }
};
struct Sink : Recorder { void progress(int, int) { ++progressCount; } };

static void drawing() {
    PauseMenuText text;
    assert(text.configure(menu_text::entries, menu_text::count, "english"));
    assert(!std::strcmp(text.paused, "PAUSED") && !std::strcmp(text.area[1], "CAVERN"));
    assert(std::strchr(text.confirmReset, '#') && !std::strchr(text.confirmReset, '*'));
    assert(palettes::count == 38 && !std::strcmp(palettes::entries[0].name, "DOWNWELL"));
    assert(palettes::entries[0].remap(0x000) == 0x000 && palettes::entries[0].remap(0xfff) == 0xfff &&
           palettes::entries[0].remap(0xf00) == 0xf00);
    SaveFile save; save.totalGems = 1500; save.shader = 2;
    Menu::View view{&text, palettes::entries, palettes::count, &save, UnlockProgress::from(1500), 1, 2, false};
    Menu m; m.openPause(false);
    Sink sink; assert(m.draw(sink, view));
    assert(sink.dithers == 8 * 21 * 2 && sink.rects == 1 && sink.progressCount == 1);
    assert(sink.has("PAUSED") && sink.has("CAVERN-2") && sink.has("RESUME") && sink.has("QUIT"));
    assert(sink.has("1500") && sink.has("2000") && sink.whiteLine == 49);
    view.hard = true; Sink hard; m.draw(hard, view); assert(hard.has("CAVERN-H2"));
    m.cursor = 3; const UnlockProgress unlock = UnlockProgress::from(5000);
    view.unlock = unlock; m.step(press('F'), save, unlock);
    Sink pal; m.draw(pal, view);
    assert(pal.has("2.AQUA") && pal.has("3.GBOY") && pal.has("1.MATCHA") && pal.has("4.VBOY") && pal.has("0.DOWNWELL"));
    assert(pal.sprites == 2);
    m.enter(Screen::stats, 0); save.furthestReached = 113; save.bestCombo = 41; save.fastestGame = 75123;
    Sink st; m.draw(st, view);
    assert(st.has("H 1-3") && st.has("41") && st.has("20:52:05") && st.has("BACK"));
    save.fastestGame = -1; Sink st2; m.draw(st2, view); assert(st2.has("NO RECORD"));
    m.enter(Screen::options, 0); save.masterGainTenths = 10;
    Sink op; m.draw(op, view);
    assert(op.has("BGM: ON") && op.has("TIMER: OFF") && op.has("VOLUME: 10"));
    m.enter(Screen::confirm, 0); m.confirmType = Menu::Confirm::quit;
    Sink cf; m.draw(cf, view); assert(cf.has(text.confirmQuit) && cf.has("YES") && cf.has("NO"));
    save.totalGems = 600000; view.unlock = UnlockProgress::from(600000);
    m.enter(Screen::pause, 0); Sink mx; m.draw(mx, view); assert(mx.has("MAXED!") && mx.lines == 0);
    char t[16]; Menu::timeString(t, 59, false); assert(!std::strcmp(t, "00:00:98"));
    Menu::timeString(t, 3600, false); assert(!std::strcmp(t, "01:00:00"));
    Menu::reachedString(t, 0); assert(!std::strcmp(t, "0-0"));
    Menu::reachedString(t, 90); assert(!std::strcmp(t, "CLEARED"));
}

int main() {
    saveFile(); unlocks(); navigation(); paletteMenu(); options(); drawing();
    std::puts("pause menu tests passed");
}
