#ifndef DOWNWELL_CORE_PAUSE_MENU_H
#define DOWNWELL_CORE_PAUSE_MENU_H
#include "Palettes.h"
#include "SaveFile.h"
#include "ShopText.h"
#include "UnlockProgress.h"
#include <cstdint>
namespace downwell {
// The English strings of objPauseMenu and its sub-menus (langString keys).
struct PauseMenuText {
    const char *paused = nullptr, *resume = nullptr, *retry = nullptr, *styleSelect = nullptr,
        *palette = nullptr, *stats = nullptr, *option = nullptr, *quickStart = nullptr,
        *quit = nullptr, *confirmQuit = nullptr, *confirmReset = nullptr, *yes = nullptr,
        *no = nullptr, *back = nullptr, *bgm = nullptr, *timer = nullptr, *volume = nullptr,
        *on = nullptr, *off = nullptr, *maxed = nullptr, *furthest = nullptr, *gems = nullptr,
        *combo = nullptr, *time = nullptr;
    const char* area[6]{};
    const char* styleName[5]{}; // unlockNotice names (DeathMenu)
    bool configure(const ShopLanguageEntry* entries, unsigned count, const char* language) {
        *this = {};
        if (!entries || !language) return false;
        const ShopLanguageLookup l{entries, count, language};
        const char* areaKeys[6] = {"area0", "area1", "area2", "area3", "area4", "area5"};
        for (unsigned i = 0; i < 6; ++i) if (!l.lookup(areaKeys[i], area[i])) return false;
        const char* styleKeys[5] = {"styleName0", "styleName1", "styleName2", "styleName3", "styleName4"};
        for (unsigned i = 0; i < 5; ++i) if (!l.lookup(styleKeys[i], styleName[i])) return false;
        return l.lookup("menuPaused", paused) && l.lookup("menuResume", resume) &&
            l.lookup("menuRetry", retry) && l.lookup("menuStyleSelect", styleSelect) &&
            l.lookup("menuPalette", palette) && l.lookup("menuStats", stats) &&
            l.lookup("menuOption", option) && l.lookup("menuQuickStart", quickStart) &&
            l.lookup("menuQuit", quit) && l.lookup("menuConfirmQuit", confirmQuit) &&
            l.lookup("menuConfirmReset", confirmReset) && l.lookup("menuYes", yes) &&
            l.lookup("menuNo", no) && l.lookup("menuBack", back) && l.lookup("menuBGM", bgm) &&
            l.lookup("menuTimer", timer) && l.lookup("menuVolume", volume) &&
            l.lookup("menuOn", on) && l.lookup("menuOff", off) && l.lookup("menuMaxed", maxed) &&
            l.lookup("statFurthest", furthest) && l.lookup("statGems", gems) &&
            l.lookup("statCombo", combo) && l.lookup("statFastestTime", time);
    }
};

// objPauseMenu, ConfirmMenu, ShaderMenu, RecordMenu and OptionMenuPC as
// one owner: each source menu destroys itself and creates the next with a
// preset cursor, which here is enter(screen, cursor). Amiga adaptations:
// - Options keep Back, BGM, Timer and Volume; DISPLAY and LANGUAGE (PC
//   window, scale and language menus) have no Amiga meaning.
// - The PC build's QUIT item is kept; confirming it leaves the game.
// Each step leaves its soundPlayOL id in `sound`; the owner plays it
// (GameplayWorld::playUiSound) and pauses/resumes all sounds around it.
struct PauseMenu {
    enum class Screen : unsigned char { closed, pause, confirm, palette, stats, options };
    enum class Action : unsigned char { none, resume, retry, styleSelect, quit };
    enum class Confirm : unsigned char { retry = 0, styleSelect = 1, quit = 4 };
    struct Input {
        bool leftPressed = false, rightPressed = false, left = false, right = false;
        bool confirm = false; // global.dUp
        bool cancel = false;  // global.padCancel
        bool pause = false;   // global.pauseInput
    };
    Screen screen = Screen::closed;
    Confirm confirmType = Confirm::retry;
    bool atSurface = false;
    int cursor = 0, maxMenu = 0;
    int powanTenths = 0, wholePowan = 0, wholePowan2 = 0, powanx = 0, buttonHold = 0;
    unsigned runQuarters = 0; // ShaderMenu Draw's runFrame, in 0.25 steps
    int sound = -1;           // last soundPlayOL id requested this update
    bool saveChanged = false; // an ini_write_real happened this update

