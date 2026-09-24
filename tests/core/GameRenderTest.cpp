#include "core/Game.h"
#include "core/RoomPack.h"
#include "core/ObjectPack.h"
#include "core/BuilderPatternPack.h"
#include "core/DeathResultsBackdrop.h"
#include "core/DeathResultsRows.h"
#include "core/DeathResultsChoices.h"
#include "core/StateDigest.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

class RecordingRenderer final : public downwell::Renderer {
public:
    void clear(downwell::Color color) override {
        clearColor = color;
        ++clearCount;
    }
    void setClipRect(int x, int y, int width, int height) override {
        clipX = x;
        clipY = y;
        clipWidth = width;
        clipHeight = height;
        ++setClipCount;
    }
    void clearClipRect() override { ++clearClipCount; }
    void fillRect(int, int, int, int, downwell::Color) override {}
    void drawSpriteTiled(std::int32_t, std::uint32_t, int, int) override {}
    void drawSprite(std::int32_t, std::uint32_t, int, int) override {}
    void drawSpriteTransformed(std::int32_t sprite, std::uint32_t, float, float,
                               float, float, float) override {
        if (sprite == targetPlayer) ++playerDraws;
    }
    void drawText(std::int32_t, const char* text, float x, float y,
                  downwell::Color color,
                  downwell::TextAlign align) override {
        if (std::strcmp(text, "GAME OVER") == 0) {
            ++resultsTitleDraws;
            if (color.r == 255) {
                resultsTitleX = x;
                resultsTitleY = y;
            }
        }
        if (color.r == 255 &&
            ((color.g == 255 && color.b == 255) ||
             (color.g == 0 && color.b == 0)) &&
            align == downwell::TextAlign::center) {
            ++creditForegroundCount;
            if (std::strcmp(text, "JOONAS TURNER") == 0 && x == 102.0F)
                sawLeftCredit = true;
            if (std::strcmp(text, "EIRIK SUHRKE") == 0 && x == 278.0F)
                sawRightCredit = true;
        }
    }

    downwell::Color clearColor{};
    int clearCount = 0;
    int setClipCount = 0;
    int clearClipCount = 0;
    int clipX = 0;
    int clipY = 0;
    int clipWidth = 0;
    int clipHeight = 0;
    int creditForegroundCount = 0;
    bool sawLeftCredit = false;
    bool sawRightCredit = false;
    int targetPlayer = -1;
    unsigned playerDraws = 0;
    unsigned resultsTitleDraws = 0;
    float resultsTitleX = 0, resultsTitleY = 0;
};

} // namespace

