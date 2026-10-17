// PixelCollisionMath against its FixedCollisionMath oracle: every operation
// over a span of values, then the crawler, snail and walker scripts side by
// side on random aligned-cell terrain, comparing all state after each event.
#include "core/CrawlerMotion.h"
#include "core/PixelCollisionMath.h"
#include "core/SnailMotion.h"
#include "core/WalkingEnemyMotion.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
using Fixed = downwell::FixedCollisionMath;
using Pixel = downwell::PixelCollisionMath;
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
struct Random {
    std::uint32_t state;
    std::uint32_t next() { state = state * 1664525u + 1013904223u; return state >> 8; }
    int range(int low, int high) { return low + int(next() % std::uint32_t(high - low + 1)); }
    bool chance(int percent) { return range(0, 99) < percent; }
};
long value(Pixel::Number v) { return Pixel::fixedValue(v); }

void operations() {
    for (long v = -40L * 1280; v <= 40L * 1280; ++v) {
        const auto p = Pixel::fixed(v);
        require(p.fraction >= 0 && p.fraction < 1280 && value(p) == v, "fixed() holds every value exactly");
        require(Pixel::roundPixel(p) == Fixed::roundPixel(v) && value(Pixel::round(p)) == Fixed::round(v),
                "rounding ties to even on the magnitude");
        require(value(Pixel::ceilAbs(p)) == Fixed::ceilAbs(v), "ceilAbs rounds the magnitude up");
        require(Pixel::floorPixel(p) == Fixed::floorPixel(v), "floorPixel floors");
        require(Pixel::truncatedQ8(p) == Fixed::truncatedQ8(v), "Q8 truncates toward zero");
        require(Pixel::floorQ8(p) == Fixed::floorQ8(v) && Pixel::floorQ8Remainder(p) == Fixed::floorQ8Remainder(v),
                "Q8 floor and its remaining fifths");
        require((-p).fraction >= 0 && (-p).fraction < 1280 && value(-p) == -v && value(-1 * p) == -v && value(1 * p) == v && value(0 * p) == 0,
                "negation and sign scaling");
    }
    Random random{12345};
    for (int i = 0; i < 2000000; ++i) {
        const long a = random.range(-9000000, 9000000), b = random.range(-9000000, 9000000);
        const auto pa = Pixel::fixed(a), pb = Pixel::fixed(b);
        const auto sum = pa + pb, difference = pa - pb;
        require(value(sum) == a + b && value(difference) == a - b && sum.fraction >= 0 && sum.fraction < 1280 &&
                difference.fraction >= 0 && difference.fraction < 1280, "addition and subtraction carry");
        require((pa < pb) == (a < b) && (pa > pb) == (a > b) && (pa <= pb) == (a <= b) &&
                (pa >= pb) == (a >= b) && (pa == pb) == (a == b) && (pa != pb) == (a != b),
                "comparisons order values");
        require(Pixel::roundPixel(pa) == Fixed::roundPixel(a) && Pixel::truncatedQ8(pa) == Fixed::truncatedQ8(a) &&
                Pixel::floorQ8(pa) == Fixed::floorQ8(a) && Pixel::floorQ8Remainder(pa) == Fixed::floorQ8Remainder(a),
                "conversions across the well's coordinate range");
    }
}

// Aligned 16x16 cells with a random fill; the crawler/snail/walker solid
// queries are point samples of it.
struct Terrain {
    std::vector<bool> cells;
    int originX = 0, originY = 0, columns = 0, rows = 0;
    bool operator()(int x, int y) const {
        const int cx = (x + originX) >> 4, cy = (y + originY) >> 4;
        return cx < 0 || cy < 0 || cx >= columns || cy >= rows || cells[std::size_t(cy * columns + cx)];
    }
    static Terrain random(Random& r) {
        Terrain t;
        t.columns = 12; t.rows = 12; t.originX = 6 * 16 + 8; t.originY = 6 * 16 + 8;
        t.cells.resize(std::size_t(t.columns * t.rows));
        const int fill = r.range(5, 45);
        for (std::size_t i = 0; i < t.cells.size(); ++i) t.cells[i] = r.chance(fill);
        return t;
    }
};

template<class A, class B> void sameCrawler(const A& a, const B& b) {
    require(Fixed::fixedValue(a.x) == value(b.x) && Fixed::fixedValue(a.y) == value(b.y) &&
            Fixed::fixedValue(a.vx) == value(b.vx) && Fixed::fixedValue(a.vy) == value(b.vy) &&
            a.direction == b.direction && a.imageDirection == b.imageDirection &&
            a.latched == b.latched && a.pixelX() == b.pixelX() && a.pixelY() == b.pixelY(),
            "crawler state matches the oracle");
}
void crawlers(Random& r) {
    for (int run = 0; run < 4000; ++run) {
        const auto terrain = Terrain::random(r);
        downwell::CrawlerMotion a; downwell::BasicCrawlerMotion<Pixel> b;
        const int px = r.range(-40, 40), py = r.range(-40, 40);
        a.initialize(px, py); b.initialize(px, py);
        if (r.chance(30)) {
            const long x = r.range(-60000, 60000), y = r.range(-60000, 60000);
            a.x = x; a.y = y; b.x = Pixel::fixed(x); b.y = Pixel::fixed(y);
        }
        a.latched = b.latched = r.chance(50);
        const auto meetingA = [&](long x, long y) { return downwell::CrawlerMotion::meetsTiles(x, y, terrain); };
        const auto meetingB = [&](Pixel::Number x, Pixel::Number y) {
            return downwell::BasicCrawlerMotion<Pixel>::meetsTiles(x, y, terrain);
        };
        for (int event = 0; event < 60; ++event) {
            const bool clockwise = r.chance(50), visible = r.chance(85);
            if (r.chance(50)) { a.step(clockwise, visible, meetingA); b.step(clockwise, visible, meetingB); }
            else { a.alarm(clockwise, visible, meetingA); b.alarm(clockwise, visible, meetingB); }
            sameCrawler(a, b);
            require(Fixed::truncatedQ8(a.x) == Pixel::truncatedQ8(b.x) &&
                    Fixed::truncatedQ8(a.vy) == Pixel::truncatedQ8(b.vy), "crawler record fields match");
        }
    }
}

