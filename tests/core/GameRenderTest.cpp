#include "core/Game.h"
#include "core/GameplayContactProfileInput.h"
#include "core/RoomPack.h"
#include "core/ObjectPack.h"
#include "core/BuilderPatternPack.h"
#include "core/DeathResultsBackdrop.h"
#include "core/DeathResultsRows.h"
#include "core/DeathResultsChoices.h"
#include "core/RunSession.h"
#include "core/StateDigest.h"
#include "core/StoredWorldCollision.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
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
    void fillRect(int x, int y, int w, int h, downwell::Color color) override {
        if(recordRoomBackground&&x==110&&y==0&&w==160&&h==284&&
           !color.r&&!color.g&&!color.b&&color.a==255)
            spriteOrder.push_back(-1000001);
        if(recordWallTile&&x==134&&y==35&&w==16&&h==16&&color.r==0&&color.g==0&&color.b==0&&color.a==255)
            spriteOrder.push_back(-739);
        if (w == 1 && h == 1 && color.r == 255 && !color.g && !color.b) ++redPixels;
        if (x == 110 && y == 0 && w == 160 && h == 284 &&
            color.r == 255 && color.g == 255 && color.b == 255 && color.a == 255)
            ++deathFlashCount;
    }
    void drawSpriteTiled(std::int32_t, std::uint32_t, int, int) override {}
    void drawSprite(std::int32_t id, std::uint32_t frame, int x, int y) override {
        spriteOrder.push_back(id);
        if(id==96) flashDraws.push_back({id,frame,float(x),float(y),1,1,0});
        if (id == 133 || id == 134) ++gemDraws;
        require(id != 458 && id != 459, "projectiles must use their firing angle, not unrotated artwork");
        if (id == 489 && x == 110 && y == 57) ++shopPanels;
        if (id == 128) {
            ++exitDitherDraws; exitDitherX = x; exitDitherY = y;
            require(clipX == 110 && clipY == 0 && clipWidth == 160 && clipHeight == 284,
                "exit dither is clipped to the room surface, not the side HUD");
        }
    }
    struct Transform { int sprite; unsigned frame; float x, y, sx, sy, angle; };
    bool recordTransforms = false;
    std::vector<Transform> transforms;
    std::vector<Transform> flashDraws;
    std::vector<int> spriteOrder;
    struct DamageText {float x,y;downwell::Color color;};
    std::vector<DamageText> damageTexts;
    void drawSpriteTransformed(std::int32_t sprite, std::uint32_t frame, float x, float y,
                               float sx, float sy, float angle) override {
        spriteOrder.push_back(sprite);
        if (recordTransforms) transforms.push_back({sprite, frame, x, y, sx, sy, angle});
        if (sprite == 458 || sprite == 459) {
            require(sx == 1 && sy == 1 && angle >= 267 && angle <= 273,
                    "machine-gun artwork follows the source spread range");
            bulletAngles.push_back(angle);
            spreadBulletDrawn |= angle != 270;
            ++bulletDraws;
        }
        if (sprite == targetPlayer) ++playerDraws;
        if (sprite == 127) ++triggerDraws;
    }
    unsigned bulletDraws = 0;
    bool recordWallTile=false;
    bool recordRoomBackground=false;
    unsigned redPixels = 0;
    std::vector<float> bulletAngles;
    bool spreadBulletDrawn = false;
    void drawText(std::int32_t, const char* text, float x, float y,
                  downwell::Color color,
                  downwell::TextAlign align) override {
        if(std::strcmp(text,"-12")==0){
            require(align==downwell::TextAlign::center,"damage text is centered");
            damageTexts.push_back({x,y,color});spriteOrder.push_back(-1001);
        }
        if (std::strstr(text, "CLEARED")) ++clearTitleDraws;
        if (std::strcmp(text, "CHOOSE 1 UPGRADE") == 0) ++clearChoiceDraws;
        if (x == 190 && y == 64 && color.r == 255 && color.g == 255 &&
            align == downwell::TextAlign::center) ++shopNameCenters;
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
    unsigned deathFlashCount = 0;
    unsigned gemDraws = 0;
    unsigned effectWords = 0;
    std::vector<int> effectPixels;
    void drawCoverageWord(int x,int y,std::uint16_t bits,downwell::Color color) override {
        require(x>=110 && x<270 && y>=0 && y<284 && bits,"controller effect words stay in the shaft");
        spriteOrder.push_back(-1000);
        ++effectWords;
        if(!effectPixels.empty())for(unsigned bit=0;bit<16;++bit)
            if(bits&(0x8000u>>bit))effectPixels[y*160+x-110+bit]=color.r==0&&color.g==0&&color.b==0?0:1;
    }
    unsigned shopPanels = 0, shopNameCenters = 0;
    unsigned clearTitleDraws = 0, clearChoiceDraws = 0;
    unsigned triggerDraws = 0;
    unsigned exitDitherDraws = 0;
    int exitDitherX = 0, exitDitherY = 0;
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
    downwell::RunSession session;
    downwell::MainCameraMotion exitCamera;
    downwell::GameMakerRandom exitRandom(42), exitOracle(42);
    session.createLevelExit(480, 4000, exitRandom, exitCamera);
    require(session.levelExitPresent && session.levelExit.x == 240 &&
        session.levelExit.y == 4000 && exitCamera.roomEnd == 3824 &&
        session.goalField.x == 240 && session.goalField.y == 4032 &&
        session.goalField.radius.equal(downwell::GoalTimeField::nextRadius(exitOracle)),
        "exit Create applies inherited camera boundary then creates the random-radius field");
    session.levelExit.active = true;
    session.levelExit.alarm0 = 5;
    session.goalStopped = session.exitControlsLocked = true;
    session.exitDitherRequested = session.levelResultRequested = true;
    session.gems.observeDeathMovement(true, 123);
    session.clock.ticks = 999;
    session.combo.state.count = 30;
    session.combo.state.best = 40;
    session.combo.state.record = 50;
    session.combo.healthReserve.maximum = 8;
    session.combo.healthReserve.pieces = 3;
    session.combo.reward.combo = 30;
    session.combo.completion.saveRecord = true;
    session.stage = {3, 2};
    session.results.open(true, 123, 123);
    session.results.previous.fire = true;
    session.consumables.pending[0] = 2;
    session.upgrades.levels[6] = 1;
    session.upgrades.order[1] = 6;
    session.upgrades.count = 1;
    const auto saved = session.records();
    session.reset(saved);
    require(!session.levelExitPresent && !session.levelExit.active &&
        session.levelExit.alarm0 == -1 && session.levelExit.alarm1 == -1 &&
        session.goalField.radius.mantissa == 0 && !session.goalStopped &&
        !session.exitControlsLocked && !session.exitDitherRequested && !session.levelResultRequested,
        "new run discards exit alarms and goal-field state");
    require(session.gems.lifetime == 123 && session.gems.record == 123 &&
        session.combo.state.record == 50, "new run reloads persistent record snapshot");
    require(!session.gems.recorded && session.gems.savedRunGems == 0 &&
        session.clock.ticks == 0 && session.combo.state.count == 0 &&
        session.combo.state.best == 0 && session.stage.area == 0 && session.stage.level == 0,
        "new run clears per-run statistics and stage");
    require(session.combo.healthReserve.maximum == 4 && session.combo.healthReserve.pieces == 0 &&
        session.combo.reward.combo == 0 && !session.combo.completion.saveRecord &&
        !session.results.active && !session.results.previous.fire,
        "new run clears upgrades, pending effects and results input history");
    require(session.consumables.pending[0] == 0, "new run discards unconsumed purchases");
    require(session.upgrades.count == 0 && session.upgrades.levels[6] == 0 &&
        session.upgrades.order[1] == 0, "new run clears upgrade ownership and acquisition order");
    require(session.gems.observeDeathMovement(true, 7) &&
        !session.gems.observeDeathMovement(true, 7) && session.gems.lifetime == 130,
        "next run death adds its own gems exactly once");
    session.reset();
    require(session.records().lifetimeGems == 0 && session.records().bestCombo == 0,
        "cold boot without saved data uses source defaults");
    session.clock.ticks = 42;
    require(session.restart(downwell::DeathMenuSequence::Action::None, saved) ==
        downwell::ResultsDestination::none && session.clock.ticks == 42,
        "absent results action does not reset a live run");
    require(session.restart(downwell::DeathMenuSequence::Action::Retry, saved) ==
        downwell::ResultsDestination::gameplay && session.stage.area == 1 && session.stage.level == 1 &&
        session.gems.lifetime == 123, "retry resets then executes scrNextLevel(1)");
    require(session.restart(downwell::DeathMenuSequence::Action::StyleSelect, saved) ==
        downwell::ResultsDestination::styleSelect && session.stage.area == 0 && session.stage.level == 0,
        "style selection stays at zero stage until confirmation");
    require(session.restart(downwell::DeathMenuSequence::Action::Surface, saved) ==
        downwell::ResultsDestination::surface && session.stage.area == 0,
        "surface uses room 20 without advancing a level");
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
    // These large independent fixtures must not share one O0 stack frame.
    auto gameStorage = std::make_unique<downwell::Game>();
    auto& game = *gameStorage;
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
    {
        const auto cb=read("generated/downwell-sprites.dwsc"),mb=read("generated/downwell-sprite-masks.dwsm");
        downwell::SpriteCollisionPack sprites(cb.data(),cb.size());downwell::SpriteMaskPack masks(mb.data(),mb.size());
        auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,sprites,masks);
        auto scene=std::make_unique<downwell::Game>(&rooms,&objects,&builders,nullptr,owner->binding());
        require(owner->attach(scene->startup()),"muzzle drawing fixture attaches production collision owner");
        auto& world=const_cast<downwell::StartupRuntime&>(scene->startup());world.skipToGameForTesting();
        auto n=[](int v){return downwell::MotionBinary64::fromInteger(v);};
        auto frames=[&](int id,unsigned& count){downwell::PackedSpriteCollision sprite;
            if(!sprites.sprite(id,sprite))return false;count=sprite.frameCount;return true;};
        std::uint32_t a=0,b=0;
        require(world.spawnScriptEffect(n(240),n(120),603,{0x3fe00000u,0},n(270),{},frames,a)&&
            world.spawnScriptEffect(n(244),n(124),602,{0x3fe00000u,0},{},{},frames,b)&&b>a,
            "registered muzzle drawing fixture uses ordinary owned script creation");
        std::vector<downwell::InstanceHandle> muzzleLifetimes;
        for(unsigned i=0;i<world.activeInstanceCount();++i){
            const auto& r=world.instanceAt(i);if(r.objectId!=219)continue;
            const auto key=world.instanceHandleAt(i);
            const auto* layer=world.sourceRoomLayers().lifetimes().resolve(key);
            require(layer&&layer->sourceId==r.instanceId&&layer->depth.attached,
                "built-in script registry inherits layer observer before its first birth");
            muzzleLifetimes.push_back(key);
        }
        require(muzzleLifetimes.size()==2,"both owned muzzle lifetimes have room elements");
        const auto digest=world.stateDigest();RecordingRenderer draw;draw.recordTransforms=true;scene->render(draw);
        std::vector<RecordingRenderer::Transform> muzzles;
        for(const auto& item:draw.transforms)if(item.sprite==602||item.sprite==603)muzzles.push_back(item);
        require(scene->renderingSucceeded()&&world.stateDigest()==digest&&muzzles.size()==2&&
            muzzles[0].sprite==602&&muzzles[1].sprite==603&&muzzles[1].angle==270&&
            muzzles[0].x==110+244-world.mainCameraRenderViewX()&&
            muzzles[0].y==124-world.mainCameraRenderViewY(),
            "SDL draws owned muzzle effects newest-first at published room coordinates without mutation");
        auto registration=world.debrisRegistration<2>();downwell::DebrisEffects<2> debris;
        unsigned first=0,last=0;
        require(world.spawnDebris(registration,debris,n(240),n(120),1,first)&&
            world.spawnDebris(registration,debris,n(244),n(124),4,last),"register debris for live SDL drawing");
        debris.get(1)->imageIndex.bits=0x40d80000u;debris.get(2)->imageIndex.bits=0x3f800000u;
        RecordingRenderer particles;particles.recordTransforms=true;scene->render(particles);
        unsigned particleCount=0;
        for(const auto& item:particles.transforms)if(item.sprite==91||item.sprite==115){
            require(item.sprite==(particleCount?115:91)&&item.frame==(particleCount?2u:1u)&&
                item.x==110+(particleCount?240:244)-world.mainCameraRenderViewX()&&
                item.y==(particleCount?120:124)-world.mainCameraRenderViewY(),"SDL debris follows room Draw order and frame selection");
            ++particleCount;
        }
        require(scene->renderingSucceeded()&&particleCount==2,"SDL direct fallback draws both debris instances");
        debris.get(2)->flash=1;RecordingRenderer hidden;scene->render(hidden);
        require(std::find(hidden.spriteOrder.begin(),hidden.spriteOrder.end(),91)==hidden.spriteOrder.end(),"flashing debris omitted");
        require(debris.clear(registration),"retire registered rendering fixtures before updating");
        scene->update({});scene->update({});RecordingRenderer retired;scene->render(retired);
        require(scene->simulationSucceeded()&&scene->renderingSucceeded()&&
            std::find(retired.spriteOrder.begin(),retired.spriteOrder.end(),602)==retired.spriteOrder.end()&&
            std::find(retired.spriteOrder.begin(),retired.spriteOrder.end(),603)==retired.spriteOrder.end(),
            "ordinary Animation End and cleanup remove muzzles from SDL drawing");
        for(auto key:muzzleLifetimes)
            require(!world.resolveInstance(key)&&!world.sourceRoomLayers().lifetimes().resolve(key),
                "Animation End retires owned script storage and layer identity together");
        require(world.spawnScriptEffect(n(248),n(128),101,{0x3fe00000u,0},{},{},frames,a),
            "wall impact uses the shared script effect owner");
        for(unsigned tick=0;tick<16;++tick){
            RecordingRenderer frame;frame.recordTransforms=true;scene->render(frame);unsigned seen=0;
            for(const auto& item:frame.transforms)if(item.sprite==101){
                require(item.frame==tick/2&&item.angle==0&&
                    item.x==110+248-world.mainCameraRenderViewX()&&item.y==128-world.mainCameraRenderViewY(),
                    "SDL wall impact retains source frame, angle and room anchor");++seen;
            }
            require(seen==1&&scene->renderingSucceeded(),"one wall impact is drawn before Animation End");
            scene->update({});require(scene->simulationSucceeded(),"wall impact ordinary animation update succeeds");
        }
        RecordingRenderer ended;scene->render(ended);
        require(std::find(ended.spriteOrder.begin(),ended.spriteOrder.end(),101)==ended.spriteOrder.end(),
            "wall impact retires after sixteen source animation updates");
        auto blood=world.bloodRegistration<2>();downwell::InstanceHandle firstBlood,lastBlood;
        downwell::MotionBinary64 bx=n(248),by=n(128);
        require(bx.add({0x3fd00000u,0})&&by.add({0x3fe00000u,0}),"fractional blood Draw anchor");
        require(world.spawnBlood(blood,bx,by,{0x402b8000u,0},[](auto){return true;},firstBlood)&&
            world.spawnBlood(blood,n(252),n(132),{0x4064c800u,0},[](auto){return true;},lastBlood),
            "registered blood Draw fixtures");
        for(unsigned tick=0;tick<18;++tick){
            RecordingRenderer frame;frame.recordTransforms=true;scene->render(frame);unsigned seen=0;
            for(const auto& item:frame.transforms)if(item.sprite==84){
                require(seen<2&&item.frame==tick/2&&item.sx==1&&item.sy==1,
                    "blood draws every source animation frame once with unit scales");
                require(item.angle==(seen?13.75f:166.25f)&&
                    item.x==110+(seen?248.25f:252.0f)-world.mainCameraRenderViewX()&&
                    item.y==(seen?128.5f:132.0f)-world.mainCameraRenderViewY(),
                    "blood retains fractional anchors, original angles and equal-depth layer order");
                ++seen;
            }
            require(seen==2&&scene->renderingSucceeded(),"registered blood default Draw is visible");
            require(blood.advanceAnimation(firstBlood)&&blood.advanceAnimation(lastBlood),"advance blood Draw frames");
        }
        RecordingRenderer expiredBlood;scene->render(expiredBlood);
        require(std::find(expiredBlood.spriteOrder.begin(),expiredBlood.spriteOrder.end(),84)==expiredBlood.spriteOrder.end()&&blood.clear(),
            "marked blood stops drawing before lifetime cleanup");
        auto skeletons=world.skeletonRegistration<1>();downwell::InstanceHandle skeleton;
        require(world.spawnSkeleton(skeletons,248,128,skeleton),"register skeleton for source inherited Draw");
        for(int sprite:{208,207,205})for(int facing:{-1,1}){
            const unsigned count=downwell::SkeletonAnimation::frames(sprite);
            for(unsigned index=0;index<count;++index){
                downwell::RunnerImageValue image;require(image.assignInteger(index),"skeleton frame value");
                require(skeletons.dispatchPublished(skeleton,[&](auto&,auto&,auto& publish,bool&){
                    return publish.sprite(sprite)&&publish.imageIndex(image)&&publish.facing(facing);
                }),"publish skeleton Draw fields");
                RecordingRenderer frame;frame.recordTransforms=true;scene->render(frame);unsigned seen=0;
                for(const auto& item:frame.transforms)if(item.sprite==sprite){
                    require(item.frame==index&&item.sx==facing&&item.sy==1&&item.angle==0&&
                        item.x==110+256-world.mainCameraRenderViewX()&&item.y==128-world.mainCameraRenderViewY(),
                        "SDL skeleton uses published snapped position, frame and facing");++seen;
                }
                require(seen==1&&scene->renderingSucceeded(),"one source skeleton sprite per Draw");
            }
        }
        require(skeletons.mark(skeleton)&&skeletons.clear(),"retire skeleton Draw fixture");
        auto bones=world.boneRegistration<2>();downwell::InstanceHandle firstBone,lastBone;
        require(world.spawnBone(bones,240,120,firstBone)&&world.spawnBone(bones,244,124,lastBone),
            "register both bones through real source IDs and layer elements");
        downwell::RunnerImageValue fractionalX{0x43744000u},fractionalY{0x42f90000u}; // 244.25,124.5
        require(bones.dispatch(lastBone,[&](auto&,auto& publish,bool&){return publish.position(fractionalX,fractionalY);}),
            "publish fractional collision coordinates without changing private bone motion");
        for(unsigned tick=0;tick<4;++tick){
            const auto before=world.stateDigest();const auto draws=world.randomDrawCount();
            RecordingRenderer frame;frame.recordTransforms=true;scene->render(frame);unsigned count=0;
            for(const auto& item:frame.transforms)if(item.sprite==460){
                require(item.frame==tick&&item.sx==1&&item.sy==1&&item.angle==0&&
                    item.x==110+(count?240.0F:244.25F)-world.mainCameraRenderViewX()&&
                    item.y==(count?120.0F:124.5F)-world.mainCameraRenderViewY(),
                    "SDL bone default drawing uses sequence frame, published coordinates and newest-first layer order");
                ++count;
            }
            require(count==2&&scene->renderingSucceeded()&&before==world.stateDigest()&&draws==world.randomDrawCount(),
                "bone render is read-only and preserves RNG");
            require(bones.advanceAnimation(firstBone)&&bones.advanceAnimation(lastBone),"advance registered bone artwork");
        }
        require(bones.mark(lastBone),"mark bone for deferred retirement");
        RecordingRenderer marked;marked.recordTransforms=true;scene->render(marked);unsigned count=0;
        for(const auto& item:marked.transforms)if(item.sprite==460)++count;
        require(scene->renderingSucceeded()&&count==1&&bones.clear(),"destroyed bone cannot reappear before storage cleanup");
    }
    {
        // A copied fixture hides only rmMain's fixed player layer. Source
        // layer visibility must affect the live actor projection; a legacy
        // room-depth traversal would still visit the player.
        auto hiddenBytes=roomBytes;
        auto raisedBytes=roomBytes;
        downwell::PackedRoom room;
        require(rooms.findRoom(downwell::StartupRuntime::mainRoomId,room),"main room exists");
        bool changed=false;
        for(unsigned i=0;i<room.layerCount;++i){
            downwell::PackedRoomLayer layer;require(rooms.roomLayer(room,i,layer),"main layer exists");
            if(layer.depth!=-50000)continue;
            require(layer.visible()&&layer.membershipCount==1,"original player fixed layer fixture");
            std::uint32_t id;require(rooms.membership(layer.firstMembership,id)&&id==100855,
                "fixture hides the serialized player layer, not a synthetic actor");
            const auto offset=downwell::RoomPack::headerSize+rooms.roomCount()*downwell::RoomPack::roomRecordSize+
                rooms.instanceCount()*downwell::RoomPack::instanceRecordSize+
                (room.firstLayer+i)*downwell::RoomPack::layerRecordSize+8;
            hiddenBytes[offset]&=0xfe;changed=true;
            // A visible player behind depth-zero actors exercises the actual
            // painter order, not merely whether the traversal visits it.
            raisedBytes[offset-4]=1;
            raisedBytes[offset-3]=raisedBytes[offset-2]=raisedBytes[offset-1]=0;
        }
        downwell::RoomPack hiddenRooms(hiddenBytes.data(),hiddenBytes.size());
        require(changed&&hiddenRooms.valid(),"hidden-layer fixture retains valid resource structure");
        auto normal=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::desktop,&rooms,&objects,2,&builders);
        auto hidden=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::desktop,&hiddenRooms,&objects,2,&builders);
        normal->skipToGameForTesting();hidden->skipToGameForTesting();
        require(normal->stateDigest()==hidden->stateDigest(),"layer visibility does not change gameplay or RNG");
        std::vector<unsigned> expected,actual;
        bool playerSeen=false;
        require(normal->visitDrawChecked([&](auto,const auto& r){
            if(r.instanceId==100855)playerSeen=true;else expected.push_back(r.instanceId);return true;
        })&&hidden->visitDrawChecked([&](auto,const auto& r){actual.push_back(r.instanceId);return true;}),
            "production actor projection traverses both layer fixtures");
        require(playerSeen&&!expected.empty()&&actual==expected,
            "live Draw honors fixed-layer visibility while retaining dynamic actors");
        unsigned calls=0;
        require(!normal->visitDrawChecked([&](auto,const auto&){++calls;return false;})&&calls==1,
            "live source-layer projection stops immediately on renderer failure");
        unsigned backgrounds=0,instances=0;
        require(normal->visitSourceDrawChecked([&](auto,const auto&,int,auto){++instances;return true;},
            [&](const auto& bg,int){require(!instances&&bg.spriteId==-1,"source background precedes main instances");
                ++backgrounds;return true;})&&backgrounds==1&&instances==expected.size()+1,
            "full source traversal retains the background omitted by actor-only projection");
        downwell::InstanceRecord* defaultSprite=nullptr;
        for(unsigned i=0;i<normal->activeInstanceCount();++i){
            const auto& r=normal->instanceAt(i);downwell::PackedObject object;downwell::PackedObjectEvent event;
            if(r.active&&!r.destroyRequested&&r.spriteId>=0&&objects.findObject(r.objectId,object)&&
               object.visible()&&object.spriteId>=0&&!objects.findEvent(r.objectId,8,0,event)){
                defaultSprite=const_cast<downwell::InstanceRecord*>(&r);break;
            }
        }
        require(defaultSprite,"main room has an ordinary default-sprite Draw instance");
        const auto savedSprite=defaultSprite->spriteId;
        defaultSprite->spriteId=-1; // Draw-only fixture, no simulation between writes.
        bool seen=false;
        require(normal->visitDrawChecked([&](auto,const auto& r){seen|=r.instanceId==defaultSprite->instanceId;return true;})&&!seen,
            "sprite_index minus one does not fall back to the object resource sprite");
        defaultSprite->spriteId=savedSprite;
        downwell::RoomPack raisedRooms(raisedBytes.data(),raisedBytes.size());
        require(raisedRooms.valid(),"raised-layer fixture remains structurally valid");
        const auto cb=read("generated/downwell-sprites.dwsc"),mb=read("generated/downwell-sprite-masks.dwsm");
        downwell::SpriteCollisionPack sprites(cb.data(),cb.size());downwell::SpriteMaskPack masks(mb.data(),mb.size());
        const downwell::RoomPack* packs[]={&rooms,&hiddenRooms,&raisedRooms};
        for(const auto* pack:packs){
            auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,sprites,masks);
            auto scene=std::make_unique<downwell::Game>(pack,&objects,&builders,nullptr,owner->binding());
            require(owner->attach(scene->startup()),"player layer render owner attaches");
            auto& world=const_cast<downwell::StartupRuntime&>(scene->startup());world.skipToGameForTesting();
            auto& inventory=const_cast<downwell::UpgradeInventory&>(world.upgrades());
            inventory.levels[12]=1; // Draw-only Laser Sight fixture.
            auto frames=[&](int id,unsigned& count){downwell::PackedSpriteCollision s;
                if(!sprites.sprite(id,s))return false;count=s.frameCount;return true;};
            std::uint32_t effect=0;
            require(world.spawnScriptEffect(downwell::MotionBinary64::fromInteger(240),
                downwell::MotionBinary64::fromInteger(120),101,{0x3fe00000u,0},{},{},frames,effect),
                "layer order fixture creates ordinary depth-zero impact");
            const auto rng=world.randomDrawCount();
            RecordingRenderer frame;frame.targetPlayer=world.mainPlayerSpriteId();scene->render(frame);
            require(scene->renderingSucceeded()&&world.randomDrawCount()==rng,"layer-aware player rendering consumes no RNG");
            const auto playerAt=std::find(frame.spriteOrder.begin(),frame.spriteOrder.end(),frame.targetPlayer);
            const auto effectAt=std::find(frame.spriteOrder.begin(),frame.spriteOrder.end(),101);
            require(effectAt!=frame.spriteOrder.end(),"depth-zero effect remains visible");
            if(pack==&hiddenRooms)
                require(frame.playerDraws==0&&frame.redPixels==0,"hidden player layer suppresses both character and Laser Sight");
            else{
                require(frame.playerDraws==1&&frame.redPixels>0,"visible player layer draws character and Laser Sight once");
                require((playerAt<effectAt)==(pack==&raisedRooms),"player painter position follows source layer depth");
            }
        }
    }
    {
        auto game=std::make_unique<downwell::Game>(&rooms,&objects,&builders);
        auto& world=const_cast<downwell::StartupRuntime&>(game->startup());world.skipToGameForTesting();
        unsigned index=0;while(index<world.activeInstanceCount()&&world.instanceAt(index).objectId!=60)++index;
        require(index<world.activeInstanceCount(),"wall draw fixture has a wall");
        const auto key=world.instanceHandleAt(index);
        struct Ops {
            downwell::StartupRuntime& world;downwell::InstanceHandle key;
            bool place(int,int,int,bool& hit){hit=false;return true;}
            bool point(int,int,int,bool& hit){hit=false;return true;}
            bool image(int frame){return world.publishWallImage(key,frame);}
            bool tile(int,int,int,int,int,int,int,int){return false;}
            bool destroy(){return false;}bool deactivate(){return false;}bool outOfView(){return false;}
            bool create(int,int,int){return false;}
        } ops{world,key};
        require(world.wallEndStepEvent(key,0,ops),"publish selected wall frame for draw fixture");
        const auto* wall=world.resolveInstance(key);require(wall&&wall->imageIndex==14,"area-zero event selects frame 14");
        RecordingRenderer frame;frame.recordTransforms=true;game->render(frame);
        unsigned matches=0;
        for(const auto& draw:frame.transforms)if(draw.sprite==wall->spriteId&&
            draw.x==110+wall->x-world.mainCameraRenderViewX()&&draw.y==wall->y-world.mainCameraRenderViewY()){
            require(draw.frame==14,"renderer uses event-selected wall frame instead of neighbor reconstruction");++matches;
        }
        require(game->renderingSucceeded()&&matches==1,"event-selected wall reaches SDL renderer once");
    }
    {
        const auto collisionBytes=read("generated/downwell-sprites.dwsc"),maskBytes=read("generated/downwell-sprite-masks.dwsm");
        const downwell::SpriteCollisionPack collisions(collisionBytes.data(),collisionBytes.size());
        const downwell::SpriteMaskPack masks(maskBytes.data(),maskBytes.size());
        auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,collisions,masks);
        require(owner->valid(),"production boot loads collision resources");
        auto bound=std::make_unique<downwell::Game>(&rooms,&objects,&builders,nullptr,owner->binding());
        auto plain=std::make_unique<downwell::Game>(&rooms,&objects,&builders);
        require(owner->attach(bound->startup()),"production Game attaches before its first update");
        unsigned gameplay=0,splashStages=0;bool garden=false,style=false,gardenDrawChecked=false;
        for(unsigned frame=0;frame<4000&&gameplay<120;++frame){
            const auto& state=bound->startup();
            require(state.showSplash()>=0&&state.showSplash()<=6,"boot splash state is in source range");
            splashStages|=1u<<state.showSplash();
            downwell::Input control;
            if(state.inMenu()){garden=true;control.right=true;}
            else if(state.inPlayMenu()){style=true;control.fire=state.playMenuYOffset()==0;}
            bound->update(control);plain->update(control);
            require(bound->simulationSucceeded()&&plain->simulationSucceeded()&&
                bound->startup().stateDigest()==plain->startup().stateDigest(),"production owner preserves boot-to-game state and RNG");
            RecordingRenderer a,b;
            a.recordRoomBackground=b.recordRoomBackground=state.inMenu();
            bound->render(a);plain->render(b);
            require(bound->renderingSucceeded()&&plain->renderingSucceeded()&&a.spriteOrder==b.spriteOrder,
                "production owner preserves boot-to-game sprite painter order");
            if(state.inMenu()&&!gardenDrawChecked){
                // Source rmMenu has serialized visible instances at entry.
                // Reverse layers, then reverse serialized membership (the
                // runner reserves each element at the head). Do not derive
                // the expectation from either runtime traversal implementation.
                std::vector<const downwell::InstanceRecord*> order;
                std::vector<std::uint32_t> serialized;
                downwell::PackedRoom room;
                require(rooms.findRoom(state.roomId(),room),"garden room resource exists");
                unsigned backgrounds=0;
                for(unsigned i=room.layerCount;i>0;--i){
                    downwell::PackedRoomLayer layer;
                    require(rooms.roomLayer(room,i-1,layer),"garden layer resource exists");
                    if(!layer.visible())continue;
                    if(layer.type==1){
                        require(order.empty()&&layer.backgroundSpriteId==-1&&
                            layer.backgroundColor==0xff000000u&&layer.backgroundFlags==1,
                            "original garden starts with an opaque black background element");
                        ++backgrounds;continue;
                    }
                    require(layer.type==2,"garden uses background and instance layers");
                    for(unsigned j=layer.membershipCount;j>0;--j){
                        std::uint32_t id;
                        require(rooms.membership(layer.firstMembership+j-1,id),"garden membership exists");
                        serialized.push_back(id);
                        const auto* r=state.instance(id);
                        downwell::PackedObject object;
                        if(!r||!r->active||r->destroyRequested||r->objectId==382||r->objectId==69||
                           !objects.findObject(r->objectId,object)||!object.visible()||object.spriteId<0||
                           (r->objectId==35&&!state.menuPlayerVisible()))continue;
                        order.push_back(r);
                    }
                }
                require(backgrounds==1,"garden has one source background quad");
                // Well Create adds a title, and room startup adds a fade.
                // Dynamic layers are inserted after fixed equal-depth layers
                // (hence drawn before them), with newest active elements first.
                for(unsigned i=0;i<state.activeInstanceCount();++i){
                    const auto& r=state.instanceAt(i);
                    if(std::find(serialized.begin(),serialized.end(),r.instanceId)!=serialized.end())continue;
                    downwell::PackedObject object;
                    if(!r.active||r.destroyRequested||r.objectId==382||r.objectId==69||
                       !objects.findObject(r.objectId,object)||!object.visible()||object.spriteId<0)continue;
                    auto at=order.begin();
                    while(at!=order.end()&&(*at)->depth>r.depth)++at;
                    order.insert(at,&r);
                }
                std::vector<int> expected{-1000001};
                for(const auto* r:order){
                    downwell::PackedObject object;require(objects.findObject(r->objectId,object),"garden object exists");
                    expected.push_back(r->objectId==35?state.menuPlayerSpriteId():
                        r->objectId==60?76:r->spriteId>=0?r->spriteId:object.spriteId);
                }
                if(a.spriteOrder!=expected){
                    std::fprintf(stderr,"Garden actual:");for(int id:a.spriteOrder)std::fprintf(stderr," %d",id);
                    std::fprintf(stderr,"\nGarden expected:");for(int id:expected)std::fprintf(stderr," %d",id);
                    std::fprintf(stderr,"\n");
                }
                require(expected.size()>2&&a.spriteOrder==expected,
                    "garden painter follows source background and serialized layer memberships");
                gardenDrawChecked=true;
            }
            gameplay+=state.inMainRoom();
        }
        require((splashStages&0x7eu)==0x7eu&&garden&&gardenDrawChecked&&style&&gameplay==120,
            "production boot traverses all six splash stages, garden and style selection into gameplay");
    }
    {
        auto tileGame=std::make_unique<downwell::Game>(&rooms,&objects,&builders);
        auto& runtime=const_cast<downwell::StartupRuntime&>(tileGame->startup());runtime.skipToGameForTesting();
        require(runtime.addWallBackground(downwell::MotionBinary64::fromInteger(runtime.mainCameraRenderViewX()+24),
            downwell::MotionBinary64::fromInteger(runtime.mainCameraRenderViewY()+35)),"store room-owned wall patch");
        const auto digest=runtime.stateDigest();
        RecordingRenderer tiles;tiles.recordWallTile=true;tileGame->render(tiles);
        require(tileGame->renderingSucceeded()&&runtime.stateDigest()==digest,"drawing retained tiles preserves state");
        auto firstTile=std::find(tiles.spriteOrder.begin(),tiles.spriteOrder.end(),-739);
        auto firstWall=std::find(tiles.spriteOrder.begin(),tiles.spriteOrder.end(),51);
        require(firstTile!=tiles.spriteOrder.end()&&firstWall!=tiles.spriteOrder.end()&&firstTile<firstWall,
            "depth-100 tile is drawn at its room-relative position before ordinary walls");
    }
    {
        std::ifstream file("generated/amiga-game.dwap",std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
        downwell::EffectMaskAssets assets;
        require(assets.bind(bytes.data(),bytes.size()),"live effect test binds offline art");
        const auto entryBytes=read("generated/amiga-game.entry-mask");
        require(assets.bindEntryMask(entryBytes.data(),entryBytes.size()),"bind immutable entry mask");
        const auto cb=read("generated/downwell-sprites.dwsc"),mb=read("generated/downwell-sprite-masks.dwsm");
        downwell::SpriteCollisionPack sprites(cb.data(),cb.size());downwell::SpriteMaskPack masks(mb.data(),mb.size());
        auto owner=std::make_unique<downwell::StoredWorldCollision>(objects,sprites,masks);
        auto effectStorage = std::make_unique<downwell::Game>(&rooms,&objects,&builders,&assets,owner->binding());
        require(owner->attach(effectStorage->startup()),"composited debris fixture attaches collision owner");
        auto& effectGame = *effectStorage;
        auto& runtime=const_cast<downwell::StartupRuntime&>(effectGame.startup());
        auto entry=runtime.entryDitherRegistration<1>();
        auto camera=[&](downwell::MotionBinary64& out){
            out=downwell::MotionBinary64::fromInteger(runtime.mainCameraRenderViewX()+80);return true;
        };
        require(runtime.withEntryDitherRoomCreation(entry,camera,[&]{runtime.skipToGameForTesting();return true;}),
            "render fixture enters through player-created dither");
        RecordingRenderer covered;covered.effectPixels.assign(160*284,-1);
        const auto entryRng=runtime.randomDrawCount();effectGame.render(covered);
        require(effectGame.renderingSucceeded()&&runtime.randomDrawCount()==entryRng&&
            std::all_of(covered.effectPixels.begin(),covered.effectPixels.end(),[](int p){return p==0;}),
            "first gameplay view is covered by opaque-black original entry art, not white or missing alpha");
        for(bool skeletonShade:{false,true}){
            auto bones=runtime.boneRegistration<1>();auto skeletons=runtime.skeletonRegistration<1>();
            downwell::InstanceHandle actor;
            const int bx=(runtime.mainCameraRenderViewX()+80)&~15,by=(runtime.mainCameraRenderViewY()+140)&~15;
            require(skeletonShade?runtime.spawnSkeleton(skeletons,bx,by,actor):runtime.spawnBone(bones,bx,by,actor),
                "inherited Draw shade fixture uses a registered actor");
            RecordingRenderer shaded;shaded.effectPixels.assign(160*284,-1);effectGame.render(shaded);
            const auto* art=assets.shade();require(art&&effectGame.renderingSucceeded(),"bone shade reaches controller");
            unsigned holes=0;
            for(int y=0;y<284;++y)for(int x=0;x<160;++x){
                const int sx=x-bx+runtime.mainCameraRenderViewX()+art->originX,
                    sy=y-by+runtime.mainCameraRenderViewY()+art->originY;
                const bool bit=sx>=0&&sy>=0&&sx<int(art->mask.width)&&sy<int(art->mask.height)&&
                    (art->mask.data[sy*art->mask.stride+sx/8]&(128u>>(sx&7)));
                require(shaded.effectPixels[y*160+x]==(bit?-1:0),"bone shade subtracts exact offline mask from entry surface");
                holes+=bit;
            }
            require(holes&&bones.clear()&&skeletons.clear(),"actor shade has nonempty verified coverage and retires cleanly");
        }
        downwell::InstanceHandle entryKey;
        require(runtime.visitEntryDitherDraw([&](auto key,const auto&,int){entryKey=key;return true;})&&entryKey.generation,
            "resolve rendered entry lifetime");
        for(int sy:{320,400,450,683,684,800}){
            // Separate static raster fixtures use the effect's actual Create
            // setters, not collision-response writes to an actor motion cache.
            require(entry.clear()&&entry.create(camera,[&](downwell::MotionBinary64& out){
                out=downwell::MotionBinary64::fromInteger(runtime.mainCameraRenderViewY()+sy);return true;
            },entryKey),"create entry cover raster fixture");
            RecordingRenderer edge;edge.effectPixels.assign(160*284,-1);effectGame.render(edge);
            require(effectGame.renderingSucceeded(),"entry cover remains renderable through reveal");
            unsigned offset;require(downwell::EntryDitherMask::offset(sy,offset),"verified mask window");
            for(unsigned y=0;y<284;++y)for(unsigned x=0;x<160;++x){
                const unsigned bx=x+110;
                const bool opaque=entryBytes[offset+y*48+bx/8]&(0x80u>>(bx%8));
                require(edge.effectPixels[y*160+x]==(opaque?0:-1),
                    "Game controller matches immutable mask coverage including black and transparent edge pixels");
            }
        }
        require(!assets.bindEntryMask(nullptr,0),"reject missing entry mask");
        RecordingRenderer missingEntry;effectGame.render(missingEntry);
        require(!effectGame.renderingSucceeded(),"live entry without its mask cannot silently disappear");
        require(assets.bindEntryMask(entryBytes.data(),entryBytes.size())&&entry.clear(),"restore art and retire test entry");
        const auto x=downwell::MotionBinary64::fromInteger(runtime.mainCameraRenderViewX()+80);
        const auto y=downwell::MotionBinary64::fromInteger(runtime.mainCameraRenderViewY()+60);
        auto registration=runtime.debrisRegistration<1>();downwell::DebrisEffects<1> debris;unsigned id=0;
        require(runtime.spawnDebris(registration,debris,x,y,1,id),"register composited debris");
        debris.get(1)->imageIndex.bits=0;
        RecordingRenderer particle;effectGame.render(particle);
        require(effectGame.renderingSucceeded()&&particle.effectWords&&
            std::find(particle.spriteOrder.begin(),particle.spriteOrder.end(),115)==particle.spriteOrder.end(),
            "registered debris reaches the controller without a duplicate direct draw");
        debris.get(1)->flash=1;RecordingRenderer hidden;effectGame.render(hidden);
        require(effectGame.renderingSucceeded()&&!hidden.effectWords,"flash gate removes composited debris coverage");
        debris.get(1)->flash=0;RecordingRenderer returned;effectGame.render(returned);
        require(effectGame.renderingSucceeded()&&returned.effectWords==particle.effectWords,"debris reappears after flash gate");
        require(debris.clear(registration),"retire composited debris fixture");
        RecordingRenderer retired;effectGame.render(retired);
        require(effectGame.renderingSucceeded()&&!retired.effectWords,"retirement clears retained effect coverage");
        require(runtime.emitSmokeBurst(x,y) && runtime.emitFlashBurst(x,y),"live world emits overlapping effect owners");
        const auto draws=runtime.randomDrawCount();
        RecordingRenderer first,second;
        effectGame.render(first);effectGame.render(second);
        require(effectGame.renderingSucceeded() && first.effectWords && first.effectWords==second.effectWords &&
            first.spriteOrder==second.spriteOrder && runtime.randomDrawCount()==draws,
            "repeated live controller composition is deterministic and does not consume RNG");
        bool flash=false,coverage=false;
        for(int id:first.spriteOrder){
            if(id==96){require(!coverage,"flash precedes controller publication");flash=true;}
            if(id==-1000){require(flash,"controller coverage follows flash");coverage=true;}
        }
        require(coverage,"live effect coverage reaches renderer");
        if(std::getenv("DOWNWELL_EFFECT_RENDER_ONLY")) {
            std::puts("Live host controller composition, painter boundary and repaint determinism passed");
            return 0;
        }
    }
    {
        auto failedStorage=std::make_unique<downwell::Game>(&rooms,&objects,&builders);
        auto& failedGame=*failedStorage;
        auto& failedRuntime=const_cast<downwell::StartupRuntime&>(failedGame.startup());
        failedRuntime.skipToGameForTesting();
        unsigned bursts=0;
        while(failedRuntime.emitFlashBurst({},{}))require(++bursts<9,"bounded flash capacity fixture");
        const auto draws=failedRuntime.randomDrawCount(),tick=failedGame.tick();
        failedGame.update({});
        require(failedGame.finished()&&!failedGame.simulationSucceeded(),"host reports simulation failure separately from ordinary quit");
        require(failedRuntime.randomDrawCount()==draws&&failedGame.tick()==tick,"failed update does not continue simulation");
        failedRuntime.reset(downwell::PlatformProfile::desktop);
        failedGame.update({});
        require(!failedGame.simulationSucceeded()&&failedGame.tick()==tick,"Game failure is terminal even if inner runtime is reset");
    }
    {
        auto flashStorage = std::make_unique<downwell::Game>(&rooms,&objects,&builders);
        auto& flashGame = *flashStorage;
        auto& runtime=const_cast<downwell::StartupRuntime&>(flashGame.startup());
        runtime.skipToGameForTesting();
        require(runtime.emitFlashBurst(downwell::MotionBinary64::fromInteger(int(runtime.mainCameraViewX())+40),
                                      downwell::MotionBinary64::fromInteger(int(runtime.mainCameraViewY())+60)),
                "decoded world owns the flash burst");
        for(unsigned frame=0;frame<=7;++frame){
            RecordingRenderer capture;
            flashGame.render(capture);
            unsigned count=0;
            runtime.flashEffects().visitNewest([&](const downwell::FlashBallEffect& effect){
                require(count<capture.flashDraws.size(),"flash effect reaches host renderer");
                const auto& draw=capture.flashDraws[count++];
                require(draw.frame==effect.drawFrame() &&
                    draw.x==110+effect.pixelX()-int(runtime.mainCameraViewX()) &&
                    draw.y==effect.pixelY()-int(runtime.mainCameraViewY()),"flash projection follows camera");
            });
            require(count==capture.flashDraws.size() && (frame<7?count>=4:count==0),
                    "host world animation retires flash effects on seventh update");
            if(count){
                bool playerSeen=false,flashSeen=false;
                for(int sprite:capture.spriteOrder){
                    if(sprite==runtime.mainPlayerSpriteId()){
                        require(!flashSeen,"source player depth precedes flash balls");
                        playerSeen=true;
                    }
                    if(sprite==96){
                        require(playerSeen,"flash balls are in front of the live player");
                        flashSeen=true;
                    }
                }
                require(playerSeen && flashSeen,"painter-order fixture covers player and flash");
            }
            runtime.step(false);
        }
        if(std::getenv("DOWNWELL_FLASH_RENDER_ONLY")) {
            require(runtime.emitCorpseExpiry(downwell::MotionBinary64::fromInteger(int(runtime.mainCameraViewX())+50),
                                            downwell::MotionBinary64::fromInteger(int(runtime.mainCameraViewY())+80)),
                    "world admits owned corpse expiry");
            unsigned bulletsSeen=0,playersSeen=0,expirySeen=0;
            for(unsigned tick=0;tick<32;++tick){
                downwell::Input fire;fire.fire=true;runtime.step(fire,false);
                RecordingRenderer capture;flashGame.render(capture);
                int previousDepth=20000;
                for(int sprite:capture.spriteOrder){
                    int depth;
                    if(sprite==133||sprite==134)depth=10000;
                    else if(sprite==458||sprite==459){depth=0;++bulletsSeen;}
                    else if(sprite==102){depth=-1000;++expirySeen;}
                    else if(sprite==runtime.mainPlayerSpriteId()){depth=-50000;++playersSeen;}
                    else if(sprite==96)depth=-200000;
                    else continue;
                    require(depth<=previousDepth,"original direct gem/bullet/player/flash depth order");
                    previousDepth=depth;
                }
            }
            require(bulletsSeen&&playersSeen,"live painter test covers fired bullets and player");
            require(expirySeen==32,"owned expiry emits one draw per surviving frame");
            require(runtime.corpseExpiryEffects().size()==1,"expiry survives independently through first 32 updates");
            for(unsigned i=0;i<8;++i)runtime.step(false);
            require(runtime.corpseExpiryEffects().size()==0,"world retires expiry on animation boundary");
            const int nx=int(runtime.mainCameraViewX())+80,ny=int(runtime.mainCameraViewY())+104;
            require(runtime.emitCorpseExpiry(downwell::MotionBinary64::fromInteger(nx),downwell::MotionBinary64::fromInteger(ny)),"older foreground fixture");
            require(runtime.emitDamageNumber(downwell::MotionBinary64::fromInteger(nx),downwell::MotionBinary64::fromInteger(ny),downwell::MotionBinary64::fromInteger(12),{}),"damage text fixture");
            RecordingRenderer textCapture;flashGame.render(textCapture);
            require(flashGame.renderingSucceeded()&&textCapture.damageTexts.size()==5,"damage text reaches host renderer");
            const int dx[]={1,-1,0,0,0},dy[]={0,0,1,-1,0};
            for(unsigned i=0;i<5;++i){const auto& t=textCapture.damageTexts[i];
                require(t.x==110+nx-runtime.mainCameraViewX()+dx[i]&&t.y==ny-4-runtime.mainCameraViewY()+dy[i],"damage text preserves source offsets");
                require(t.color.r==(i==4?255:0)&&t.color.g==0&&t.color.b==0,"damage text has black outline and red center");
            }
            unsigned textCommands=0;
            for(int sprite:textCapture.spriteOrder){if(sprite==-1001)++textCommands;if(sprite==102)require(textCommands==5,"new text precedes older equal-depth sprite");}
            require(runtime.emitDamageNumber({}, {},{0x3fe00000u,0},{}),"unsupported display amount fixture");
            RecordingRenderer unsupported;flashGame.render(unsupported);require(!flashGame.renderingSucceeded(),"unsupported formatting fails explicitly");
            std::puts("Production host flash ownership, camera projection and retirement passed");
            return 0;
        }
    }
    {
        auto enemyStorage = std::make_unique<downwell::Game>(&rooms, &objects, &builders);
        auto& enemyGame = *enemyStorage;
        auto& runtime = const_cast<downwell::StartupRuntime&>(enemyGame.startup());
        runtime.skipToGameForTesting();
        downwell::InstanceRecord* fixture = nullptr;
        for (unsigned i = 0; i < runtime.activeInstanceCount(); ++i) {
            auto& instance = const_cast<downwell::InstanceRecord&>(runtime.instanceAt(i));
            if (instance.active && instance.objectId == 60) { fixture = &instance; break; }
        }
        require(fixture != nullptr, "enemy draw fixture has an active instance");
        fixture->x = static_cast<int>(runtime.mainCameraViewX()) + 37;
        fixture->y = static_cast<int>(runtime.mainCameraViewY()) + 91;
        fixture->imageIndex = 1;
        fixture->scaleX = -1; fixture->scaleY = -1; fixture->rotation = 90;
        // Published Step selections: defaults, frog stun, bat hit/hang/wake,
        // snail stun, and crawler. Rendering cannot infer these from grounded.
        const int selections[][2] = {{111,163},{111,164},{112,186},{112,187},{112,188},
            {113,166},{113,170},{113,168},{113,173},{116,153},{116,154},{116,157},
            {116,162},{117,177},{117,178},{119,278}};
        const auto rngBefore = runtime.randomDrawCount();
        for (const auto& selection : selections) {
            fixture->objectId = selection[0]; fixture->spriteId = selection[1];
            RecordingRenderer capture; capture.recordTransforms = true;
            enemyGame.render(capture);
            unsigned matches = 0;
            for (const auto& draw : capture.transforms)
                if (draw.x == fixture->x - runtime.mainCameraViewX() + 110 &&
                    draw.y == fixture->y - runtime.mainCameraViewY()) {
                    require(draw.sprite == selection[1] && draw.frame == 1 &&
                            draw.sx == -1 && draw.sy == -1 && draw.angle == 90,
                            "enemy rendering preserves the runtime-selected sprite and transform");
                    ++matches;
                }
            require(matches == 1, "published enemy pose is drawn exactly once");
        }
        require(runtime.randomDrawCount() == rngBefore, "enemy rendering leaves RNG untouched");
    }
    auto damagedStorage = std::make_unique<downwell::Game>(&rooms, &objects, &builders);
    auto& damagedGame = *damagedStorage;
    const_cast<downwell::StartupRuntime&>(damagedGame.startup()).skipToGameForTesting();
    unsigned mainTicks = 0, hidden = 0, flashingVisible = 0, firstDamage = 0;
    bool damaged = false, recovered = false, gemFlashChecked = false;
    for (unsigned tick = 0; tick < downwell::gameplayContactProfileFrames && !recovered; ++tick) {
        const auto& state = damagedGame.startup();
        input = {};
        if (state.inMenu()) input.right = true;
        else if (state.inPlayMenu() && state.playMenuYOffset() == 0) input.fire = true;
        else if (state.inMainRoom()) {
            // Use the same natural-contact tape as the host/native profile
            // gate, revalidated after source entry RNG changes. Do not inject
            // damage when a former tape stops reaching an enemy.
            input = downwell::gameplayContactProfileInput(mainTicks,
                int(state.mainPlayerX()), int(state.mainPlayerY()));
        }
        damagedGame.update(input);
        require(damagedGame.simulationSucceeded(), "natural damage route has no terminal simulation failure");
        if (!state.inMainRoom()) continue;
        ++mainTicks;
        if (!gemFlashChecked && state.mainCurrencyCount()) {
            auto& gem = const_cast<downwell::StartupRuntime::MainCurrencyState&>(state.mainCurrencyAt(0));
            const unsigned oldAge = gem.motion.lifetime.age;
            const auto rng = state.randomDrawCount();
            unsigned baseline = 0;
            for (std::size_t i = 1; i < state.mainCurrencyCount(); ++i)
                baseline += state.mainCurrencyAt(i).motion.lifetime.visible();
            for (unsigned age : {0u, 188u, 189u, 190u, 307u, 308u, 309u, 65536u}) {
                gem.motion.lifetime.age = age;
                RecordingRenderer capture;
                damagedGame.render(capture);
                require(capture.gemDraws == baseline + (age < 189 || (age & 1u)==0),
                        "live renderer omits flashing currency without changing other gems");
                require(gem.motion.lifetime.age == age && state.randomDrawCount() == rng,
                        "repainting a flashing gem changes neither lifetime nor RNG");
            }
            gem.motion.lifetime.age = oldAge;
            gemFlashChecked = true;
        }
        const int immunity = state.mainPlayerInvulnerability();
        if (immunity > 0) {
            if (!damaged) firstDamage = mainTicks;
            damaged = true;
        }
        if (!damaged) continue;
        renderer.targetPlayer = state.mainPlayerSpriteId();
        renderer.playerDraws = 0;
        renderer.redPixels = 0;
        renderer.bulletAngles.clear();
        damagedGame.render(renderer);
        require(renderer.bulletAngles.size() == state.mainBulletCount(),
                "each live projectile emits exactly one artwork transform");
        for (std::size_t i = 0; i < state.mainBulletCount(); ++i)
            require(renderer.bulletAngles[i] == static_cast<float>(
                        state.mainBulletAt(state.mainBulletCount()-1-i).trajectory.direction * (360.0 / 4294967296.0)),
                    "artwork receives retained direction in original newest-first Draw order");
        require(renderer.playerDraws == (immunity % 2 == 0 ? 1u : 0u),
                "actual Game render suppresses odd damage phases and draws even phases once");
        auto& inventory = const_cast<downwell::UpgradeInventory&>(state.upgrades());
        const auto originalLevel = inventory.levels[12];
        const auto drawsBefore = state.randomDrawCount();
        const auto redBefore = renderer.redPixels;
        inventory.levels[12] = 1; // Presentation-only fixture; do not change the replay's shots.
        damagedGame.render(renderer);
        inventory.levels[12] = originalLevel;
        require(renderer.redPixels - redBefore > redBefore && state.randomDrawCount() == drawsBefore,
                "Laser Sight renders through damage blink without consuming RNG");
        if (immunity == 0) recovered = true;
        else if (immunity % 2) ++hidden;
        else ++flashingVisible;
    }
    if(!(hidden>0 && flashingVisible>0 && recovered))
        std::fprintf(stderr,"Damage tape: main=%u hidden=%u visible=%u health=%d recovered=%d x=%.2f y=%.2f grounded=%d activated=%u defeated=%u\n",
            mainTicks,hidden,flashingVisible,damagedGame.startup().mainPlayerHealth(),int(recovered),
            damagedGame.startup().mainPlayerX(),damagedGame.startup().mainPlayerY(),
            int(damagedGame.startup().mainPlayerGrounded()),damagedGame.startup().mainEnemiesActivated(),
            damagedGame.startup().mainEnemiesDefeated());
    if (!recovered) {
        const auto& state = damagedGame.startup();
        std::fprintf(stderr,"Builder initialized=%d y=%d rows=%u gen=%d camera=%.2f instances=%zu\n",
            int(state.mainBuilderInitialized()),state.mainBuilderY(),state.mainBuilderRowsGenerated(),
            state.mainBuilderGenerationCount(),state.mainCameraViewY(),state.activeInstanceCount());
    }
    require(hidden > 0 && flashingVisible > 0 && recovered,
            "live render tape covers hidden, visible and fully recovered player states");
    std::printf("Damage checkpoint: first=%u recovery=%u hidden=%u visible=%u health=%d\n",
        firstDamage,mainTicks,hidden,flashingVisible,damagedGame.startup().mainPlayerHealth());
    // Revalidated with the unchanged input tape in both world adapters after
    // restoring source gem RNG/timing; do not inject damage to satisfy this.
    require(firstDamage == 645 && mainTicks == 735 && hidden == 45 && flashingVisible == 45 &&
            damagedGame.startup().mainPlayerHealth() == 3,
            "render route reaches the verified natural contact and complete immunity interval");
    require(gemFlashChecked, "live render tape exercises a spawned currency item's flash phases");
    require(renderer.bulletDraws > 0, "live render tape exercises projectile transforms");
    require(renderer.spreadBulletDrawn, "live rendering actually exercises nonzero spread angles");
    std::printf("Damage route: %u main updates, %u hidden and %u visible damage frames, recovered\n",
        mainTicks,hidden,flashingVisible);
    if (std::getenv("DOWNWELL_DAMAGE_RENDER_ONLY")) return 0;
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
    // Drive real Game input dispatch; only publication of the results menu is
    // injected here (natural death has separate long-replay coverage).
    for (int selection = 0; selection < 3; ++selection) {
        auto& runtime = const_cast<downwell::StartupRuntime&>(damagedGame.startup());
        if (!runtime.inMainRoom())
            require(runtime.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 42),
                    "dispatch fixture recreates main room");
        damagedGame.update({});
        auto& results = const_cast<downwell::DeathResults&>(runtime.deathResults());
        results = {};
        results.open(true, 0, 0);
        results.menu.sequenceOver = true;
        results.menu.visible[5] = true;
        results.menu.cursor = selection;
        input = {};
        input.fire = true;
        damagedGame.update(input, 42);
        const int destinations[] = {11, 8, 20};
        require(runtime.roomId() == destinations[selection] && !runtime.deathResults().active &&
                    runtime.randomSeed() == 42 && !damagedGame.finished(),
                "confirmation executes the chosen results destination using supplied seed");
        damagedGame.update(input, 99);
        require(runtime.randomSeed() == 42 && !runtime.playMenuSelected(),
                "held confirmation cannot repeat restart or select a style");
    }
    auto deathStorage = std::make_unique<downwell::Game>(&rooms, &objects, &builders);
    auto& naturalDeath = *deathStorage;
    // This separate route retains natural startup and room-entry timing.
    unsigned combatTicks = 1;
    bool retried = false;
    unsigned observedFlashFrames = 0;
    for (unsigned frame = 0; frame < 14000 && !retried; ++frame) {
        const auto& runtime = naturalDeath.startup();
        input = {};
        if (runtime.inMenu()) input.right = true;
        else if (runtime.inPlayMenu() && runtime.playMenuYOffset() == 0) input.fire = true;
        else if (runtime.inMainRoom()) {
            if (runtime.deathResults().active) {
                const auto& menu = runtime.deathResults().menu;
                if (menu.sequenceOver && !menu.inputDisabled && !menu.goalPassed) {
                    const auto lifetime = runtime.runGemStatistics().lifetime;
                    require(runtime.mainPlayerHealth() == 0, "natural results follow actual fatal damage");
                    input.fire = true;
                    naturalDeath.update(input, 42);
                    require(runtime.inMainRoom() && runtime.mainPlayerHealth() == 4 &&
                        !runtime.deathResults().active && runtime.runTimeTicks() == 0 &&
                        runtime.runGemStatistics().lifetime == lifetime,
                        "natural death confirmation returns to a living run with records retained");
                    retried = true;
                    break;
                }
                if (menu.goalPassed && menu.goalNotice >= 3) input.fire = (frame % 2) == 0;
            } else {
                input.left = (combatTicks / 90) % 2 == 0;
                input.right = !input.left;
                input.fire = (combatTicks + downwell::gameplayContactProfilePhase) % 10 < 5;
                // Do not let the timed sweep settle into the left side room:
                // its floor can trap this periodic tape away from new enemies.
                if (runtime.mainPlayerX() < 192) { input.left=false; input.right=true; }
                else if (runtime.mainPlayerX() > 288) { input.left=true; input.right=false; }
                if (runtime.runStage().level == 1 && runtime.mainPlayerY() < 1264) {
                    // Only level one has this recovered entrance bowl. Later
                    // entrances need the normal sweep to find an opening.
                    input.left = runtime.mainPlayerX() > 242;
                    input.right = runtime.mainPlayerX() < 238;
                }
                {
                    // Seek a nearby live enemy instead of repeatedly killing
                    // every reachable threat with the blind firing sweep.
                    // This also applies to enemies near the entrance bowl.
                    const downwell::InstanceRecord* threat = nullptr;
                    float nearest = 64;
                    for (std::size_t i=0;i<runtime.activeInstanceCount();++i) {
                        const auto& enemy=runtime.instanceAt(i);
                        const int id=enemy.objectId;
                        if (!enemy.active || enemy.health<=0 ||
                            !(id==111 || id==112 || id==113 || id==116 || id==117 || id==119)) continue;
                        if (std::abs(enemy.y-runtime.mainPlayerY())>64) continue;
                        const float distance=std::abs(enemy.x-runtime.mainPlayerX())+
                            std::abs(enemy.y-runtime.mainPlayerY());
                        if (distance<nearest) {nearest=distance;threat=&enemy;}
                    }
                    if (threat) {
                        input.left=runtime.mainPlayerX()>threat->x;
                        input.right=runtime.mainPlayerX()<threat->x;
                        // Stop shooting only when the threat is at contact
                        // height. Otherwise the player can wait forever on a
                        // platform above it instead of breaking through.
                        if (std::abs(threat->y-runtime.mainPlayerY()) < 16)
                            input.fire=false;
                    }
                    ++combatTicks;
                }
            }
        }
        const bool wasDeadWithResults = runtime.mainPlayerHealth() == 0 && runtime.deathResults().active;
        const auto lifetime = runtime.runGemStatistics().lifetime;
        naturalDeath.update(input, 42);
        require(naturalDeath.simulationSucceeded(), "natural death route has no terminal simulation failure");
        if (std::getenv("DOWNWELL_TRACE_DEATH_ROUTE") && frame % 100 == 0)
            std::fprintf(stderr,"route frame=%u combat=%u level=%u xy=%.3f,%.3f v=%.3f,%.3f ground=%d ammo=%d hp=%d input=%d%d%d lock=%d\n",
                frame,combatTicks,unsigned(runtime.runStage().level),runtime.mainPlayerX(),runtime.mainPlayerY(),
                runtime.mainPlayerSpeedX(),runtime.mainPlayerSpeedY(),runtime.mainPlayerGrounded(),
                runtime.mainAmmo(),runtime.mainPlayerHealth(),input.left,input.right,input.fire,
                runtime.shops().noControl);
        if (runtime.mainDeathSequence().started && runtime.mainDeathSequence().frames <= 3) {
            RecordingRenderer flashRenderer;
            naturalDeath.render(flashRenderer);
            const unsigned expected = runtime.mainDeathSequence().frames < 3 ? 1 : 0;
            require(flashRenderer.deathFlashCount == expected,
                    "live rendering flashes only the central playfield on the source interval");
            observedFlashFrames += flashRenderer.deathFlashCount;
        }
        // The original event can acknowledge its last unlock and confirm the
        // selected action on the same press. Observe that path as well.
        if (wasDeadWithResults && !runtime.deathResults().active) {
            require(runtime.inMainRoom() && runtime.mainPlayerHealth() == 4 &&
                runtime.runTimeTicks() == 0 && runtime.runGemStatistics().lifetime == lifetime &&
                runtime.randomSeed() == 42, "unlock acknowledgement can also execute Retry in its source event");
            retried = true;
        }
    }
    if (!retried) std::fprintf(stderr, "death tape health=%d y=%.1f room=%d results=%d over=%d goal=%d notice=%d\n",
        naturalDeath.startup().mainPlayerHealth(), naturalDeath.startup().mainPlayerY(),
        naturalDeath.startup().roomId(), naturalDeath.startup().deathResults().active,
        naturalDeath.startup().deathResults().menu.sequenceOver,
        naturalDeath.startup().deathResults().menu.goalPassed,
        naturalDeath.startup().deathResults().menu.goalNotice);
    if (!retried) {
        const auto& runtime=naturalDeath.startup();
        std::fprintf(stderr,"death route x=%.1f grounded=%d defeated=%u\n",runtime.mainPlayerX(),
            int(runtime.mainPlayerGrounded()),runtime.mainEnemiesDefeated());
        for(std::size_t i=0;i<runtime.activeInstanceCount();++i){
            const auto& enemy=runtime.instanceAt(i);
            const int id=enemy.objectId;
            if(enemy.active && enemy.health>0 &&
               (id==111 || id==112 || id==113 || id==116 || id==117 || id==119))
                std::fprintf(stderr," threat=%d xy=%d,%d health=%d\n",id,enemy.x,enemy.y,enemy.health);
        }
    }
    require(retried, "bounded natural-death tape reaches and executes Retry");
    require(observedFlashFrames == 2, "natural death renders both white flash frames before Retry");
    std::printf("Natural death route: %u combat updates, %u flash frames, Retry passed\n",
        combatTicks-1, observedFlashFrames);
    auto shopStorage = std::make_unique<downwell::Game>(&rooms, &objects, &builders);
    auto& shopGame = *shopStorage;
    auto& shopRuntime = const_cast<downwell::StartupRuntime&>(shopGame.startup());
    shopRuntime.skipToGameForTesting();
    downwell::PackedObject exitObject;
    require(objects.findObject(306, exitObject) && !exitObject.visible() && exitObject.spriteId == 127,
        "original exit trigger has a collision sprite but is invisible");
    bool insertedTrigger = false;
    for (unsigned i = 0; i < shopRuntime.activeInstanceCount(); ++i) {
        auto& instance = const_cast<downwell::InstanceRecord&>(shopRuntime.instanceAt(i));
        if (instance.active && instance.objectId == 60) {
            instance.objectId = 306;
            insertedTrigger = true;
            break;
        }
    }
    require(insertedTrigger, "render fixture contains an active generated exit trigger");
    auto* shop = shopRuntime.registerShop(250, 240);
    require(shop != nullptr, "shop renderer fixture registration");
    shop->menu.active = true;
    shop->menu.menuState = 1;
    shop->menu.cursor = 1;
    const int phaseBefore = shop->menu.cursorPhaseQuarters;
    RecordingRenderer shopRenderer;
    shopGame.render(shopRenderer);
    shopGame.render(shopRenderer);
    require(shopRenderer.shopPanels == 2 && shopRenderer.shopNameCenters == 2,
        "Game renders shared shop panel and middle-centered item text at landscape offset");
    require(shopRenderer.triggerDraws == 0, "active level-exit collision sprite is never drawn");
    require(shop->menu.cursorPhaseQuarters == phaseBefore,
        "extra rendering cannot advance shop cursor presentation state");
    shop->menu.active = false;
    RecordingRenderer closedShop;
    shopGame.render(closedShop);
    require(closedShop.shopPanels == 0 && closedShop.shopNameCenters == 0,
        "closed shop no longer submits menu overlay");
    auto& exitDither = const_cast<downwell::ExitDither&>(shopRuntime.exitDither());
    exitDither.create(shopRuntime.mainCameraRenderViewX(), shopRuntime.mainCameraRenderViewY());
    exitDither.step(shopRuntime.mainCameraRenderViewY(), 284);
    const int ditherWorldY = exitDither.y;
    RecordingRenderer closingLevel;
    shopGame.render(closingLevel); shopGame.render(closingLevel);
    require(closingLevel.exitDitherDraws == 2 && closingLevel.exitDitherX == 190 &&
        closingLevel.exitDitherY == -92 && exitDither.y == ditherWorldY && exitDither.alarm0 == 15,
        "Game projects recovered exit sprite and repeated rendering does not advance motion or alarms");
    auto clearStorage = std::make_unique<downwell::Game>(&rooms, &objects, &builders);
    auto& clearGame = *clearStorage;
    auto& clearRuntime = const_cast<downwell::StartupRuntime&>(clearGame.startup());
    clearRuntime.skipToGameForTesting();
    clearRuntime.registerLevelExit(int(clearRuntime.mainPlayerY()));
    for (unsigned tick = 0; tick < 96; ++tick) clearGame.update({});
    require(clearRuntime.levelClearActive(), "Game automatically consumes result request at scene boundary");
    const auto clockBeforeClear = clearRuntime.runTimeTicks();
    for (unsigned tick = 0; tick < 60; ++tick) clearGame.update({});
    require(clearRuntime.levelClear().selection.visible, "live clear scene reaches upgrade selection");
    const auto timeBeforeDraw = clearRuntime.levelClearFrame().time;
    RecordingRenderer clearRenderer;
    clearGame.render(clearRenderer); clearGame.render(clearRenderer);
    require(clearRenderer.clearTitleDraws == 14 && clearRenderer.clearChoiceDraws == 14 &&
        clearRuntime.levelClearFrame().time == timeBeforeDraw && clearRuntime.runTimeTicks() == clockBeforeClear,
        "live clear title/choice shadow layers render without advancing time or gameplay");
    downwell::Input navigate; navigate.right = true;
    clearGame.update(navigate); clearGame.update(navigate);
    require(clearRuntime.levelClear().selection.cursor == 2, "held navigation causes only one menu edge");
    require(clearRuntime.grantUpgrade(19), "entry retention fixture increases charge capacity");
    const int retainedCapacity = clearRuntime.mainAmmoCapacity();
    const unsigned retainedCurrency = clearRuntime.mainCurrencyTotal();
    int retainedHealth = clearRuntime.mainPlayerHealth();
    downwell::Input confirm; confirm.fire = true;
    bool nextRoomLoaded = false;
    for (unsigned tick = 0; tick < 240; ++tick) {
        if (clearRuntime.levelClearActive() && !clearRuntime.levelClear().selection.visible)
            retainedHealth = clearRuntime.mainPlayerHealth();
        clearGame.update(confirm);
        if (clearRuntime.runStage().level == 2) { nextRoomLoaded = true; break; }
    }
    require(nextRoomLoaded && !clearRuntime.levelClearActive() && clearRuntime.inMainRoom() &&
        clearRuntime.roomLoadSucceeded() && clearRuntime.deferredCommandsSucceeded(),
        "live clear selection and departure load the next area-one room");
    require(clearRuntime.mainBuilderGenerationCount() == -10 && !clearRuntime.mainBuilderShopSuppressed(),
        "second stage uses its source generation count and permits natural shops");
    require(clearRuntime.mainAmmoCapacity() == retainedCapacity && clearRuntime.mainAmmo() == retainedCapacity &&
        clearRuntime.mainCurrencyTotal() >= retainedCurrency && clearRuntime.mainPlayerHealth() == retainedHealth &&
        clearRuntime.upgrades().count == 1 && clearRuntime.runTimeTicks() == clockBeforeClear,
        "room loading preserves grants, health, currency and run clock while refilling charge");
    const auto beforePlay = clearRuntime.mainPlayerY();
    const auto beforePlayTick = clearRuntime.tick();
    clearGame.update({});
    require(clearRuntime.startTimeField()&&clearRuntime.runTimeTicks()==clockBeforeClear&&
        clearRuntime.tick()==beforePlayTick+1&&clearRuntime.mainPlayerY()>=beforePlay,
        "new stage updates gameplay while its entrance field pauses the clock");
    for(unsigned tick=0;tick<120&&clearRuntime.runTimeTicks()==clockBeforeClear;++tick)clearGame.update({});
    require(clearRuntime.runTimeTicks()>clockBeforeClear,
        "leaving the entrance field resumes the run clock");
    clearRuntime.registerLevelExit(int(clearRuntime.mainPlayerY()));
    for (unsigned tick = 0; tick < 330 && clearRuntime.runStage().level == 2; ++tick)
        clearGame.update(confirm);
    require(clearRuntime.runStage().area == 1 && clearRuntime.runStage().level == 3 &&
        !clearRuntime.levelClearActive() && clearRuntime.upgrades().count == 2,
        "a second complete selection/departure enters area-one level three without resetting inventory");
    clearRuntime.registerLevelExit(int(clearRuntime.mainPlayerY()));
    for (unsigned tick = 0; tick < 330; ++tick) clearGame.update(confirm);
    require(clearRuntime.levelClearActive() && clearRuntime.nextLevelRequested() &&
        clearRuntime.runStage().area == 1 && clearRuntime.runStage().level == 3,
        "unimplemented next-area builder retains request rather than silently reusing area-one terrain");
    std::puts("game render tests passed");
    return 0;
}