int main() {
    struct ChoiceProbe {
        struct Call { unsigned index; int x, y; unsigned pen; };
        std::vector<Call> calls;
        void choice(unsigned index, int x, int y, unsigned pen) { calls.push_back({index,x,y,pen}); }
    } choices;
    downwell::DeathMenuSequence choiceMenu;
    downwell::drawDeathResultsChoices(choices, choiceMenu);
    require(choices.calls.empty(), "choices hidden before results sequence finishes");
    choiceMenu.sequenceOver = true;
    for (int cursor = 0; cursor < 3; ++cursor) {
        for (int bounce = 0; bounce <= 12; bounce += 4) {
            choices.calls.clear();
            choiceMenu.cursor = cursor;
            choiceMenu.cursorBounceTenths = bounce;
            downwell::drawDeathResultsChoices(choices, choiceMenu);
            require(choices.calls.size() == 4, "selected label has shadow plus foreground");
            unsigned call = 0;
            for (int i = 0; i < 3; ++i) {
                const int y = 190 + i * 10 - (cursor >= i ? 2 : 0);
                const auto& red = choices.calls[call++];
                require(red.index == static_cast<unsigned>(i) && red.pen == 2 &&
                    red.x == (16+i*8)*10 + (cursor == i ? bounce-10 : 0) &&
                    red.y == y + (cursor == i), "source left anchor and red layer");
                if (cursor == i) {
                    const auto& white = choices.calls[call++];
                    require(white.pen == 1 && white.x == (16+i*8)*10+bounce &&
                        white.y == y+1-bounce/10, "source fractional bounce and floor on white baseline");
                }
            }
        }
    }
    choices.calls.clear();
    choiceMenu.goalPassed = true;
    downwell::drawDeathResultsChoices(choices, choiceMenu);
    require(choices.calls.empty(), "unlock notice hides choices");
    downwell::RunStage stage;
    require(stage.area == 0 && stage.level == 0, "new run has source zero stage before confirmation");
    stage.advance();
    require(stage.area == 1 && stage.level == 1, "first confirmation advances to area one, level one");
    stage.advance(); stage.advance();
    require(stage.area == 1 && stage.level == 3, "three levels per area");
    stage.advance();
    require(stage.area == 2 && stage.level == 1, "fourth advance rolls area and resets level");
    stage.advance(4);
    require(stage.area == 4 && stage.level == 1, "explicit area override follows the ordinary increment");
    stage.advance(-1);
    require(stage.area == 4 && stage.level == 2, "nonpositive override does not replace normal progression");
    struct RowProbe {
        struct Call { int id; unsigned frame; int x, y; };
        std::vector<Call> calls;
        void asset(int id, unsigned frame, int x, int y) { calls.push_back({id, frame, x, y}); }
    } rows;
    downwell::DeathMenuSequence rowMenu;
    downwell::drawDeathResultsRows(rows, rowMenu, 84, 10, 3699, 25, stage);
    require(rows.calls.empty(), "results rows remain hidden until published");
    rowMenu.visible[1] = rowMenu.visible[2] = true;
    rowMenu.rowOffset[1] = 3; rowMenu.rowOffset[2] = 2;
    downwell::drawDeathResultsRows(rows, rowMenu, 84, 10, 3699, 25, stage);
    require(rows.calls.size() == 8 && rows.calls[0].id == 401 && rows.calls[0].frame == 1 &&
                rows.calls[0].x == 42 && rows.calls[0].y == 102 && rows.calls[1].frame == 0 &&
                rows.calls[2].id == 403 && rows.calls[2].frame == 8 && rows.calls[2].x == 72 &&
                rows.calls[2].y == 105 && rows.calls[3].frame == 4 && rows.calls[3].x == 78 &&
                rows.calls[4].frame == 4 && rows.calls[6].frame == 1 && rows.calls[7].frame == 0 &&
                rows.calls[7].y == 114,
            "source row icons, colon, digit order and separate numeric bounce");
    rows.calls.clear();
    rowMenu.visible[1] = rowMenu.visible[2] = false;
    rowMenu.visible[4] = true;
    downwell::drawDeathResultsRows(rows, rowMenu, 84, 10, 3699, 25, stage);
    const unsigned timeFrames[] = {6,0,6,4,11,0,1,11,0,1};
    const int timeX[] = {42,62,108,114,102,90,96,84,72,78};
    require(rows.calls.size() == 10, "time row has icon, colon and eight timer glyphs");
    for (unsigned i = 0; i < 10; ++i)
        require(rows.calls[i].frame == timeFrames[i] && rows.calls[i].x == timeX[i] &&
                    rows.calls[i].y == (i < 2 ? 140 : 144),
                "timer preserves source pair order, zero padding, separators and independent bounce");
    rows.calls.clear();
    rowMenu.visible[4] = false;
    rowMenu.visible[3] = true;
    rowMenu.rowOffset[3] = 2;
    downwell::drawDeathResultsRows(rows, rowMenu, 84, 10, 3699, 25, stage);
    require(rows.calls.size() == 4 && rows.calls[0].id == 401 && rows.calls[0].frame == 5 &&
                rows.calls[0].x == 42 && rows.calls[0].y == 126 &&
                rows.calls[1].frame == 0 && rows.calls[1].x == 62 && rows.calls[1].y == 126 &&
                rows.calls[2].id == 403 && rows.calls[2].frame == 2 && rows.calls[2].x == 72 &&
                rows.calls[2].y == 128 && rows.calls[3].frame == 5 && rows.calls[3].x == 78,
            "best-combo row uses source icon and separate digit bounce after fourteen-pixel gap");
    rows.calls.clear();
    downwell::drawDeathResultsRows(rows, rowMenu, 84, 10, 3699, 0, stage);
    require(rows.calls.size() == 3 && rows.calls[2].frame == 0,
            "a run without a completed combo displays zero, not a blank row");
    rows.calls.clear();
    rowMenu.visible[3] = false;
    rowMenu.visible[0] = true;
    downwell::drawDeathResultsRows(rows, rowMenu, 84, 10, 3699, 0, stage);
    require(rows.calls.size() == 5 && rows.calls[0].id == 401 && rows.calls[0].frame == 3 &&
                rows.calls[0].x == 42 && rows.calls[0].y == 92 && rows.calls[1].frame == 0 &&
                rows.calls[2].frame == 4 && rows.calls[2].x == 72 && rows.calls[2].y == 96 &&
                rows.calls[3].frame == 10 && rows.calls[3].x == 79 &&
                rows.calls[4].frame == 2 && rows.calls[4].x == 84,
            "area-level row uses source frame IDs and asymmetric separator spacing");
    rows.calls.clear();
    downwell::drawResultNumber(rows, 0, 72, 98);
    require(rows.calls.size() == 1 && rows.calls[0].frame == 0, "zero is one visible digit");
    rows.calls.clear();
    downwell::drawResultNumber(rows, 0xffffffffu, 72, 98);
    const unsigned maximumDigits[] = {4,2,9,4,9,6,7,2,9,5};
    require(rows.calls.size() == 10, "maximum run counter does not overflow digit divisor");
    for (unsigned i = 0; i < 10; ++i)
        require(rows.calls[i].frame == maximumDigits[i] && rows.calls[i].x == 72 + static_cast<int>(i)*6,
                "all decimal digits use the source six-pixel spacing");
    struct BackdropProbe {
        unsigned tiles = 0, titles = 0;
        bool panel = false;
        void sprite(int frame, int x, int y) {
            require(frame >= 0 && frame <= 7 && x >= -7 && x <= 160 && y >= -7 && y < 284,
                    "results backdrop stays in source tiling range");
            ++tiles;
        }
        void blackRect(int x, int y, int width, int height) {
            panel = x == 0 && y == 80 && width == 161 && height == 129;
        }
        void title(int x, int y, bool black) {
            require(black || (x == 80 && y == 68), "source title position with entry bounce");
            ++titles;
        }
    } backdrop;
    downwell::drawDeathResultsBackdrop(backdrop, 4, 160, 0);
    require(backdrop.tiles == 1056 && backdrop.titles == 5 && backdrop.panel,
            "source tiled shade, two gradients, inclusive panel and four-way title outline");
    downwell::Game game;
    RecordingRenderer renderer;
    game.render(renderer);

    require(renderer.clearCount == 1 && renderer.clearColor.r == 0 &&
                renderer.clearColor.g == 0 && renderer.clearColor.b == 0 &&
                renderer.clearColor.a == 255,
            "the full 4:3 canvas is cleared to opaque black");
    require(renderer.setClipCount == 1 &&
                renderer.clipX == downwell::Game::playfieldX &&
                renderer.clipY == 0 &&
                renderer.clipWidth == downwell::StartupRuntime::surfaceWidth &&
                renderer.clipHeight == downwell::Game::logicalHeight,
            "game rendering is clipped to the centred 160x284 well");
    require(renderer.clearClipCount == 1,
            "the game clip is released before the HUD layer");

    downwell::Input input;
    input.fire = true;
    game.update(input);
    input.fire = false;
    game.update(input);
    input.fire = true;
    game.update(input);
    require(game.startup().showSplash() == 3,
            "two fire edges advance the reference to credits");
    renderer.clearClipCount = 0;
    game.render(renderer);
    require(renderer.clearClipCount == 1 &&
                renderer.creditForegroundCount == 6 &&
                renderer.sawLeftCredit && renderer.sawRightCredit,
            "credits release the playfield clip and use both 4:3 side areas");

    const auto read = [](const char* path) {
        std::ifstream file(path, std::ios::binary);
        return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
    };
    const auto roomBytes = read("generated/downwell-rooms.dwrm");
    const auto objectBytes = read("generated/downwell-objects.dwob");
    const auto builderBytes = read("generated/downwell-builder-patterns.dwbp");
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    const downwell::BuilderPatternPack builders(builderBytes.data(), builderBytes.size());
    require(rooms.valid() && objects.valid() && builders.valid(), "damage-render test loads real room/object/builder data");
    downwell::Game damagedGame(&rooms, &objects, &builders);
    unsigned mainTicks = 0, hidden = 0, flashingVisible = 0;
    bool damaged = false, recovered = false;
    for (unsigned tick = 0; tick < 6000 && !recovered; ++tick) {
        const auto& state = damagedGame.startup();
        input = {};
        if (state.inMenu()) input.right = true;
        else if (state.inPlayMenu() && state.playMenuYOffset() == 0) input.fire = true;
        else if (state.inMainRoom()) {
            input.right = (mainTicks / 90) % 2 == 0;
            input.left = !input.right;
            input.fire = mainTicks % 10 < 5;
        }
        damagedGame.update(input);
        if (!state.inMainRoom()) continue;
        ++mainTicks;
        const int immunity = state.mainPlayerInvulnerability();
        if (immunity > 0) damaged = true;
        if (!damaged) continue;
        renderer.targetPlayer = state.mainPlayerSpriteId();
        renderer.playerDraws = 0;
        damagedGame.render(renderer);
        require(renderer.playerDraws == (immunity % 2 == 0 ? 1u : 0u),
                "actual Game render suppresses odd damage phases and draws even phases once");
        if (immunity == 0) recovered = true;
        else if (immunity % 2) ++hidden;
        else ++flashingVisible;
    }
    require(hidden > 0 && flashingVisible > 0 && recovered,
            "live render tape covers hidden, visible and fully recovered player states");
    // Renderer fixture only: natural death entry is exercised by the long
    // paired simulation replay. This tape deliberately ends after recovery.
    auto& resultsFixture = const_cast<downwell::DeathResults&>(damagedGame.startup().deathResults());
    resultsFixture.open(true, 0, 0);
    for (int offset = 4; offset >= 0; --offset) {
        resultsFixture.menu.titleOffset = offset;
        downwell::StateDigest before, after;
        resultsFixture.appendState(before);
        renderer.resultsTitleDraws = 0;
        damagedGame.render(renderer);
        resultsFixture.appendState(after);
        require(renderer.resultsTitleDraws == 5 && renderer.resultsTitleX == 190 &&
                    renderer.resultsTitleY == 64 + offset,
                "Game renderer submits results title and four-way outline at source anchors");
        require(before.value() == after.value(), "drawing results never advances simulation");
    }
    std::puts("game render tests passed");
    return 0;
}