struct Calls {
    std::vector<int> log;
    bool fail = false;
};
void snails(Random& r) {
    for (int run = 0; run < 4000; ++run) {
        const auto terrain = Terrain::random(r);
        downwell::SnailMotion a; downwell::BasicSnailMotion<Pixel> b;
        const int px = r.range(-40, 40), py = r.range(-40, 40);
        const bool descending = r.chance(50);
        a.initialize(px, py, descending); b.initialize(px, py, descending);
        Calls ca, cb;
        const int failAt = r.chance(10) ? r.range(0, 40) : -1;
        for (int event = 0; event < 80; ++event) {
            const bool stopped = r.chance(10);
            const auto transform = [&](Calls& calls) {
                return [&calls, event, failAt](downwell::RunnerTransformField field, int v) {
                    calls.log.push_back(int(field) * 4 + v + 1);
                    return event != failAt;
                };
            };
            const bool ra = a.step(terrain, stopped, transform(ca));
            const bool rb = b.step(terrain, stopped, transform(cb));
            require(ra == rb && ca.log == cb.log, "snail result and transforms match");
            require(Fixed::fixedValue(a.x) == value(b.x) && Fixed::fixedValue(a.y) == value(b.y) &&
                    Fixed::fixedValue(a.vy) == value(b.vy) && a.facing == b.facing &&
                    a.verticalFlip == b.verticalFlip && a.attached == b.attached &&
                    Fixed::roundPixel(a.y) == Pixel::roundPixel(b.y) &&
                    Fixed::truncatedQ8(a.vy) == Pixel::truncatedQ8(b.vy), "snail state matches the oracle");
            if (!ra) break;
        }
    }
}
void walkers(Random& r) {
    for (int run = 0; run < 4000; ++run) {
        const auto terrain = Terrain::random(r);
        downwell::WalkingEnemyMotion a; downwell::BasicWalkingEnemyMotion<Pixel> b;
        const int px = r.range(-40, 40), py = r.range(-40, 40);
        const long speedX = r.range(-640, 640) * 5L, speedY = r.range(-400, 400) * 5L;
        const bool ground = r.chance(50), turtle = r.chance(40);
        a.initialize(px, py, speedX, speedY, ground); b.initialize(px, py, speedX, speedY, ground);
        const int failAt = r.chance(10) ? r.range(0, 40) : -1;
        Calls ca, cb;
        const auto meetingA = [&](long x, long y) { return a.meetsTiles(x, y, turtle, terrain); };
        const auto meetingB = [&](Pixel::Number x, Pixel::Number y) { return b.meetsTiles(x, y, turtle, terrain); };
        for (int event = 0; event < 80; ++event) {
            const bool walking = r.chance(60), visible = r.chance(85);
            const auto presentation = [&](Calls& calls) {
                return [&calls, event, failAt](int sprite, int scale) {
                    calls.log.push_back(sprite * 4 + scale + 1);
                    return event != failAt;
                };
            };
            const bool ra = a.step(turtle, walking, visible, meetingA, presentation(ca));
            const bool rb = b.step(turtle, walking, visible, meetingB, presentation(cb));
            require(ra == rb && ca.log == cb.log, "walker result and presentation match");
            require(Fixed::fixedValue(a.x) == value(b.x) && Fixed::fixedValue(a.y) == value(b.y) &&
                    Fixed::fixedValue(a.vx) == value(b.vx) && Fixed::fixedValue(a.vy) == value(b.vy) &&
                    a.facing == b.facing && a.publishedX == b.publishedX && a.publishedY == b.publishedY &&
                    a.grounded == b.grounded, "walker state matches the oracle");
            require(Fixed::floorQ8(a.x) == Pixel::floorQ8(b.x) && Fixed::floorQ8(a.vy) == Pixel::floorQ8(b.vy) &&
                    Fixed::floorQ8Remainder(a.y) == Pixel::floorQ8Remainder(b.y) &&
                    Fixed::floorQ8Remainder(a.vy) == Pixel::floorQ8Remainder(b.vy), "walker record fields match");
            if (!ra) break;
        }
    }
}
}

void pixelBounds() {
    for (int mask = 0; mask < 600; ++mask)
        for (int sx : {-1, 1}) for (int sy : {-1, 1})
            for (int x = -300; x <= 300; x += 7) for (int y = -9000; y <= 9000; y += 131) {
                const auto a = downwell::contactWorldBounds({x * 256L, y * 256L, sx, sy, mask});
                const auto b = downwell::contactPixelBounds(x, y, sx, sy, mask);
                require(a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom,
                        "pixel mask boxes match their Q8 form");
            }
}

int main() {
    operations();
    pixelBounds();
    Random random{0x5eed};
    crawlers(random);
    snails(random);
    walkers(random);
    std::cout << "Pixel enemy motion matches its fixed-point oracle\n";
}