    bool open() const { return screen != Screen::closed; }
    // objControlerN: pause opens objPauseMenu (sound 326) with cursor 0.
    void openPause(bool surface) { atSurface = surface; enter(Screen::pause, 0); sound = 326; }
    void close() { screen = Screen::closed; }
    void enter(Screen s, int c) {
        screen = s; cursor = c; powanTenths = 0; wholePowan = 4; wholePowan2 = 2;
        powanx = 0; buttonHold = 0; runQuarters = 0;
        maxMenu = s == Screen::pause ? 6 : s == Screen::confirm ? 1 :
                  s == Screen::options ? 3 : 0;
    }
    static int sign(int v) { return (v > 0) - (v < 0); }
    void moveCursor(const Input& in) {
        if (in.rightPressed) { ++cursor; powanTenths = 0; powanx = 1; sound = 324; }
        if (in.leftPressed) { --cursor; powanTenths = 0; powanx = 1; sound = 324; }
        if (cursor > maxMenu) cursor = 0;
        if (cursor < 0) cursor = maxMenu;
    }
    void stepPalette(int delta, SaveFile& save, const UnlockProgress& unlock) {
        int shader = int(save.shader) + delta;
        if (shader > int(unlock.paletteUnlock)) shader = 0;
        if (shader < 0) shader = int(unlock.paletteUnlock);
        save.shader = unsigned(shader);
        saveChanged = true;
        powanTenths = 0; powanx = 4 * delta; sound = 338;
    }

    Action step(const Input& in, SaveFile& save, const UnlockProgress& unlock) {
        sound = -1; saveChanged = false;
        if (screen == Screen::closed) return Action::none;
        if (powanTenths < 10) powanTenths += 4;
        wholePowan -= sign(wholePowan);
        wholePowan2 -= sign(wholePowan2);
        powanx -= sign(powanx);
        Action action = Action::none;
        bool cancel = in.cancel;
        switch (screen) {
        case Screen::pause:
            moveCursor(in);
            if (in.confirm) {
                sound = cursor == 0 ? 322 : 321;
                if (cursor == 0) action = Action::resume;
                else if (cursor == 1) {
                    if (atSurface) action = Action::retry;
                    else { confirmType = Confirm::retry; enter(Screen::confirm, 0); }
                } else if (cursor == 2) {
                    if (atSurface) action = Action::styleSelect;
                    else { confirmType = Confirm::styleSelect; enter(Screen::confirm, 0); }
                } else if (cursor == 3) enter(Screen::palette, 0);
                else if (cursor == 4) enter(Screen::stats, 0);
                else if (cursor == 5) enter(Screen::options, 0);
                else if (cursor == 6) { confirmType = Confirm::quit; enter(Screen::confirm, 0); }
                cancel = false;
            }
            if (cancel) action = Action::resume;
            break;
        case Screen::confirm: {
            moveCursor(in);
            const int back = confirmType == Confirm::retry ? 1 : confirmType == Confirm::styleSelect ? 2 : 6;
            if (in.confirm) {
                if (cursor == 0) {
                    sound = 321;
                    action = confirmType == Confirm::retry ? Action::retry :
                             confirmType == Confirm::styleSelect ? Action::styleSelect : Action::quit;
                } else if (cursor == 1) { enter(Screen::pause, back); sound = 322; }
                cancel = false;
            }
            if (cancel) enter(Screen::pause, back);
            break;
        }
        case Screen::palette:
            if (in.rightPressed) stepPalette(1, save, unlock);
            if (in.right && !in.left && ++buttonHold >= 30) { stepPalette(1, save, unlock); buttonHold -= 8; }
            if (in.leftPressed) stepPalette(-1, save, unlock);
            if (!in.right && in.left && --buttonHold <= -30) { stepPalette(-1, save, unlock); buttonHold += 8; }
            if (in.left == in.right) buttonHold = 0;
            if ((in.confirm || in.cancel) && cursor == 0) { enter(Screen::pause, 3); sound = 318; }
            break;
        case Screen::stats:
            moveCursor(in);
            if (in.confirm) { if (cursor == 0) { enter(Screen::pause, 4); sound = 322; } cancel = false; }
            if (cancel) enter(Screen::pause, 4);
            break;
        case Screen::options:
            moveCursor(in);
            if (in.confirm) {
                if (cursor == 0) { enter(Screen::pause, 5); sound = 322; }
                else sound = 321;
                if (cursor == 1) { save.noBgm = !save.noBgm; saveChanged = true; }
                if (cursor == 2) { save.showTimer = !save.showTimer; saveChanged = true; }
                if (cursor == 3) {
                    save.masterGainTenths = save.masterGainTenths ? save.masterGainTenths - 1 : 10;
                    saveChanged = true; sound = 321;
                }
            }
            if (cancel) enter(Screen::pause, 5);
            break;
        case Screen::closed: break;
        }
        // objPauseMenu and every sub-menu destroy themselves on pauseInput.
        if (in.pause && action == Action::none) action = Action::resume;
        if (action != Action::none) close();
        return action;
    }

