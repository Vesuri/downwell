#include "core/MainCameraMotion.h"
#include "core/SideCameraLocker.h"
#include "core/StandardBulletStep.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void require(bool value, const char* text) {
    if (!value) { std::cerr << text << '\n'; std::exit(1); }
}
int main() {
    {
        struct Random {
            unsigned calls=0;
            unsigned irandomSmall(unsigned max){
                require(max==(calls%2?4u:2u),"shake requests inclusive X then Y ranges");
                ++calls;return max;
            }
        } random;
        downwell::MainCameraMotion camera;
        camera.sampleShake(random);
        require(!random.calls,"idle camera consumes no shake RNG");
        camera.requestTimeStopShake();camera.sampleShake(random);
        require(random.calls==2&&camera.pixelX()==241&&camera.pixelY()==178&&
            camera.x==240*4096L&&camera.y==176*4096L,"shake offsets published position only");
        camera.stopShake();
        require(camera.pixelX()==241,"alarm does not retroactively publish a camera Step");
        camera.sampleShake(random);
        require(random.calls==2&&camera.pixelX()==240&&camera.pixelY()==176,"next Step clears expired shake without RNG");
        camera.requestTimeStopShake();camera.sampleShake(random);
        camera.collideSideLocker(1000);
        require(camera.pixelX()==241&&camera.pixelY()==1000,"locker collision overrides shaken Y but preserves published X");
        camera.reset(240,176);
        require(!camera.shaking&&!camera.shakeAmount&&!camera.shakeX&&!camera.shakeY,"reset clears shake state");
    }
    {
        using downwell::SideCameraLocker;
        require(SideCameraLocker::createdX(159) == 80 &&
            SideCameraLocker::createdX(160) == 160 &&
            SideCameraLocker::createdX(320) == 320 &&
            SideCameraLocker::createdX(321) == 400,
            "locker Create uses strict side boundaries");
        for (int x : {80, 160, 400}) {
            const SideCameraLocker locker{x, 1024};
            for (int dx = -132; dx <= 132; ++dx)
                for (int dy = -224; dy <= 224; ++dy)
                    require(locker.overlaps(x + dx, 1024 + dy) ==
                        (dx >= -129 && dx <= 129 && dy >= -221 && dy <= 221),
                        "full camera mask intersects the locker at inclusive pixel edges");
        }
        downwell::MainCameraMotion camera;
        camera.reset(400, 1000);
        camera.collideSideLocker(1000);
        require(!camera.sideLocked, "equal target does not latch a side lock");
        camera.collideSideLocker(1024);
        require(camera.sideLocked && camera.y == 1024 * 4096L &&
            camera.targetY == camera.y && camera.x == 400 * 4096L,
            "first collision snaps vertical state without moving horizontal state");
        camera.collideSideLocker(1040);
        require(camera.y == 1024 * 4096L && camera.targetY == 1040 * 4096L,
            "another locker updates target without a second snap");
        camera.step(400 * 256L, 1500 * 256L, false, true);
        require(camera.sideLocked && camera.y > 1024 * 4096L &&
            camera.y < 1040 * 4096L, "overlap retains the lock while target eases");
        camera.step(400 * 256L, 1500 * 256L, true, false);
        require(!camera.sideLocked && camera.x == 240 * 4096L &&
            camera.y == 1524 * 4096L && camera.slowdownX == 1 && camera.slowdownY == 1,
            "release branch also runs during death and snaps to the shaft/player target");
        camera.collideSideLocker(1600);
        camera.setLevelExit(1376);
        camera.step(400 * 256L, 1500 * 256L, false, false);
        require(camera.y == 1200 * 4096L && camera.targetY == camera.y,
            "exit cap applies after side-lock release");
        camera.collideSideLocker(100);
        require(camera.y == 100 * 4096L, "Collision does not run Step's top clamp");
        camera.reset(240, 176);
        require(!camera.sideLocked, "room reset clears side-lock state");
        downwell::SideCameraLockers lockers;
        const auto empty = lockers.digest(camera);
        for (unsigned i = 0; i < lockers.capacity; ++i)
            require(lockers.create(0, 1024 + int(i) * 16), "bounded locker registration");
        const auto full = lockers.digest(camera);
        require(!lockers.create(400, 2000) && lockers.digest(camera) == full,
            "capacity failure does not partially append or change state");
        lockers.clear();
        require(lockers.digest(camera) == empty, "room reset discards all locker registrations");
    }
    {
        downwell::MainCameraMotion capped;
        capped.reset(240, 1000);
        capped.setLevelExit(1376);
        capped.step(240*256L, 2000*256L);
        require(capped.roomEnd == 1200 && capped.targetY == 1200*4096L &&
            capped.y == 1040*4096L, "exit camera cap constrains target before normal easing");
        capped.y = 1500*4096L;
        capped.step(240*256L, 2000*256L);
        require(capped.y == 1200*4096L, "overshot camera snaps to exit cap before easing");
        capped.y = 1500*4096L; capped.targetY = 1100*4096L;
        capped.step(0,0,true);
        require(capped.y == 1420*4096L,
            "current y is not independently clamped when source target is below cap");
        capped.reset(240,176);
        require(capped.roomEnd == -1, "new room resets source limit sentinel");
    }
    for (long whole : {-3L, -2L, 0L, 1L, 2L, 1456L}) {
        for (long fraction : {-2049L, -2048L, -2047L, 0L, 2047L, 2048L, 2049L}) {
            downwell::MainCameraMotion pixel;
            pixel.x = pixel.y = whole * 4096 + fraction;
            const long expected = downwell::roundGameMakerCoordinate(pixel.x / 4096.0);
            require(pixel.pixelX() == expected && pixel.pixelY() == expected,
                    "published camera coordinates use exact Q12 nearest-even rounding");
            require(pixel.x == whole * 4096 + fraction,
                    "publication does not discard fractional camera easing state");
            require(pixel.viewLeftQ12() == (expected - 80) * 4096 &&
                        pixel.viewRightQ12() == (expected + 80) * 4096 &&
                        pixel.viewTopQ12() == (expected - 142) * 4096 &&
                        pixel.viewBottomQ12() == (expected + 142) * 4096,
                    "all view edges derive from the published camera instance");
        }
    }
    downwell::MainCameraMotion camera;
    camera.y = 200L * 4096 + 2047;
    require(!downwell::standardBulletOutsideView(-42L * 4096, camera.viewTopQ12()),
            "bullet at rounded top minus 100 survives instead of using fractional easing");
    camera.y = 200L * 4096 + 2049;
    require(!downwell::standardBulletOutsideView(375L * 4096, camera.viewTopQ12()) &&
                downwell::standardBulletOutsideView(376L * 4096, camera.viewTopQ12()),
            "bullet bottom expiry uses the published view with strict boundary comparison");
    camera.reset(240, 176);
    camera.step(240 * 256L, 176 * 256L);
    require(camera.targetX == 240L * 4096 && camera.targetY == 200L * 4096 &&
            camera.slowdownX == 10 && camera.slowdownY == 5,
            "shaft uses coincident horizontal clamps and source downward bias");
    camera.step(160 * 256L - 1, 800 * 256L);
    require(camera.x == 80L * 4096 && camera.targetY == 200L * 4096 && camera.slowdownY == 5,
            "left room retains vertical target and smoothing despite player descent");
    camera.step(160 * 256L, 900 * 256L);
    require(camera.targetX == 80L * 4096 && camera.targetY == 200L * 4096,
            "exact shaft boundary changes neither target");
    camera.step(160 * 256L + 1, 300 * 256L);
    require(camera.x == 240L * 4096 && camera.y == 324L * 4096 && camera.slowdownY == 1,
            "return from side room snaps both axes with source slowdown one");
    camera.step(320 * 256L + 1, 1000 * 256L);
    require(camera.x == 400L * 4096 && camera.targetY == 324L * 4096,
            "right room also retains vertical target");
    camera.step(320 * 256L, 1100 * 256L);
    require(camera.targetX == 400L * 4096, "right equality retains prior side target");
    camera.reset(240, 176);
    camera.targetY = 400L * 4096;
    camera.step(0, 0, true);
    require(camera.targetY == 400L * 4096 && camera.y > 176L * 4096 && camera.x == 240L * 4096,
            "death retains targets but continues easing without synthetic scroll");
    camera.reset(240, 100);
    camera.step(240 * 256L, -100 * 256L);
    require(camera.y == 142L * 4096, "camera centre is clamped to source top limit");
    camera.reset(240, 176);
    double expected = 176;
    for (int i = 0; i < 1000; ++i) {
        const int playerY = 176 + i * 3;
        camera.step(240 * 256L, playerY * 256L);
        expected += (playerY + 24 - expected) / 5;
        require(std::fabs(camera.y / 4096.0 - expected) < .002,
                "Q12 camera easing stays within .002 pixels of source equations");
    }
    camera.reset(240, 299900);
    camera.step(240 * 256L, 300000 * 256L);
    require(camera.targetY == 300024L * 4096 && camera.y > 299900L * 4096 &&
            camera.y < 2147483647L, "camera covers the serialized room height in signed 32-bit state");
    std::cout << "shared main camera tests passed\n";
}
