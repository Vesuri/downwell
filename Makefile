# Downwell host reference build. Shared logic is kept free of SDL dependencies;
# only src/platform/sdl and main.cpp include SDL headers.

CXX := clang++

SDL_CFLAGS  := $(shell pkg-config --cflags sdl2 SDL2_image)
SDL_LDFLAGS := $(shell pkg-config --libs sdl2 SDL2_image)

CPPFLAGS := -Isrc $(SDL_CFLAGS)
CXXFLAGS := -std=c++17 -g -O0 -Wall -Wextra -Wpedantic -MMD -MP
LDFLAGS  := $(SDL_LDFLAGS)

SOURCES := \
	src/core/AreaOneBuilder.cpp \
	src/core/BuilderPatternPack.cpp \
	src/core/Game.cpp \
	src/core/GameMakerRandom.cpp \
	src/core/HudNumbers.cpp \
	src/core/FontPack.cpp \
	src/core/ObjectPack.cpp \
	src/core/Replay.cpp \
	src/core/RoomPack.cpp \
	src/core/SpritePack.cpp \
	src/core/StartupRuntime.cpp \
	src/platform/sdl/PlatformSDL.cpp \
	src/main.cpp

OBJECTS := $(patsubst %.cpp,build/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)
TARGET  := build/downwell-sdl
STARTUP_TEST_TARGET := build/startup-runtime-test
STARTUP_TEST_SOURCES := tests/core/StartupRuntimeTest.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
REPLAY_TEST_TARGET := build/replay-test
REPLAY_TEST_SOURCES := tests/core/ReplayTest.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
ALARM_TEST_TARGET := build/alarm-bank-test
ALARM_TEST_SOURCES := tests/core/AlarmBankTest.cpp
INSTANCE_TEST_TARGET := build/instance-store-test
INSTANCE_TEST_SOURCES := tests/core/InstanceStoreTest.cpp
EVENT_TEST_TARGET := build/event-scheduler-test
EVENT_TEST_SOURCES := tests/core/EventSchedulerTest.cpp
ROOM_TEST_TARGET := build/room-loader-test
ROOM_TEST_SOURCES := tests/core/RoomLoaderTest.cpp
ROOM_PACK_TEST_TARGET := build/room-pack-test
ROOM_PACK_TEST_SOURCES := tests/core/RoomPackTest.cpp src/core/RoomPack.cpp
OBJECT_PACK_TEST_TARGET := build/object-pack-test
OBJECT_PACK_TEST_SOURCES := tests/core/ObjectPackTest.cpp src/core/ObjectPack.cpp
SPRITE_PACK_TEST_TARGET := build/sprite-pack-test
SPRITE_PACK_TEST_SOURCES := tests/core/SpritePackTest.cpp src/core/SpritePack.cpp
FONT_PACK_TEST_TARGET := build/font-pack-test
FONT_PACK_TEST_SOURCES := tests/core/FontPackTest.cpp src/core/FontPack.cpp
BUILDER_PATTERN_PACK_TEST_TARGET := build/builder-pattern-pack-test
BUILDER_PATTERN_PACK_TEST_SOURCES := tests/core/BuilderPatternPackTest.cpp src/core/BuilderPatternPack.cpp
DEFERRED_INSTANCE_TEST_TARGET := build/deferred-instance-queue-test
DEFERRED_INSTANCE_TEST_SOURCES := tests/core/DeferredInstanceQueueTest.cpp src/core/ObjectPack.cpp
RANDOM_TEST_TARGET := build/game-maker-random-test
RANDOM_TEST_SOURCES := tests/core/GameMakerRandomTest.cpp src/core/GameMakerRandom.cpp
PLAYER_COLLISION_TEST_TARGET := build/player-collision-test
PLAYER_COLLISION_TEST_SOURCES := tests/core/PlayerCollisionTest.cpp
GAME_RENDER_TEST_TARGET := build/game-render-test
GAME_RENDER_TEST_SOURCES := tests/core/GameRenderTest.cpp src/core/Game.cpp src/core/HudNumbers.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
HUD_NUMBER_TEST_TARGET := build/hud-number-test
HUD_NUMBER_TEST_SOURCES := tests/core/HudNumbersTest.cpp src/core/HudNumbers.cpp
AMIGA_PLAYER_TEST_TARGET := build/amiga-player-test
AMIGA_PLAYER_TEST_SOURCES := tests/platform/amiga/AmigaPlayerTest.cpp src/platform/amiga/AmigaPlayer.cpp
AMIGA_PORT_TEST_TARGET := build/amiga-port-runtime-test
AMIGA_PORT_TEST_SOURCES := tests/platform/amiga/AmigaPortRuntimeTest.cpp src/platform/amiga/AmigaPortRuntime.cpp src/platform/amiga/AmigaPlayer.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp
AMIGA_SCENE_TEST_TARGET := build/amiga-scene-runtime-test
AMIGA_SCENE_TEST_SOURCES := tests/platform/amiga/AmigaSceneRuntimeTest.cpp src/platform/amiga/AmigaSceneRuntime.cpp src/platform/amiga/AmigaTerrain.cpp src/platform/amiga/AmigaPortRuntime.cpp src/platform/amiga/AmigaPlayer.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
AMIGA_MENU_TEST_TARGET := build/amiga-menu-test
AMIGA_MENU_TEST_SOURCES := tests/platform/amiga/AmigaMenuTest.cpp src/platform/amiga/AmigaMenu.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
AMIGA_MENU_SCROLL_TEST_TARGET := build/amiga-menu-scroll-test
AMIGA_MENU_SCROLL_TEST_SOURCES := tests/platform/amiga/AmigaMenuScrollTest.cpp src/platform/amiga/AmigaMenuScroll.cpp
AMIGA_RENDERER_TEST_TARGET := build/amiga-renderer-test
AMIGA_RENDERER_TEST_SOURCES := tests/platform/amiga/AmigaRendererTest.cpp src/platform/amiga/AmigaRenderer.c
PORT_REPLAY_REFERENCE_TARGET := build/port-replay-reference
PORT_REPLAY_REFERENCE_SOURCES := tools/port-replay-reference.cpp src/platform/amiga/AmigaPortRuntime.cpp src/platform/amiga/AmigaPlayer.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp
AMIGA_STYLE_FRAME_SHA256 := b30993c1fc9519c33c41c6197d8aaf51574ec894e260808bcd335f465ac6a017
AMIGA_SPLASH_FRAME_SHA256 := b82d3f72e70a9788a16bdeb5f681fc675d831562d407bfeac317f4508d67d9f2
AMIGA_MENU_WAKE_FRAME_SHA256 := 6c6630c652c744f6dace85395b1094f3bd990b71e11119a135950bfac11c6987
AREA_ONE_TRACE_TARGET := build/trace-area-one
AREA_ONE_TRACE_SOURCES := tools/trace-area-one.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
TEST_TARGETS := $(STARTUP_TEST_TARGET) $(REPLAY_TEST_TARGET) $(ALARM_TEST_TARGET) $(INSTANCE_TEST_TARGET) $(EVENT_TEST_TARGET) $(ROOM_TEST_TARGET) $(ROOM_PACK_TEST_TARGET) $(OBJECT_PACK_TEST_TARGET) $(SPRITE_PACK_TEST_TARGET) $(FONT_PACK_TEST_TARGET) $(BUILDER_PATTERN_PACK_TEST_TARGET) $(DEFERRED_INSTANCE_TEST_TARGET) $(RANDOM_TEST_TARGET) $(PLAYER_COLLISION_TEST_TARGET) $(GAME_RENDER_TEST_TARGET) $(HUD_NUMBER_TEST_TARGET) $(AMIGA_PLAYER_TEST_TARGET) $(AMIGA_PORT_TEST_TARGET) $(AMIGA_SCENE_TEST_TARGET) $(AMIGA_MENU_TEST_TARGET) $(AMIGA_MENU_SCROLL_TEST_TARGET) $(AMIGA_RENDERER_TEST_TARGET)

