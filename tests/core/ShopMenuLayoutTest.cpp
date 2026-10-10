#include "core/ShopMenuLayout.h"
#include "core/ShopMenuText.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
using Text = downwell::ShopMenuText;
struct Command {
    int type, id, value, x, y, extra;
    bool operator==(const Command& b) const {
        return type == b.type && id == b.id && value == b.value &&
            x == b.x && y == b.y && extra == b.extra;
    }
};
struct Sink {
    std::vector<Command> commands;
    void asset(int id, unsigned frame, int x, int y) { commands.push_back({0,id,int(frame),x,y,0}); }
    void arrow(int phase, int x, int y, int angle) { commands.push_back({1,484,phase,x,y,angle}); }
    void text(Text id, unsigned value, int x, int y, bool red) {
        commands.push_back({2,int(id),int(value),x,y,red});
    }
};
struct TextSink {
    std::vector<Command> commands;
    void mask(int id, int x, int y, unsigned pen) {
        commands.push_back({3,id,0,x,y,int(pen)});
    }
};
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    TextSink text;
    downwell::drawShopMenuText(text, Text::price, 300, 80, 111, true);
    require(text.commands.size() == 20, "three digits and G in five complete text passes");
    const int ids[] = {65443,65440,65440,65450};
    const int xs[] = {64,72,80,96};
    const int dx[] = {1,-1,0,0,0}, dy[] = {0,0,1,-1,0};
    for (unsigned pass = 0; pass < 5; ++pass)
        for (unsigned i = 0; i < 4; ++i)
            require(text.commands[pass * 4 + i] == Command{3,ids[i],0,xs[i]+dx[pass],111+dy[pass],pass == 4 ? 2 : 3},
                "price retains space advance, center anchor and whole-string outline ordering");
    text.commands.clear();
    downwell::drawShopMenuText(text, Text::price, 0, 80, 111, false);
    require(text.commands.size() == 10 && text.commands[8] == Command{3,65440,0,72,111,1} &&
        text.commands[9] == Command{3,65450,0,88,111,1}, "zero price has one digit and white foreground");
    text.commands.clear();
    downwell::drawShopMenuText(text, Text::price, 0xffffffffu, 80, 111, false);
    require(text.commands.size() == 55, "maximum unsigned price fits scratch storage");
    for (unsigned item = 100; item <= 105; ++item) {
        text.commands.clear();
        downwell::drawShopMenuText(text, Text::itemDescription, item, 85, 137, false);
        require(text.commands.size() == 5 && text.commands.back() == Command{3,65430+int(item)-100,0,85,137,1},
            "description resolves original upgrade ID to offline centered mask");
    }
    downwell::ShopWorld::Instance shop;
    shop.present = true; shop.x = 300; shop.y = 500;
    shop.menu.active = true; shop.menu.menuState = 1;
    shop.menu.cursorWobbleX = 5; shop.menu.cursorPhaseQuarters = -3;
    shop.stock.items[0] = {100,1,300};
    Sink sink;
    auto draw = [&](unsigned money = 300, int height = 284) {
        sink.commands.clear();
        return downwell::drawShopMenu(shop, 200, 400, height, money, sink);
    };
    require(draw(), "buy branch supported");
    const std::vector<Command> expected{
        {0,489,0,0,57,0}, {1,484,-3,140,72,0}, {0,498,1,140,72,0},
        {0,152,0,85,93,0}, {2,int(Text::itemName),100,85,69,0},
        {2,int(Text::price),300,85,111,0}, {2,int(Text::itemDescription),100,85,137,0}};
    require(sink.commands == expected, "source ordered buy layout, viewport subtraction and opposite wobble signs");
    draw(299);
    require(sink.commands[5].extra == 1, "insufficient price alone becomes red");
    draw(300, 240);
    require(sink.commands[0].y == 35 && sink.commands[1].y == 72,
        "camera height shifts panel, not world-anchored arrow");
    shop.stock.items[0].remaining = 0;
    draw();
    require(sink.commands.size() == 4 && sink.commands.back().id == int(Text::purchased),
        "sold item hides icon, price and description");
    shop.menu.cursor = 0;
    draw();
    require(sink.commands.size() == 3 && sink.commands[1].extra == 270 &&
        sink.commands[1].x == 121 && sink.commands.back().id == int(Text::back),
        "leave rotates arrow and omits numbered overlay");
    shop.menu.menuState = 0; shop.menu.menuSelect = 1;
    draw();
    require(sink.commands.size() == 4 && sink.commands[1].x == 32 &&
        sink.commands[2].x == 80 && sink.commands[3].x == 75 && sink.commands[3].y == 87,
        "legacy entry branch uses fixed choices and decaying cursor offset");
    shop.menu.menuState = 2;
    require(!draw() && sink.commands.empty(), "unimplemented selling must be explicit");
    shop.sourceId=123;
    require(draw()&&sink.commands.empty(),"registered parent does not enter compact menu composition");
    shop.sourceId=0;
    shop.menu.active = false;
    require(draw() && sink.commands.empty(), "inactive menu emits nothing");
    shop.menu.active = true; shop.present = false;
    require(draw() && sink.commands.empty(), "retired counter cannot retain menu overlay");
    std::puts("Shared shop menu source layout checks passed");
}
