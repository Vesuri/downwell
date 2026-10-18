#include "platform/sdl/PlatformSDL.h"
#include "core/ShopWorld.h"
#include "core/Replay.h"
#include <cassert>
#include <cstdio>
int main(){
    downwell::PlatformSDL platform;assert(platform.ready());
    assert(!platform.pollInput().cancel);
    auto key=[](Uint32 type,SDL_Keycode code){
        SDL_Event e{};e.type=type;e.key.keysym.sym=code;
        assert(SDL_PushEvent(&e)==1);
    };
    key(SDL_KEYDOWN,SDLK_x);
    assert(platform.pollInput().cancel&&platform.pollInput().cancel);
    key(SDL_KEYUP,SDLK_x);assert(!platform.pollInput().cancel);
    key(SDL_KEYDOWN,SDLK_x);key(SDL_KEYUP,SDLK_x);
    assert(platform.pollInput().cancel&&!platform.pollInput().cancel);
    key(SDL_KEYDOWN,SDLK_ESCAPE);
    auto escape=platform.pollInput();assert(escape.quit&&!escape.cancel);
    key(SDL_KEYDOWN,SDLK_x);assert(platform.pollInput().cancel);
    SDL_Event focus{};focus.type=SDL_WINDOWEVENT;
    focus.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
    assert(SDL_PushEvent(&focus)==1);assert(!platform.pollInput().cancel);
    const int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    assert(index>=0&&SDL_IsGameController(index));
    SDL_Joystick* joystick=SDL_JoystickOpen(index);assert(joystick);
    downwell::ShopWorld world;
    assert(!platform.pollInput().cancel);
    assert(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_B,1)==0);
    auto input=platform.pollInput();assert(input.cancel&&!input.fire&&!input.pause);
    world.publishInput(downwell::unpackReplayInput(downwell::packReplayInput(input)));
    assert(world.input.cancel);world.input.cancel=false;
    input=platform.pollInput();assert(input.cancel);
    world.publishInput(input);assert(!world.input.cancel);
    assert(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_B,0)==0);
    world.publishInput(platform.pollInput());assert(!world.input.cancel);
    assert(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_B,1)==0);
    world.publishInput(platform.pollInput());assert(world.input.cancel);
    SDL_JoystickClose(joystick);
    assert(SDL_JoystickDetachVirtual(index)==0);
    input=platform.pollInput();assert(!input.cancel);
    world.publishInput(input);assert(!world.input.cancel&&!world.previous.cancel);
    std::puts("SDL controller Cancel: hotplug, held edge, consumption, replay and detach passed");
}