.PHONY: all run test trace-area-one extract verify-extraction generate-room-pack verify-room-pack generate-object-pack verify-object-pack generate-sprite-pack verify-sprite-pack generate-font-pack verify-font-pack generate-builder-pattern-pack verify-builder-pattern-pack generate-amiga-hud verify-amiga-hud generate-amiga-game verify-amiga-game verify-native-data verify-amiga-startup verify-amiga-menu-title verify-amiga-menu-entry verify-amiga-menu-wake-frame verify-amiga-splash-frame verify-amiga-style-frame verify-amiga-scenes verify-cross-replay amiga clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

run: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack generate-builder-pattern-pack $(TARGET)
	./$(TARGET)

$(STARTUP_TEST_TARGET): $(STARTUP_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(STARTUP_TEST_SOURCES)

$(REPLAY_TEST_TARGET): $(REPLAY_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(REPLAY_TEST_SOURCES)

$(ALARM_TEST_TARGET): $(ALARM_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(ALARM_TEST_SOURCES)

$(INSTANCE_TEST_TARGET): $(INSTANCE_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(INSTANCE_TEST_SOURCES)

$(EVENT_TEST_TARGET): $(EVENT_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(EVENT_TEST_SOURCES)

$(ROOM_TEST_TARGET): $(ROOM_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(ROOM_TEST_SOURCES)

$(ROOM_PACK_TEST_TARGET): $(ROOM_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(ROOM_PACK_TEST_SOURCES)

$(OBJECT_PACK_TEST_TARGET): $(OBJECT_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(OBJECT_PACK_TEST_SOURCES)

$(SPRITE_PACK_TEST_TARGET): $(SPRITE_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(SPRITE_PACK_TEST_SOURCES)

$(FONT_PACK_TEST_TARGET): $(FONT_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(FONT_PACK_TEST_SOURCES)

$(BUILDER_PATTERN_PACK_TEST_TARGET): $(BUILDER_PATTERN_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(BUILDER_PATTERN_PACK_TEST_SOURCES)

$(DEFERRED_INSTANCE_TEST_TARGET): $(DEFERRED_INSTANCE_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(DEFERRED_INSTANCE_TEST_SOURCES)

$(RANDOM_TEST_TARGET): $(RANDOM_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(RANDOM_TEST_SOURCES)

$(PLAYER_COLLISION_TEST_TARGET): $(PLAYER_COLLISION_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(PLAYER_COLLISION_TEST_SOURCES)

$(GAME_RENDER_TEST_TARGET): $(GAME_RENDER_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(GAME_RENDER_TEST_SOURCES)

$(HUD_NUMBER_TEST_TARGET): $(HUD_NUMBER_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(HUD_NUMBER_TEST_SOURCES)

$(AMIGA_PLAYER_TEST_TARGET): $(AMIGA_PLAYER_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_PLAYER_TEST_SOURCES)

$(AMIGA_PORT_TEST_TARGET): $(AMIGA_PORT_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_PORT_TEST_SOURCES)

$(AMIGA_SCENE_TEST_TARGET): $(AMIGA_SCENE_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_SCENE_TEST_SOURCES)

$(AMIGA_MENU_TEST_TARGET): $(AMIGA_MENU_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_MENU_TEST_SOURCES)

$(AMIGA_MENU_SCROLL_TEST_TARGET): $(AMIGA_MENU_SCROLL_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_MENU_SCROLL_TEST_SOURCES)

$(AMIGA_RENDERER_TEST_TARGET): $(AMIGA_RENDERER_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(AMIGA_RENDERER_TEST_SOURCES)

$(PORT_REPLAY_REFERENCE_TARGET): $(PORT_REPLAY_REFERENCE_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(PORT_REPLAY_REFERENCE_SOURCES)

$(AREA_ONE_TRACE_TARGET): $(AREA_ONE_TRACE_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(filter-out -O0,$(CXXFLAGS)) -O2 \
		-DDOWNWELL_TRACE_AREA_ONE -o $@ $(AREA_ONE_TRACE_SOURCES)

trace-area-one: generate-room-pack generate-object-pack generate-builder-pattern-pack $(AREA_ONE_TRACE_TARGET)
	./$(AREA_ONE_TRACE_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

test: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack generate-builder-pattern-pack generate-amiga-game $(TEST_TARGETS)
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	./$(REPLAY_TEST_TARGET)
	./$(ALARM_TEST_TARGET)
	./$(INSTANCE_TEST_TARGET)
	./$(EVENT_TEST_TARGET)
	./$(ROOM_TEST_TARGET)
	./$(ROOM_PACK_TEST_TARGET) generated/downwell-rooms.dwrm
	./$(OBJECT_PACK_TEST_TARGET) generated/downwell-objects.dwob
	./$(SPRITE_PACK_TEST_TARGET) generated/downwell-sprites.dwsp
	./$(FONT_PACK_TEST_TARGET) generated/downwell-fonts.dwfn
	./$(BUILDER_PATTERN_PACK_TEST_TARGET) generated/downwell-builder-patterns.dwbp
	./$(DEFERRED_INSTANCE_TEST_TARGET)
	./$(RANDOM_TEST_TARGET)
	./$(PLAYER_COLLISION_TEST_TARGET)
	./$(GAME_RENDER_TEST_TARGET)
	./$(HUD_NUMBER_TEST_TARGET)
	./$(AMIGA_PLAYER_TEST_TARGET)
	./$(AMIGA_PORT_TEST_TARGET)
	./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	./$(AMIGA_MENU_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob
	./$(AMIGA_MENU_SCROLL_TEST_TARGET)
	./$(AMIGA_RENDERER_TEST_TARGET) generated/amiga-game.dwap

extract:
	tools/extract.sh
	tools/verify-extraction.sh

verify-extraction:
	tools/verify-extraction.sh

generate-room-pack:
	tools/generate-room-pack.py

verify-room-pack: $(ROOM_PACK_TEST_TARGET)
	tools/generate-room-pack.py --check
	./$(ROOM_PACK_TEST_TARGET) generated/downwell-rooms.dwrm
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm

generate-object-pack:
	tools/generate-object-pack.py

verify-object-pack: $(OBJECT_PACK_TEST_TARGET)
	tools/generate-object-pack.py --check
	./$(OBJECT_PACK_TEST_TARGET) generated/downwell-objects.dwob

generate-sprite-pack:
	tools/generate-sprite-pack.py

verify-sprite-pack: $(SPRITE_PACK_TEST_TARGET)
	tools/generate-sprite-pack.py --check
	./$(SPRITE_PACK_TEST_TARGET) generated/downwell-sprites.dwsp

generate-font-pack:
	tools/generate-font-pack.py

verify-font-pack: $(FONT_PACK_TEST_TARGET)
	tools/generate-font-pack.py --check
	./$(FONT_PACK_TEST_TARGET) generated/downwell-fonts.dwfn

generate-builder-pattern-pack:
	tools/generate-builder-pattern-pack.py

verify-builder-pattern-pack: $(BUILDER_PATTERN_PACK_TEST_TARGET)
	tools/generate-builder-pattern-pack.py --check
	./$(BUILDER_PATTERN_PACK_TEST_TARGET) generated/downwell-builder-patterns.dwbp

generate-amiga-hud:
	tools/generate-amiga-hud.py

verify-amiga-hud: generate-amiga-hud
	tools/generate-amiga-hud.py --check

generate-amiga-game:
	tools/generate-amiga-hud.py --profile game

verify-amiga-game: generate-amiga-game
	tools/generate-amiga-hud.py --profile game --check

verify-native-data: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack generate-builder-pattern-pack $(STARTUP_TEST_TARGET)
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

verify-cross-replay: $(PORT_REPLAY_REFERENCE_TARGET)
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga REPLAY_VERIFY=1'
	bash -c '. amiga/env.sh && amiga/verify_replay.sh ../$(PORT_REPLAY_REFERENCE_TARGET)'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-startup:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'
	bash -c '. amiga/env.sh && amiga/verify_startup.sh'

verify-amiga-menu-title:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_menu_title.gdb READY_MARKER=DOWNWELL_MENU_TITLE_SPRITE_READY READY_DESCRIPTION="hardware-scrolled menu with seven-sprite title" amiga/verify_startup.sh'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-menu-entry:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_menu_entry.gdb READY_MARKER=DOWNWELL_MENU_ENTRY_SCROLL_READY READY_DESCRIPTION="hardware-scrolled menu entry at deep camera position" amiga/verify_startup.sh'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-menu-wake-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_menu_wake_frame.gdb READY_MARKER=DOWNWELL_MENU_WAKE_FRAME_READY READY_DESCRIPTION="hardware-scrolled waking garden frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_MENU_WAKE_FRAME_SHA256)' 'amiga/.run/menu-wake-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-menu-frame.py amiga/.run/menu-wake-frame.bin build/amiga-menu-wake-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-style-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_style_frame.gdb READY_MARKER=DOWNWELL_STYLE_FRAME_READY READY_DESCRIPTION="stable style-select planar frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_STYLE_FRAME_SHA256)' 'amiga/.run/style-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-style-frame.py amiga/.run/style-frame.bin build/amiga-style-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-splash-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_splash_frame.gdb READY_MARKER=DOWNWELL_SPLASH_FRAME_READY READY_DESCRIPTION="complete Devolver publisher planar frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_SPLASH_FRAME_SHA256)' 'amiga/.run/splash-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-style-frame.py amiga/.run/splash-frame.bin build/amiga-splash-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-scenes:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_scenes.gdb READY_MARKER=DOWNWELL_GAMEPLAY_ACTIVE READY_DESCRIPTION="complete native scene sequence through scrolling gameplay and enemy activation" amiga/verify_startup.sh'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

amiga:
	@bash -c '. amiga/env.sh && $(MAKE) -C amiga'

clean:
	$(RM) -r build
	$(MAKE) -C amiga clean

-include $(DEPS)
