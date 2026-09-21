#ifndef DOWNWELL_CORE_INPUT_H
#define DOWNWELL_CORE_INPUT_H

namespace downwell {

struct Input {
    bool quit = false;
    bool left = false;
    bool right = false;
    bool fire = false;
    bool pause = false;
};

} // namespace downwell

#endif