    // ---- Draw, in 160x284 view-local coordinates (vx = vy = 0) ----
    // Sink: dither(frame,x,y) sprDitherFade; blackRect(x,y,w,h) inclusive
    // source rectangle; text(s,x,y,halign,kind) with halign -1/0/1 and kind
    // red, white, borderBlack (white on a black outline), borderRed (red on
    // a black outline); line(x,y,w,white); progress(x,y) sprProgress;
    // sprite(id,frame,x,y) for the palette screen's runner and gem.
    enum class Kind : unsigned char { red, white, borderBlack, borderRed };
    struct View {
        const PauseMenuText* text = nullptr;
        const PaletteEntry* palettes = nullptr;
        unsigned paletteCount = 0;
        const SaveFile* save = nullptr;
        UnlockProgress unlock;
        int area = 1, level = 1;
        bool hard = false;
    };
    static unsigned decimal(char* out, std::uint64_t value) {
        char buffer[24]; unsigned n = 0;
        // 32-bit divisions only: the 68000 build has no 64-bit helpers.
        do {
            const std::uint64_t quotient = DeathMenuProgress::divideSmall(value, 10);
            buffer[n++] = char('0' + unsigned(value - quotient * 10));
            value = quotient;
        } while (value);
        for (unsigned i = 0; i < n; ++i) out[i] = buffer[n - 1 - i];
        out[n] = 0;
        return n;
    }
    static void append(char* out, unsigned& n, const char* s) { while (*s) out[n++] = *s++; out[n] = 0; }
    static void two(char* out, unsigned& n, unsigned v) {
        out[n++] = char('0' + v / 10 % 10); out[n++] = char('0' + v % 10); out[n] = 0;
    }
    // stringGameTimeConvert(Hard): MM:SS:CC, or the "no record" text.
    static void timeString(char* out, std::int32_t frames, bool hard) {
        unsigned n = 0; out[0] = 0;
        if (frames <= 0) { append(out, n, hard ? " " : "NO RECORD"); return; }
        const unsigned whole = unsigned(frames) / 60;
        const unsigned hundredths = (unsigned(frames) % 60) * 100 / 60;
        if (unsigned(frames) < 3600) append(out, n, "00");
        else two(out, n, whole / 60);
        out[n++] = ':'; two(out, n, whole % 60); out[n++] = ':'; two(out, n, hundredths);
        out[n] = 0;
    }
    // stringAreaLevelRecord.
    static void reachedString(char* out, unsigned record) {
        unsigned n = 0; out[0] = 0;
        if (record >= 100) { append(out, n, "H "); record -= 100; }
        if (record == 0) append(out, n, "0-0");
        else if (record == 51) append(out, n, "BOSS");
        else if (record == 90) append(out, n, "CLEARED");
        else if (record / 10 >= 1 && record / 10 <= 4 && record % 10 >= 1 && record % 10 <= 3) {
            out[n++] = char('0' + record / 10); out[n++] = '-'; out[n++] = char('0' + record % 10); out[n] = 0;
        }
    }
    int powan() const { return powanTenths / 10; } // draw_text pixel snap
    template<class Sink> void item(Sink& sink, const char* s, int i, int x, int y, int halign) const {
        if (cursor == i) {
            sink.text(s, x + powan() - 1, y + 1, halign, Kind::red);
            sink.text(s, x + powan(), y + 1 - powan(), halign, Kind::white);
        } else sink.text(s, x, y, halign, Kind::red);
    }
    template<class Sink> void backdrop(Sink& sink, int top) const {
        const int bottom = top + 80;
        for (int i = 0; i <= 7; ++i)
            for (int x = 0; x <= 160; x += 8) {
                sink.dither(i, x, top - i * 8);
                sink.dither(i, x, bottom + i * 8);
            }
        sink.blackRect(0, top, 161, 81);
    }
    template<class Sink> bool draw(Sink& sink, const View& v) {
        if (screen == Screen::closed) return true;
        if (!v.text || !v.save || !v.palettes || !v.paletteCount) return false;
        const PauseMenuText& t = *v.text;
        char buffer[48];
        const auto powany = [&](int i) { return cursor >= i ? -2 : 0; };
        switch (screen) {
        case Screen::pause: {
            const int top = 82;
            backdrop(sink, top);
            sink.text(t.paused, 80, top - 16 + wholePowan, 0, Kind::borderBlack);
            unsigned n = 0; buffer[0] = 0;
            const int area = v.area < 0 ? 0 : v.area > 5 ? 5 : v.area;
            append(buffer, n, t.area[area]);
            if (area > 0) {
                append(buffer, n, v.hard ? "-H" : "-");
                n += decimal(buffer + n, unsigned(v.level < 0 ? 0 : v.level));
            }
            sink.text(buffer, 80, top - 16 + 14 + wholePowan, 0, Kind::borderBlack);
            const char* items[7] = {t.resume, atSurface ? t.quickStart : t.retry, t.styleSelect,
                                    t.palette, t.stats, t.option, t.quit};
            for (int i = 0; i <= maxMenu; ++i)
                item(sink, items[i], i, 16 + i * 8, top + 28 + i * 10 + powany(i) + wholePowan2, -1);
            // The PC build moves the gem progress 16 pixels down.
            const int gemY = top + 72 + wholePowan2 - 8 + 42 + 16;
            sink.progress(80, gemY - 8);
            decimal(buffer, v.save->totalGems);
            sink.text(buffer, 72, gemY, 1, Kind::borderBlack);
            sink.text("/", 80, gemY, 0, Kind::borderBlack);
            const unsigned gr = UnlockProgress::pauseGoal(v.save->totalGems);
            if (!v.unlock.maxed) {
                decimal(buffer, DeathMenuProgress::goals[gr]);
                sink.text(buffer, 88, gemY, -1, Kind::borderBlack);
                const int barY = gemY + 14, barX = 80 - 48;
                sink.line(barX, barY, 97, false);
                sink.line(barX + 1, barY + 1, 95, false);
                sink.line(barX + 1, barY - 1, 95, false);
                const int length = UnlockProgress::pauseBarLength(v.save->totalGems);
                sink.line(barX, barY, length + 1, true);
                if (length > 1) {
                    sink.line(barX + 1, barY + 1, length - 1, true);
                    sink.line(barX + 1, barY - 1, length - 1, true);
                }
            } else {
                sink.text(t.maxed, 88, gemY + 1, -1, Kind::red);
                sink.text(t.maxed, 88, gemY, -1, Kind::white);
            }
            break;
        }
        case Screen::confirm: {
            const int top = 90;
            backdrop(sink, top);
            sink.text(confirmType == Confirm::quit ? t.confirmQuit : t.confirmReset,
                      80, 72 + wholePowan + 32, 0, Kind::borderBlack);
            const char* items[2] = {t.yes, t.no};
            for (int i = 0; i <= maxMenu; ++i)
                item(sink, items[i], i, 64 + i * 8, top + 20 + i * 10 + powany(i) + wholePowan2 + 48, -1);
            break;
        }
        case Screen::palette: {
            const int top = 90;
            backdrop(sink, top);
            sink.text(t.palette, 80, 72 + wholePowan, 0, Kind::borderBlack);
            const int x = 80 + powanx - 40;
            // shaderTexty = vy + 72 + 48 + wholePowan + powanx * 1.5 + 16.
            const int y = 136 + wholePowan + (3 * powanx) / 2;
            const unsigned unlocked = v.unlock.paletteUnlock;
            const auto name = [&](unsigned index) {
                unsigned n = decimal(buffer, index);
                buffer[n++] = '.'; buffer[n] = 0;
                append(buffer, n, index < v.paletteCount ? v.palettes[index].name : "");
                return buffer;
            };
            const unsigned shader = v.save->shader;
            name(shader);
            sink.text(buffer, x - 1, y + 1, -1, Kind::red);
            sink.text(buffer, x, y, -1, Kind::white);
            const auto wrapUp = [&](unsigned value) { return value > unlocked ? value - (unlocked + 1) : value; };
            const auto wrapDown = [&](int value) { return unsigned(value < 0 ? value + int(unlocked) + 1 : value); };
            sink.text(name(wrapUp(shader + 1)), x + 10, y + 12, -1, Kind::borderRed);
            if (unlocked > 2) sink.text(name(wrapUp(shader + 2)), x + 18, y + 20, -1, Kind::borderRed);
            sink.text(name(wrapDown(int(shader) - 1)), x - 10, y - 12, -1, Kind::borderRed);
            if (unlocked > 2) sink.text(name(wrapDown(int(shader) - 2)), x - 18, y - 20, -1, Kind::borderRed);
            for (int i = 0; i <= 160; i += 8)
                for (int k = 0; k < 4; ++k) {
                    sink.dither(7 - k, i, y - 20 - 2 * k);
                    sink.dither(7 - k, i, y + 20 + 2 * k);
                }
            ++runQuarters; // Draw advances runFrame by 0.25 before using it.
            // sprPlayerRunExg (8 frames) and sprGemM (4 frames) wrap.
            sink.sprite(27, (runQuarters / 4) % 8, 72, 96 + wholePowan);
            sink.sprite(134, (runQuarters / 4) % 4, 88, 98 + wholePowan);
            item(sink, t.back, 0, 80, top + 20 + 64 + powany(0) + wholePowan2 + 16, 0);
            break;
        }
        case Screen::stats: {
            const int top = 90;
            backdrop(sink, top);
            sink.text(t.stats, 80, 48 + wholePowan, 0, Kind::borderBlack);
            const int labelY = 64 + wholePowan + 8;
            sink.text(t.furthest, 12, labelY, -1, Kind::borderBlack);
            sink.text(t.gems, 12, labelY + 32, -1, Kind::borderBlack);
            sink.text(t.combo, 12, labelY + 64, -1, Kind::borderBlack);
            sink.text(t.time, 12, labelY + 96, -1, Kind::borderBlack);
            const int valueY = labelY + 10;
            reachedString(buffer, v.save->furthestReached);
            sink.text(buffer, 148, valueY, 1, Kind::borderBlack);
            decimal(buffer, v.save->bestRunGems);
            sink.text(buffer, 148, valueY + 32, 1, Kind::borderBlack);
            decimal(buffer, v.save->bestCombo);
            sink.text(buffer, 148, valueY + 64, 1, Kind::borderBlack);
            timeString(buffer, v.save->fastestGameHard, true);
            sink.text(buffer, 148, valueY + 96 - 10, 1, Kind::borderBlack);
            timeString(buffer, v.save->fastestGame, false);
            sink.text(buffer, 148, valueY + 96, 1, Kind::borderBlack);
            item(sink, t.back, 0, 16, 192 + 8 + powany(0) + wholePowan2, -1);
            break;
        }
        case Screen::options: {
            const int top = 90;
            backdrop(sink, top);
            sink.text(t.option, 80, 72 + wholePowan, 0, Kind::borderBlack);
            char rows[4][24];
            unsigned n = 0; rows[0][0] = 0; append(rows[0], n, t.back);
            n = 0; rows[1][0] = 0; append(rows[1], n, t.bgm); append(rows[1], n, ": ");
            append(rows[1], n, v.save->noBgm ? t.off : t.on);
            n = 0; rows[2][0] = 0; append(rows[2], n, t.timer); append(rows[2], n, ": ");
            append(rows[2], n, v.save->showTimer ? t.on : t.off);
            n = 0; rows[3][0] = 0; append(rows[3], n, t.volume); append(rows[3], n, ": ");
            n += decimal(rows[3] + n, v.save->masterGainTenths);
            for (int i = 0; i <= maxMenu; ++i)
                item(sink, rows[i], i, 16 + i * 8, top + 20 + i * 10 + powany(i) + wholePowan2, -1);
            break;
        }
        case Screen::closed: break;
        }
        return true;
    }
};
}
#endif
