#ifndef DOWNWELL_CORE_TITLE_PARTICLE_RANDOM_H
#define DOWNWELL_CORE_TITLE_PARTICLE_RANDOM_H
namespace downwell {
// objTitle Alarm 0 / Step 0: particle rendering is currently omitted, but
// argument evaluation must advance the shared stream in VM order. The VM
// evaluates emitMovingFx arguments right-to-left; parentMovingFx Create has
// no random calls. Range offsets do not affect the consumed words.
template<class Random>
void consumeTitleBurstRandom(Random& random) {
    for (unsigned particle = 0; particle < 20; ++particle) {
        random.random(0.1); // movement speed
        random.random(0.165); // animation speed
        random.chooseIndexSmall(2); // sprite
        random.random(32.0); // y
        random.random(140.0); // x
    }
}
template<class Random>
void consumeTitleRiseRandom(Random& random) {
    if (random.chooseIndexSmall(2)) {
        random.random(0.4); // animation speed
        random.chooseIndexSmall(2); // sprite
        random.random(32.0); // y
        random.random(132.0); // x
    }
}
}
#endif
