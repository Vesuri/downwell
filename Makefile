# Downwell host reference build. Shared logic is kept free of SDL dependencies;
# only src/platform/sdl and main.cpp include SDL headers.

CXX := clang++
# Define this before any prerequisite list expands it. Several host checks
# compile multiple sources in one invocation; their .d file covers only the
# final source and cannot replace these explicit header dependencies.
HOST_CHECK_HEADERS := $(wildcard src/core/*.h src/platform/amiga/*.h) generated/clear-motion.inc
SKIP_INTRO ?= 0
SKIP_TO_GAME ?= 0
SKIP_LEVEL ?= 0
ifneq ($(filter-out 0,$(TERRAIN_RUN_STORE)),)
$(error TERRAIN_RUN_STORE was removed; use the default registered-terrain reference)
endif
REFERENCE_RANDOM ?= 0
REFERENCE_PATTERNS ?= 0
REFERENCE_BREAKABLES ?= 0
REFERENCE_GEM_POSITION ?= 0
export SKIP_INTRO SKIP_TO_GAME SKIP_LEVEL

SDL_CFLAGS  := $(shell pkg-config --cflags sdl2 SDL2_image sndfile)
SDL_LDFLAGS := $(shell pkg-config --libs sdl2 SDL2_image sndfile)

CPPFLAGS := -Isrc $(SDL_CFLAGS)
ifeq ($(SKIP_INTRO),1)
CPPFLAGS += -DDOWNWELL_SKIP_INTRO
endif
ifneq ($(filter 1,$(SKIP_TO_GAME) $(SKIP_LEVEL)),)
CPPFLAGS += -DDOWNWELL_SKIP_TO_GAME
endif
ifeq ($(SKIP_LEVEL),1)
CPPFLAGS += -DDOWNWELL_SKIP_LEVEL
endif
HOST_OPT ?= -O2
CXXFLAGS := -std=c++17 -g $(HOST_OPT) -ffp-contract=off -Wall -Wextra -Wpedantic -MMD -MP
ifeq ($(REFERENCE_RANDOM),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_RANDOM
endif
ifeq ($(REFERENCE_PATTERNS),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_PATTERNS
endif
ifeq ($(REFERENCE_BREAKABLES),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_BREAKABLES
endif
ifeq ($(REFERENCE_GEM_POSITION),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_GEM_POSITION
endif
ifeq ($(REFERENCE_CONTACT_MOTION),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_CONTACT_MOTION
endif
ifeq ($(REFERENCE_CLEAR_MOTION),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_CLEAR_MOTION
endif
ifeq ($(REFERENCE_CONTACT_COORDINATES),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_CONTACT_COORDINATES
endif
ifeq ($(REFERENCE_ENEMY_ALARM_ROSTER),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
endif
ifeq ($(REFERENCE_ENEMY_UPDATES),1)
CXXFLAGS += -DDOWNWELL_REFERENCE_ENEMY_UPDATES
endif
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
	src/core/PlayerMotion.cpp \
	src/platform/sdl/PlatformSDL.cpp \
	src/platform/sdl/AudioSDL.cpp \
	src/main.cpp

OBJECTS := $(patsubst %.cpp,build/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)
TARGET  := build/downwell-sdl
STARTUP_TEST_TARGET := build/startup-runtime-test
STARTUP_TEST_SOURCES := tests/core/StartupRuntimeTest.cpp src/core/StartupRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
REPLAY_TEST_TARGET := build/replay-test
REPLAY_TEST_SOURCES := tests/core/ReplayTest.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
ALARM_TEST_TARGET := build/alarm-bank-test
ALARM_TEST_SOURCES := tests/core/AlarmBankTest.cpp
INSTANCE_TEST_TARGET := build/instance-store-test
INSTANCE_TEST_SOURCES := tests/core/InstanceStoreTest.cpp
EVENT_TEST_TARGET := build/event-scheduler-test
EVENT_TEST_SOURCES := tests/core/EventSchedulerTest.cpp
ROOM_TEST_TARGET := build/room-loader-test
ROOM_TEST_SOURCES := tests/core/RoomLoaderTest.cpp
ROOM_PACK_TEST_TARGET := build/room-pack-test
ROOM_PACK_TEST_SOURCES := tests/core/RoomPackTest.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp
OBJECT_PACK_TEST_TARGET := build/object-pack-test
OBJECT_PACK_TEST_SOURCES := tests/core/ObjectPackTest.cpp src/core/ObjectPack.cpp
SPRITE_PACK_TEST_TARGET := build/sprite-pack-test
SPRITE_PACK_TEST_SOURCES := tests/core/SpritePackTest.cpp src/core/SpritePack.cpp src/core/ObjectPack.cpp src/core/GameMakerRandom.cpp
FONT_PACK_TEST_TARGET := build/font-pack-test
FONT_PACK_TEST_SOURCES := tests/core/FontPackTest.cpp src/core/FontPack.cpp
BUILDER_PATTERN_PACK_TEST_TARGET := build/builder-pattern-pack-test
BUILDER_PATTERN_PACK_TEST_SOURCES := tests/core/BuilderPatternPackTest.cpp src/core/BuilderPatternPack.cpp
AREA_ONE_BUILDER_TEST_TARGET := build/area-one-builder-test
AREA_ONE_BUILDER_TEST_SOURCES := tests/core/AreaOneBuilderTest.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp src/core/GameMakerRandom.cpp
AMIGA_LIVE_TERRAIN_TEST_TARGET := build/amiga-live-terrain-test
AMIGA_LIVE_TERRAIN_TEST_SOURCES := tests/platform/amiga/AmigaLiveTerrainTest.cpp src/platform/amiga/AmigaTerrain.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/Replay.cpp
DEFERRED_INSTANCE_TEST_TARGET := build/deferred-instance-queue-test
DEFERRED_INSTANCE_TEST_SOURCES := tests/core/DeferredInstanceQueueTest.cpp src/core/ObjectPack.cpp
RANDOM_TEST_TARGET := build/game-maker-random-test
RANDOM_TEST_SOURCES := tests/core/GameMakerRandomTest.cpp src/core/GameMakerRandom.cpp
PLAYER_COLLISION_TEST_TARGET := build/player-collision-test
PLAYER_COLLISION_TEST_SOURCES := tests/core/PlayerCollisionTest.cpp
PLAYER_CONTROLS_TEST_TARGET := build/player-controls-test
GAME_RENDER_TEST_TARGET := build/game-render-test

GAME_RENDER_TEST_SOURCES := tests/core/GameRenderTest.cpp src/core/Game.cpp src/core/HudNumbers.cpp src/core/StartupRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
HUD_NUMBER_TEST_TARGET := build/hud-number-test
HUD_NUMBER_TEST_SOURCES := tests/core/HudNumbersTest.cpp src/core/HudNumbers.cpp
AMIGA_PLAYER_TEST_TARGET := build/amiga-player-test
AMIGA_PLAYER_TEST_SOURCES := tests/core/PlayerMotionTest.cpp src/core/PlayerMotion.cpp
AMIGA_PORT_TEST_TARGET := build/amiga-port-runtime-test
AMIGA_PORT_TEST_SOURCES := tests/platform/amiga/AmigaPortRuntimeTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp
AMIGA_SCENE_TEST_TARGET := build/amiga-scene-runtime-test
AMIGA_SCENE_TEST_SOURCES := tests/platform/amiga/AmigaSceneRuntimeTest.cpp src/platform/amiga/AmigaSceneRuntime.cpp src/platform/amiga/AmigaGameplayGeneration.cpp src/platform/amiga/AmigaTerrain.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
AMIGA_MENU_TEST_TARGET := build/amiga-menu-test
AMIGA_MENU_TEST_SOURCES := tests/platform/amiga/AmigaMenuTest.cpp src/platform/amiga/AmigaMenu.cpp src/core/StartupRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
AMIGA_MENU_SCROLL_TEST_TARGET := build/amiga-menu-scroll-test
AMIGA_MENU_SCROLL_TEST_SOURCES := tests/platform/amiga/AmigaMenuScrollTest.cpp src/platform/amiga/AmigaMenuScroll.cpp
AMIGA_RENDERER_TEST_TARGET := build/amiga-renderer-test
AMIGA_RENDERER_TEST_SOURCES := tests/platform/amiga/AmigaRendererTest.cpp src/platform/amiga/AmigaRenderer.c
PORT_REPLAY_REFERENCE_TARGET := build/port-replay-reference
PORT_REPLAY_REFERENCE_SOURCES := tools/port-replay-reference.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp
AMIGA_STYLE_FRAME_SHA256 := b23334336ced93fbc77435a47aaec7586972bd384902e3c850988c11c25d4c8f
AMIGA_LOCKED_STYLE_FRAME_SHA256 := 0a767cebfc24ed23cb936f3ebdc6dd8359a6784a2ba475be7f10a0cb0adaf524
AMIGA_SELECTED_STYLE_FRAME_SHA256 := 2bae3a77e4f22ba81e42ba3c648c917a4227b427511d458b7345b2c564363fb9
AMIGA_SPLASH_FRAME_SHA256 := b82d3f72e70a9788a16bdeb5f681fc675d831562d407bfeac317f4508d67d9f2
AMIGA_CONTROLS_FRAME_SHA256 := ee3f455e6c9192d08f64ea26cf6b56338badf0dadeee56acf85ae2cd2838560a
AMIGA_CREDITS_FRAME_SHA256 := 9e7137cae00e1925f0a61c045f23ed1571358e9e02451b2acf9db036453d3af1
AMIGA_MENU_WAKE_FRAME_SHA256 := d616f4469720f60a3f18bdbc55a8851422b923e62ec7c75547edcce08fe8b449
AMIGA_MENU_TITLE_FRAME_SHA256 := 403f3d3586e62a35c6ba69dd622f3ad02bb1dc4bf3dc16bdad8a315984a906de
AMIGA_MENU_ENTRY_FRAME_SHA256 := 21e58d135d92be921bbfa40245e7d074505fecf0b1d7b1c0fb47f92ef0fa61fe
AMIGA_GAMEPLAY_FRAME_SHA256 := ac1ce0c2494d731bf8bca49240464b28c92d9ebbf1c3917214f18921f91203ce
AREA_ONE_TRACE_TARGET := build/trace-area-one
AREA_ONE_TRACE_SOURCES := tools/trace-area-one.cpp src/core/StartupRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
TEST_TARGETS := $(STARTUP_TEST_TARGET) $(REPLAY_TEST_TARGET) $(ALARM_TEST_TARGET) $(INSTANCE_TEST_TARGET) $(EVENT_TEST_TARGET) $(ROOM_TEST_TARGET) $(ROOM_PACK_TEST_TARGET) $(OBJECT_PACK_TEST_TARGET) $(SPRITE_PACK_TEST_TARGET) $(FONT_PACK_TEST_TARGET) $(BUILDER_PATTERN_PACK_TEST_TARGET) $(DEFERRED_INSTANCE_TEST_TARGET) $(RANDOM_TEST_TARGET) $(PLAYER_COLLISION_TEST_TARGET) $(GAME_RENDER_TEST_TARGET) $(HUD_NUMBER_TEST_TARGET) $(AMIGA_PLAYER_TEST_TARGET) $(AMIGA_PORT_TEST_TARGET) $(AMIGA_SCENE_TEST_TARGET) $(AMIGA_MENU_TEST_TARGET) $(AMIGA_MENU_SCROLL_TEST_TARGET) $(AMIGA_RENDERER_TEST_TARGET)

.PHONY: all run test trace-area-one extract verify-extraction generate-room-pack verify-room-pack generate-object-pack verify-object-pack generate-sprite-pack verify-sprite-pack generate-font-pack verify-font-pack generate-builder-pattern-pack verify-builder-pattern-pack generate-amiga-hud verify-amiga-hud generate-amiga-game verify-amiga-game verify-native-data verify-amiga-startup verify-amiga-menu-title verify-amiga-menu-entry verify-amiga-menu-wake-frame verify-amiga-menu-title-frame verify-amiga-menu-entry-frame verify-amiga-gameplay-frame verify-amiga-gameplay-active-frame verify-amiga-splash-frame verify-amiga-controls-frame verify-amiga-credits-frame verify-amiga-style-frame verify-amiga-locked-style-frame verify-amiga-selected-style-frame verify-amiga-scenes verify-cross-replay amiga clean

all: $(TARGET)

.PHONY: test-white-jelly-birth
test: test-moving-fx-instances test-moving-fx test-red-ball-birth
.PHONY: test-moving-fx-instances
test-moving-fx-instances: build/moving-fx-instances-test
	./build/moving-fx-instances-test
build/moving-fx-instances-test: tests/core/MovingFxInstancesTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: test-moving-fx
test-moving-fx: build/moving-fx-test
	./build/moving-fx-test
	python3 tools/verify-red-ball-events.py
build/moving-fx-test: tests/core/MovingFxTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test: test-red-ball-step
test: test-red-ball-events
test: test-red-ball-draw
.PHONY: test-red-ball-draw
test-red-ball-draw: build/red-ball-draw-test
	./build/red-ball-draw-test
	python3 tools/verify-red-ball-events.py
build/red-ball-draw-test: tests/core/RedBallDrawTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/RedBallDrawTest.cpp
.PHONY: test-red-ball-events
test-red-ball-events: build/red-ball-events-test
	./build/red-ball-events-test
	python3 tools/verify-red-ball-events.py
build/red-ball-events-test: tests/core/RedBallEventsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/RedBallEventsTest.cpp src/core/GameMakerRandom.cpp
.PHONY: test-red-ball-step
test-red-ball-step: build/red-ball-step-test
	./build/red-ball-step-test
	python3 tools/verify-red-ball-events.py
build/red-ball-step-test: tests/core/RedBallStepTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/RedBallStepTest.cpp src/core/GameMakerRandom.cpp
.PHONY: test-red-ball-birth
test-red-ball-birth: build/red-ball-birth-test
	./build/red-ball-birth-test
	python3 tools/verify-enemy-create.py
build/red-ball-birth-test: tests/core/RedBallBirthTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/RedBallBirthTest.cpp src/core/GameMakerRandom.cpp
build/white-jelly-birth-test build/white-jelly-step-test build/white-jelly-events-test build/gameplay-white-jellies-test build/stored-white-jelly-test: src/core/SteeringEnemyBirthState.h
test: test-gameplay-white-jellies
.PHONY: test-gameplay-white-jellies
test-gameplay-white-jellies: build/gameplay-white-jellies-test
	./build/gameplay-white-jellies-test
build/gameplay-white-jellies-test: tests/core/GameplayWhiteJelliesTest.cpp src/core/GameplayWhiteJellies.h src/core/GameplaySteeringEnemies.h src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/GameplayWhiteJelliesTest.cpp src/core/GameMakerRandom.cpp

test: test-gameplay-red-balls
.PHONY: test-gameplay-red-balls
test-gameplay-red-balls: build/gameplay-red-balls-test
	./build/gameplay-red-balls-test
build/gameplay-red-balls-test: tests/core/GameplayWhiteJelliesTest.cpp src/core/GameplayRedBalls.h src/core/GameplaySteeringEnemies.h src/core/RedBallStep.h src/core/RedBallBirth.h src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_TEST_RED_BALL -o $@ tests/core/GameplayWhiteJelliesTest.cpp src/core/GameMakerRandom.cpp

test: test-white-jelly-events
.PHONY: test-white-jelly-events
test-white-jelly-events: build/white-jelly-events-test
	./build/white-jelly-events-test
build/white-jelly-events-test: tests/core/WhiteJellyEventsTest.cpp src/core/WhiteJellyStep.h src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/WhiteJellyEventsTest.cpp src/core/GameMakerRandom.cpp

test: test-stored-white-jelly
.PHONY: test-stored-white-jelly
test-stored-white-jelly: build/stored-white-jelly-test
	./build/stored-white-jelly-test
build/stored-white-jelly-test: tests/core/StoredWhiteJellyStatesTest.cpp src/core/StoredWhiteJellyStates.h src/core/StoredSteeringEnemyStates.h src/core/StoredRedBallStates.h src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/StoredWhiteJellyStatesTest.cpp src/core/GameMakerRandom.cpp

test: test-check-collision-script
.PHONY: test-check-collision-script
test-check-collision-script: build/check-collision-script-test
	./build/check-collision-script-test
build/check-collision-script-test: tests/core/CheckCollisionWithScriptTest.cpp src/core/CheckCollisionWithScript.h src/core/WhiteJellyStep.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/CheckCollisionWithScriptTest.cpp

test: test-white-jelly-step
.PHONY: test-white-jelly-step
test-white-jelly-step: build/white-jelly-step-test
	./build/white-jelly-step-test
build/white-jelly-step-test: tests/core/WhiteJellyStepTest.cpp src/core/WhiteJellyStep.h src/core/WhiteJellyBirth.h src/core/GameMakerRandom.cpp src/core/SmokeDirection.h src/core/SmokeTrig.h generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/WhiteJellyStepTest.cpp src/core/GameMakerRandom.cpp

test: test-white-jelly-birth
test-white-jelly-birth: build/white-jelly-birth-test
	./build/white-jelly-birth-test
	python3 tools/verify-enemy-create.py
build/white-jelly-birth-test: tests/core/WhiteJellyBirthTest.cpp src/core/WhiteJellyBirth.h src/core/GameMakerRandom.cpp src/core/RandomBinary64.h src/core/MultiplyBinary64.h src/core/RunnerImageValue.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/WhiteJellyBirthTest.cpp src/core/GameMakerRandom.cpp

.PHONY: test-spawner-step
test: test-spawner-step
test-spawner-step: build/spawner-step-test
	./build/spawner-step-test
	python3 tools/verify-spawner-configuration.py build/spawner-step-test
build/spawner-step-test: tests/core/SpawnerStepTest.cpp src/core/StoredSpawnerStates.h src/core/SpawnerStep.h src/core/GameMakerRandom.cpp src/core/GameMakerRandom.h src/core/RunnerRealComparison.h src/core/MultiplyBinary64.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/SpawnerStepTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-amiga-wall-damage
test: verify-amiga-wall-damage
verify-amiga-wall-damage: build/amiga-wall-damage-test
	./build/amiga-wall-damage-test
build/amiga-wall-damage-test: tests/platform/amiga/AmigaWallDamageTest.cpp src/platform/amiga/AmigaWallDamage.h src/core/WallFootprint.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
build/gameplay-collision-owner-test: src/platform/amiga/AmigaWallDamage.h
build/gameplay-collision-owner-test build/gameplay-collision-owner-scalar-test build/gameplay-collision-owner-snapshot-test build/live-collision-lifecycle-test build/terrain-row-selection-test build/terrain-ownership-runs-test build/amiga-terrain-painter-test build/amiga-scene-runtime-test build/amiga-currency-promotion-test build/amiga-currency-promotion-reference-test: tests/core/OwnedTerrainFixture.h
build/gameplay-collision-owner-test build/stored-shop-child-test build/registered-shop-runtime-test: tests/core/ShopPurchaseChecks.h tests/core/ShopViewChecks.h tests/core/ShopMenuChecks.h tests/core/ShopParentChecks.h
build/gameplay-collision-owner-test build/stored-shop-child-test: tests/core/ShopSpeechChecks.h
build/gameplay-collision-owner-test build/live-collision-lifecycle-test: tests/core/TimeoutDamageEvents.h
build/gameplay-collision-owner-test build/live-collision-lifecycle-test: tests/core/RegisteredSkeletonStompChecks.h tests/core/SkeletonSoundChecks.h
build/gameplay-collision-owner-test build/live-collision-lifecycle-test: tests/core/BlastSkeletonChecks.h

.PHONY: verify-wall-draw-lookup
.PHONY: verify-terrain-create
test: verify-wall-end-step
.PHONY: verify-wall-end-step
verify-wall-end-step: build/wall-end-step-test
	./build/wall-end-step-test
	python3 tools/verify-wall-end-step.py build/wall-end-step-test
build/wall-end-step-test: tests/core/WallEndStepTest.cpp src/core/GameMakerRandom.cpp src/core/WallEndStep.h src/core/NativeTerrainLifetimes.h src/core/PackedWallState.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

test: verify-terrain-create
verify-terrain-create: build/terrain-create-test
	python3 tools/verify-terrain-create.py
	./build/terrain-create-test
build/terrain-create-test: tests/core/TerrainCreateTest.cpp src/core/GameMakerRandom.cpp src/core/TerrainCreate.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

verify-wall-draw-lookup: build/wall-draw-lookup-test
	./build/wall-draw-lookup-test

build/wall-draw-lookup-test: tests/core/WallDrawLookupTest.cpp src/core/WallDrawLookup.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $<

test: verify-wall-draw-lookup verify-runner-collision-phase verify-runner-collision-pairs verify-runner-collision-dispatch verify-runner-collision-context verify-runner-collision-motion

.PHONY: verify-runner-collision-motion
verify-runner-collision-motion: build/runner-collision-motion-test
	./build/runner-collision-motion-test

build/runner-collision-motion-test: tests/core/RunnerCollisionMotionTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ $<

.PHONY: verify-runner-collision-context
verify-runner-collision-context: build/runner-collision-context-test
	./build/runner-collision-context-test

build/runner-collision-context-test: tests/core/RunnerCollisionEventContextTest.cpp src/core/RunnerCollisionEventContext.h src/core/InstanceHandle.h src/core/ObjectPack.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $<

.PHONY: verify-runner-collision-dispatch
verify-runner-collision-dispatch: build/runner-collision-dispatch-test
	./build/runner-collision-dispatch-test

build/runner-collision-dispatch-test: tests/core/RunnerCollisionDispatchTest.cpp src/core/RunnerCollisionDispatch.h src/core/InstanceHandle.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $<

.PHONY: verify-runner-collision-pairs
test: test-prepared-collision-events
.PHONY: test-prepared-collision-events
test-prepared-collision-events: build/prepared-collision-events-test
	./build/prepared-collision-events-test
build/prepared-collision-events-test: tests/core/PreparedCollisionEventsTest.cpp src/core/ObjectPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
verify-runner-collision-pairs: build/runner-collision-pairs-test
	./build/runner-collision-pairs-test

build/runner-collision-pairs-test: tests/core/RunnerCollisionPairsTest.cpp src/core/RunnerCollisionPairs.h src/core/InstanceHandle.h src/core/SlotIndex.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $<

.PHONY: verify-runner-collision-phase
verify-runner-collision-phase:
	python3 tools/verify-runner-collision-phase.py

build/state-digest-test: tests/core/StateDigestTest.cpp src/core/StateDigest.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $<

.PHONY: verify-state-digest
verify-state-digest: build/state-digest-test
	./build/state-digest-test

test: verify-state-digest

.PHONY: generate-area-two-patterns verify-area-two-patterns
.PHONY: verify-catacomb-symbols
.PHONY: verify-skeleton-throw
.PHONY: verify-skeleton-timing
.PHONY: verify-runner-image-value
.PHONY: verify-legacy-image-animation
.PHONY: verify-sequence-timeline
.PHONY: verify-unit-sequence-frames
.PHONY: verify-skeleton-animation
.PHONY: verify-bone-store
verify-bone-store: build/bone-store-test
	./build/bone-store-test

build/bone-store-test: tests/core/BoneProjectileStoreTest.cpp src/core/BoneProjectileStore.h src/core/BoneProjectile.h src/core/SkeletonThrow.h src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ $< src/core/GameMakerRandom.cpp

.PHONY: verify-skeleton-actor
test: verify-flash-effects
test: verify-smoke-birth
test: verify-debris-birth
test: verify-debris-motion
test: verify-debris-animation
test: verify-debris-effects
test: verify-debris-draw
test: verify-dead-body-creation
test: verify-corpse-birth
test: verify-currency-create
.PHONY: verify-currency-create
verify-currency-create: build/currency-create-test build/currency-spawn-script-test build/currency-homing-test build/currency-free-motion-test build/currency-gain-test
	./build/currency-gain-test
	./build/currency-free-motion-test
	python3 tools/verify-runner-integer-range.py
	./build/currency-homing-test
	python3 tools/verify-currency-create.py
	python3 tools/verify-runner-move-snap.py
	python3 tools/verify-runner-alarms.py
	./build/currency-create-test
	./build/currency-spawn-script-test

build/currency-gain-test: tests/core/CurrencyGainTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/currency-free-motion-test: tests/core/CurrencyFreeMotionTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/currency-homing-test: tests/core/CurrencyHomingTest.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/currency-spawn-script-test: tests/core/CurrencySpawnScriptTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/currency-create-test: tests/core/CurrencyCreateTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test: verify-corpse-lifecycle
test: verify-corpse-motion
test: verify-out-of-view-lifetime
test: verify-corpse-creation-integration
test: verify-corpse-animation
test: verify-corpse-effects
test: verify-corpse-replay
test: verify-runner-fp-startup

.PHONY: verify-runner-fp-startup
verify-runner-fp-startup:
	python3 tools/verify-runner-fp-startup.py
test: verify-corpse-expiry
test: verify-corpse-expiry-owner

.PHONY: verify-corpse-expiry-owner
verify-corpse-expiry-owner: build/corpse-expiry-owner-test
	./build/corpse-expiry-owner-test

build/corpse-expiry-owner-test: tests/core/CorpseExpiryEffectsTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: verify-corpse-expiry
verify-corpse-expiry: build/corpse-expiry-test
	python3 tools/verify-corpse-expiry.py
	python3 tools/verify-animation-metadata.py
	./build/corpse-expiry-test

build/corpse-expiry-test: tests/core/CorpseExpiryEffectTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ $<

.PHONY: verify-corpse-replay
verify-corpse-replay: build/corpse-verification-test
	./build/corpse-verification-test

build/corpse-verification-test: tests/core/CorpseVerificationTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/CorpseVerificationTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-corpse-effects
verify-corpse-effects: build/corpse-effects-test
	./build/corpse-effects-test

build/corpse-effects-test: tests/core/CorpseEffectsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/CorpseEffectsTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-corpse-animation
test: verify-currency-animation
.PHONY: verify-currency-animation
verify-currency-animation: build/currency-animation-contract-test
	python3 tools/verify-currency-create.py
	python3 tools/verify-animation-metadata.py
	python3 tools/verify-runner-image-fields.py
	./build/currency-animation-contract-test

build/currency-animation-contract-test: tests/core/CurrencyAnimationContractTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/CurrencyAnimationContractTest.cpp

verify-corpse-animation: build/corpse-animation-test
	python3 tools/verify-animation-metadata.py
	./build/corpse-animation-test

build/corpse-animation-test: tests/core/CorpseAnimationTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/CorpseAnimationTest.cpp

.PHONY: verify-corpse-creation-integration
verify-corpse-creation-integration: build/corpse-creation-integration-test
	python3 tools/verify-dead-body-creation.py
	python3 tools/verify-debris-create.py
	./build/corpse-creation-integration-test

build/corpse-creation-integration-test: tests/core/CorpseCreationIntegrationTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/CorpseCreationIntegrationTest.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp

.PHONY: verify-out-of-view-lifetime
verify-out-of-view-lifetime: build/out-of-view-lifetime-test
	python3 tools/verify-dead-body-creation.py
	python3 tools/verify-main-room-lifetime.py
	./build/out-of-view-lifetime-test

build/out-of-view-lifetime-test: tests/core/OutOfViewLifetimeTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/OutOfViewLifetimeTest.cpp

.PHONY: verify-corpse-motion
verify-corpse-motion: build/corpse-motion-test
	python3 tools/verify-dead-body-creation.py
	./build/corpse-motion-test

build/corpse-motion-test: tests/core/CorpseMotionTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/CorpseMotionTest.cpp

.PHONY: verify-corpse-lifecycle
test: verify-consumption-blood
.PHONY: verify-consumption-blood
verify-consumption-blood: build/consumption-blood-test
	python3 tools/verify-consumption-blood.py
	./build/consumption-blood-test

build/consumption-blood-test: tests/core/ConsumptionBloodEffectTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/ConsumptionBloodEffectTest.cpp src/core/GameMakerRandom.cpp

verify-corpse-lifecycle: build/corpse-lifecycle-test
	python3 tools/verify-dead-body-creation.py
	./build/corpse-lifecycle-test

build/corpse-lifecycle-test: tests/core/CorpseLifecycleTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/CorpseLifecycleTest.cpp

.PHONY: verify-corpse-birth
verify-corpse-birth: build/corpse-birth-test
	python3 tools/verify-dead-body-creation.py
	python3 tools/verify-runner-move-snap.py
	./build/corpse-birth-test

build/corpse-birth-test: tests/core/CorpseBirthTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/CorpseBirthTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-dead-body-creation
verify-dead-body-creation: build/dead-body-creation-test
	python3 tools/verify-dead-body-creation.py
	./build/dead-body-creation-test

build/dead-body-creation-test: tests/core/DeadBodyCreationTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/DeadBodyCreationTest.cpp

.PHONY: verify-debris-draw
.PHONY: verify-runner-depth-changes
verify-runner-depth-changes: build/runner-layer-elements-test build/runner-layers-test build/runner-depth-changes-test build/runner-layer-draw-test build/runner-layer-instances-test
	python3 tools/verify-runner-depth-changes.py
	./build/runner-layer-elements-test
	./build/runner-layers-test
	./build/runner-depth-changes-test
	./build/runner-layer-draw-test
	./build/runner-layer-instances-test
test: verify-runner-depth-changes

build/runner-layer-elements-test: tests/core/RunnerLayerElementsTest.cpp src/core/RunnerLayerElements.h src/core/LowestFreeSlotMap.h src/core/SlotIndex.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

build/runner-layers-test: tests/core/RunnerLayersTest.cpp src/core/RunnerLayers.h src/core/RunnerLayerElements.h src/core/LowestFreeSlotMap.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

build/runner-depth-changes-test: tests/core/RunnerDepthChangesTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

build/runner-layer-draw-test: tests/core/RunnerLayerDrawTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: test-amiga-memory-copy
test: test-amiga-memory-copy
test-amiga-memory-copy: build/amiga-memory-copy-test
	./build/amiga-memory-copy-test

build/amiga-memory-copy-test: tests/platform/amiga/AmigaMemoryCopyTest.cpp src/platform/amiga/AmigaMemoryCopy.h src/platform/amiga/AmigaMemoryCopyVerification.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

build/runner-layer-instances-test: tests/core/RunnerLayerInstancesTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

verify-debris-draw: build/debris-draw-test
	python3 tools/verify-debris-create.py
	python3 tools/verify-dead-body-creation.py
	python3 tools/verify-effect-depths.py
	./build/debris-draw-test

build/debris-draw-test: tests/core/DebrisDrawTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/DebrisDrawTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-debris-effects
verify-debris-effects: build/debris-effects-test build/debris-registered-test build/debris-registry-test
	./build/debris-effects-test
	./build/debris-registered-test
	./build/debris-registry-test
build/debris-registry-test: tests/core/GameplayDebrisInstancesTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/GameplayDebrisInstancesTest.cpp src/core/GameMakerRandom.cpp
build/debris-registered-test: tests/core/DebrisRegisteredLifecycleTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/DebrisRegisteredLifecycleTest.cpp src/core/GameMakerRandom.cpp

build/debris-effects-test: tests/core/DebrisEffectsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/DebrisEffectsTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-debris-animation
verify-debris-animation: build/debris-animation-test
	python3 tools/verify-runner-image-fields.py
	python3 tools/verify-animation-metadata.py
	./build/debris-animation-test

build/debris-animation-test: tests/core/DebrisAnimationTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/DebrisAnimationTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-debris-motion
verify-debris-motion: build/debris-motion-test
	python3 tools/verify-debris-create.py
	python3 tools/verify-runner-integer-range.py
	./build/debris-motion-test

build/debris-motion-test: tests/core/DebrisMotionTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -ffp-contract=off -o $@ tests/core/DebrisMotionTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-debris-birth
verify-debris-birth: build/debris-birth-test
	python3 tools/verify-debris-create.py
	./build/debris-birth-test

build/debris-birth-test: tests/core/DebrisBirthTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/DebrisBirthTest.cpp src/core/GameMakerRandom.cpp
test: verify-smoke-motion
test: verify-smoke-effect
test: verify-smoke-distance
test: verify-smoke-direction
test: verify-smoke-trig
test: verify-smoke-owner
test: verify-smoke-world
test: verify-damage-number-worlds
.PHONY: verify-damage-number-worlds
test: verify-enemy-death-worlds
test: verify-level-number
.PHONY: verify-level-number
verify-level-number: build/level-number-test
	./build/level-number-test
	python3 tools/verify-runner-text-draw.py
generated/downwell-title-glyphs.h: tools/generate-title-glyphs.py extracted/utmt-0.9.2.0/manifest.json $(wildcard extracted/utmt-0.9.2.0/TexturePages/*.png)
	python3 tools/generate-title-glyphs.py
generated/downwell-area-titles.h: tools/generate-area-titles.py data/language.ini
	python3 tools/generate-area-titles.py
generated/downwell-shop-text.h: tools/generate-shop-text.py data/language.ini
	python3 tools/generate-shop-text.py
build/gameplay-collision-owner-test build/registered-shop-runtime-test: generated/downwell-shop-text.h
build/shop-text-test: tests/core/ShopTextTest.cpp src/core/ShopText.h src/core/ShopInventory.h generated/downwell-shop-text.h
	@mkdir -p build
	$(CXX) -Isrc -Igenerated $(CXXFLAGS) -o $@ $<
.PHONY: test-shop-text
build/shop-parent-draw-test: tests/core/ShopParentDrawTest.cpp tests/core/ShopParentDrawChecks.h $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $<
.PHONY: test-shop-parent-draw
test-shop-parent-draw: build/shop-parent-draw-test
	./build/shop-parent-draw-test
test: test-shop-parent-draw
test-shop-text: build/shop-text-test
	python3 tests/test_generate_shop_text.py
	./build/shop-text-test
test: test-shop-text
build/level-number-test: tests/core/LevelNumberTest.cpp src/core/FontPack.cpp generated/downwell-area-titles.h generated/downwell-title-glyphs.h generated/downwell-fonts.dwfn $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< src/core/FontPack.cpp
$(GAME_RENDER_TEST_TARGET): generated/downwell-title-glyphs.h
build/amiga-gameplay-draw-list-test build/amiga-gameplay-draw-list-replay-test: generated/downwell-title-glyphs.h
.PHONY: verify-enemy-death-worlds
test: verify-run-controller-instances
.PHONY: verify-run-controller-instances
verify-run-controller-instances: build/run-controller-instances-test
	./build/run-controller-instances-test
build/run-controller-instances-test: tests/core/RunControllerInstancesTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $<
verify-enemy-death-worlds: build/enemy-death-worlds-test generate-room-pack generate-object-pack generate-sprite-pack generate-precise-masks
	python3 tools/verify-enemy-death.py
	./build/enemy-death-worlds-test
build/enemy-death-worlds-test: tests/core/EnemyDeathWorldsTest.cpp $(filter-out tests/core/StartupRuntimeTest.cpp,$(STARTUP_TEST_SOURCES)) src/core/GameplayWorld.cpp src/core/Replay.cpp src/core/FontPack.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc generated/downwell-area-titles.h generated/downwell-fonts.dwfn
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ $(filter %.cpp,$^)
verify-damage-number-worlds: build/damage-number-worlds-test generate-room-pack generate-object-pack generate-builder-pattern-pack
	./build/damage-number-worlds-test
build/damage-number-worlds-test: tests/core/DamageNumberWorldsTest.cpp $(filter-out tests/core/StartupRuntimeTest.cpp,$(STARTUP_TEST_SOURCES)) src/core/GameplayWorld.cpp src/core/Replay.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ $(filter %.cpp,$^)

.PHONY: verify-smoke-world
verify-smoke-world: build/smoke-world-test generate-room-pack generate-object-pack generate-builder-pattern-pack
	./build/smoke-world-test
build/smoke-world-test: tests/core/SmokeWorldTest.cpp $(filter-out tests/core/StartupRuntimeTest.cpp,$(STARTUP_TEST_SOURCES)) src/core/GameplayWorld.cpp src/core/Replay.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -ffp-contract=off -Isrc -o $@ $(filter %.cpp,$^)
test: verify-effect-placement
test: verify-binary-effects
.PHONY: verify-binary-effects
build/palette-effects-test: tests/core/PaletteEffectSurfaceTest.cpp src/core/PaletteEffectSurface.h src/core/EffectMaskAssets.h src/core/BinaryEffectSurface.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $<
.PHONY: verify-palette-effects
verify-palette-effects: build/palette-effects-test generate-amiga-game
	./build/palette-effects-test generated/amiga-game.dwap
	./build/palette-effects-test generated/amiga-game.dwap interleaved
test: verify-palette-effects

build/smoke-composition-test: tests/core/SmokeCompositionTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $(filter %.cpp,$^)
.PHONY: verify-smoke-composition
build/smoke-composition-probes-test: tests/core/SmokeCompositionTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -DDOWNWELL_PROBES -Isrc -o $@ $(filter %.cpp,$^)
verify-smoke-composition: build/smoke-composition-test build/smoke-composition-probes-test generate-amiga-game
	./build/smoke-composition-test generated/amiga-game.dwap
	./build/smoke-composition-probes-test generated/amiga-game.dwap
test: verify-smoke-composition

verify-binary-effects: build/binary-effects-test generate-amiga-game
	./build/binary-effects-test generated/amiga-game.dwap

build/binary-effects-test: tests/core/BinaryEffectSurfaceTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ $<

.PHONY: verify-effect-placement
verify-effect-placement: build/effect-placement-test
	./build/effect-placement-test

build/effect-placement-test: tests/core/EffectSurfacePlacementTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ $<

.PHONY: verify-smoke-owner
build/terrain-wall-contacts-test: tests/core/TerrainWallContactsTest.cpp src/core/TerrainWallContacts.h src/core/EnemyContact.h
	@mkdir -p build
	$(CXX) -Isrc -std=c++17 -O2 -Wall -Wextra -o $@ $<
.PHONY: test-terrain-wall-contacts
test-terrain-wall-contacts: build/terrain-wall-contacts-test
	./build/terrain-wall-contacts-test
test: test-terrain-wall-contacts
verify-smoke-owner: build/smoke-owner-test
	./build/smoke-owner-test

.PHONY: verify-effect-birth-sequences
verify-effect-birth-sequences: build/effect-birth-sequences-test
	./build/effect-birth-sequences-test
test: verify-effect-birth-sequences
build/effect-birth-sequences-test: tests/core/EffectBirthSequenceTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/EffectBirthSequenceTest.cpp src/core/GameMakerRandom.cpp

build/smoke-owner-test: tests/core/SmokeEffectsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SmokeEffectsTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-smoke-trig
verify-smoke-trig: build/smoke-trig-test
	python3 tools/export-smoke-trig.py --check
	./build/smoke-trig-test

$(OBJECTS) $(TEST_TARGETS) $(PORT_REPLAY_REFERENCE_TARGET): generated/smoke-trig.inc generated/atan-q16.inc generated/contact-motion.inc generated/clear-motion.inc

build/generate-clear-motion: tools/generate-clear-motion.cpp src/core/LevelClearMotion.h src/core/HudReal.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $<

generated/clear-motion.inc: build/generate-clear-motion tools/generate-clear-motion.py data/data.win data/Downwell.exe extracted/utmt-0.9.2.0/manifest.json
	python3 tools/generate-clear-motion.py

build/generate-contact-motion: tools/generate-contact-motion.cpp src/core/ContactMotionCodec.h src/core/RunnerPointDirection.h src/core/SmokeDirection.h src/core/SmokeTrig.h src/core/MotionBinary64.h src/core/MultiplyBinary64.h src/core/RunnerImageValue.h generated/smoke-trig.inc generated/atan-q16.inc
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $<

generated/contact-motion.inc: build/generate-contact-motion tools/generate-contact-motion.py
	python3 tools/generate-contact-motion.py

# Header-only core tests also need the generated include on a clean parallel
# build. The generator does not include ContactMotion.h, so this is acyclic.
src/core/ContactMotion.h: | generated/contact-motion.inc

build/contact-motion-test: tests/core/ContactMotionTest.cpp $(wildcard src/core/*.h) generated/contact-motion.inc
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<
.PHONY: verify-contact-motion
build/contact-coordinates-test: tests/core/ContactCoordinatesTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<
.PHONY: verify-contact-coordinates
verify-contact-coordinates: build/contact-coordinates-test
	./build/contact-coordinates-test
test: verify-contact-coordinates
verify-contact-motion: build/contact-motion-test
	./build/contact-motion-test
test: verify-contact-motion
build/player-damage-number-test build/player-damage-numbers-test build/damage-number-worlds-test: generated/contact-motion.inc

generated/smoke-trig.inc: tools/export-smoke-trig.py data/Downwell.exe
	python3 tools/export-smoke-trig.py

build/probe-contact-motion-domain: tools/probe-contact-motion-domain.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc generated/atan-q16.inc
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY: probe-contact-motion-domain
probe-contact-motion-domain: build/probe-contact-motion-domain
	./build/probe-contact-motion-domain

build/smoke-trig-test: tests/core/SmokeTrigTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SmokeTrigTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-runner-step-order
verify-runner-step-order:
	python3 tools/verify-runner-step-order.py
	python3 tools/verify-runner-step-eligibility.py

.PHONY: verify-smoke-direction
test: verify-machine-gun-source-motion verify-effect-spawn
.PHONY: verify-effect-spawn
verify-effect-spawn: build/effect-spawn-script-test build/script-spawn-effects-test
	python3 tools/verify-effect-spawn.py
	./build/effect-spawn-script-test
	./build/script-spawn-effects-test
build/script-spawn-effects-test: tests/core/ScriptSpawnEffectsTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
build/effect-spawn-script-test: tests/core/EffectSpawnScriptTest.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: verify-machine-gun-source-motion verify-bullet-solid-script verify-machine-gun-source-step
test: verify-runner-sprite-vertices
.PHONY: verify-runner-sprite-vertices
verify-runner-sprite-vertices:
	python3 tools/verify-runner-sprite-vertices.py
	python3 tests/impact-angle-art-test.py
test: verify-bullet-solid-script verify-machine-gun-source-step verify-box-destruction
.PHONY: verify-box-destruction
verify-box-destruction: build/box-destruction-test build/box-destruction-integration-test
	python3 tools/verify-box-destruction.py
	./build/box-destruction-test
	./build/box-destruction-integration-test
build/box-destruction-integration-test: tests/core/BoxDestructionIntegrationTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/BoxDestructionIntegrationTest.cpp src/core/GameMakerRandom.cpp
build/box-destruction-test: tests/core/BoxDestructionScriptTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
verify-machine-gun-source-step: build/machine-gun-source-step-test
	python3 tools/verify-machine-gun.py
	./build/machine-gun-source-step-test
test: verify-drone-bullet
.PHONY: verify-drone-bullet
verify-drone-bullet: build/drone-bullet-source-step-test
	python3 tools/verify-drone-bullet.py
	python3 tools/verify-machine-gun.py
	python3 tools/verify-runner-alarms.py
	python3 tools/verify-runner-image-fields.py
	./build/drone-bullet-source-step-test
build/drone-bullet-source-step-test: tests/core/DroneBulletSourceStepTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $< src/core/GameMakerRandom.cpp
build/machine-gun-source-step-test: tests/core/MachineGunSourceStepTest.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
verify-bullet-solid-script: build/bullet-solid-script-test
	python3 tools/verify-machine-gun.py
	./build/bullet-solid-script-test

.PHONY: verify-bullet-wall-sound
test: verify-bullet-wall-sound
verify-bullet-wall-sound: build/bullet-wall-sound-test
	python3 tools/verify-bullet-wall-sound.py
	./build/bullet-wall-sound-test

build/bullet-wall-sound-test: tests/core/BulletWallSoundTest.cpp tests/core/BulletWallSoundChecks.h $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/BulletWallSoundTest.cpp src/core/GameMakerRandom.cpp

build/live-collision-lifecycle-test build/gameplay-collision-owner-test build/gameplay-collision-owner-scalar-test build/gameplay-collision-owner-snapshot-test: tests/core/BulletWallSoundChecks.h tests/core/SourceBulletSchedulingChecks.h

build/bullet-solid-script-test: tests/core/BulletSolidScriptTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test: verify-runner-spatial-line
.PHONY: verify-runner-spatial-line
verify-runner-spatial-line: build/runner-spatial-line-test
	python3 tools/verify-runner-line-point.py
	./build/runner-spatial-line-test

build/runner-spatial-line-test: tests/core/RunnerSpatialLineTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ $<

verify-machine-gun-source-motion: build/machine-gun-source-motion-test
	python3 tools/verify-machine-gun.py
	python3 tools/verify-runner-line-point.py
	python3 tools/verify-runner-smoke-geometry.py
	python3 tools/verify-runner-effect-ranges.py
	./build/machine-gun-source-motion-test

build/machine-gun-source-motion-test: tests/core/MachineGunSourceMotionTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/MachineGunSourceMotionTest.cpp src/core/GameMakerRandom.cpp

verify-smoke-direction: build/smoke-direction-test
	./build/smoke-direction-test

build/smoke-direction-test: tests/core/SmokeDirectionTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SmokeDirectionTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-smoke-distance
verify-smoke-distance: build/smoke-distance-test
	./build/smoke-distance-test

build/smoke-distance-test: tests/core/SmokeDistanceTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SmokeDistanceTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-smoke-effect
test: verify-blood-effect
test: verify-time-stop-goal-contact
.PHONY: verify-time-stop-goal-contact
verify-time-stop-goal-contact: build/time-stop-goal-contact-test
	./build/time-stop-goal-contact-test

build/time-stop-goal-contact-test: tests/core/TimeStopGoalContactTest.cpp $(wildcard src/core/*.h) generated/downwell-goal-mask.h generated/downwell-goal-mask.mask generated/downwell-time-field-mask.h generated/downwell-time-field-mask.mask
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeStopGoalContactTest.cpp

test: verify-blood-goal-contact
.PHONY: verify-blood-goal-contact
verify-blood-goal-contact: build/blood-goal-contact-test
	./build/blood-goal-contact-test

build/blood-goal-contact-test: tests/core/BloodGoalContactTest.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc generated/downwell-goal-mask.h generated/downwell-goal-mask.mask generated/downwell-time-field-mask.h generated/downwell-time-field-mask.mask
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/BloodGoalContactTest.cpp

test: verify-blood-bounds
.PHONY: verify-blood-bounds
verify-blood-bounds: build/blood-bounds-test
	./build/blood-bounds-test

build/blood-bounds-test: tests/core/BloodCollisionBoundsTest.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/BloodCollisionBoundsTest.cpp

test: verify-time-stop-bound
test: verify-player-time-stop
test: verify-time-change
test: verify-time-stop-masks
test: verify-ordinary-time-field
test: verify-runner-spatial-split
.PHONY: verify-runner-spatial-split
.PHONY: verify-contact-profile
build/encounter-coverage-test: tests/platform/amiga/GameplayEncounterCoverageTest.cpp tests/platform/amiga/GameplayEncounterCoverage.h src/core/GameplayContactProfileInput.h src/core/Input.h src/core/GemLifetime.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: verify-encounter-coverage
verify-encounter-coverage: build/encounter-coverage-test
	./$<
test: verify-encounter-coverage
build/entry-profile-input-test: tests/platform/amiga/AmigaEntryProfileInputTest.cpp src/platform/amiga/AmigaSceneRuntime.cpp src/core/GameplayEntryProfileInput.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

.PHONY: verify-entry-profile-input
verify-entry-profile-input: build/entry-profile-input-test
	./$<
test: verify-entry-profile-input

verify-contact-profile: $(AMIGA_SCENE_TEST_TARGET)
	DOWNWELL_VERIFY_CONTACT_PROFILE=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

.PHONY: generate-shop-profile
build/shop-profile-input-test: tests/core/GameplayShopProfileInputTest.cpp src/core/GameplayShopProfileInput.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: verify-shop-profile-input
verify-shop-profile-input: build/shop-profile-input-test
	./$<
test: verify-shop-profile-input

generate-shop-profile: $(AMIGA_SCENE_TEST_TARGET) verify-shop-profile-input
	python3 tools/generate-shop-profile.py

.PHONY: verify-random-replay
verify-random-replay: build/random-replay-test
	DOWNWELL_VERIFY_CONTACT_PROFILE=1 ./build/random-replay-test generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

build/random-replay-test: $(AMIGA_SCENE_TEST_SOURCES) $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG -DDOWNWELL_AUDIT_RANDOM -o $@ $(AMIGA_SCENE_TEST_SOURCES)

# Domain evidence only: traces actual post-wrapper atan inputs, not a new
# approximation. This macro and observer are absent from shipping builds.
build/contact-direction-domain-test: $(AMIGA_SCENE_TEST_SOURCES) tests/core/ContactDirectionTrace.cpp $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_TRACE_CONTACT_DIRECTION -o $@ $(AMIGA_SCENE_TEST_SOURCES) tests/core/ContactDirectionTrace.cpp

.PHONY: trace-contact-direction-domain
trace-contact-direction-domain: build/contact-direction-domain-test
	DOWNWELL_VERIFY_CONTACT_PROFILE=1 ./$< generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_VERIFY_LASER_SIGHT=1 DOWNWELL_VERIFY_COMBAT=1 ./$< generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

verify-runner-spatial-split: build/runner-spatial-split-test build/runner-spatial-dirty-test build/runner-object-membership-test build/runner-sprite-bounds-test build/runner-place-query-test build/runner-spatial-instances-test
	./build/runner-spatial-split-test
	./build/runner-spatial-dirty-test
	./build/runner-object-membership-test
	./build/runner-sprite-bounds-test
	./build/runner-place-query-test
	./build/runner-spatial-instances-test
	python3 tools/verify-runner-field-lookup.py
	python3 tools/verify-runner-collision-bounds.py

build/runner-spatial-instances-test: tests/core/RunnerSpatialInstancesTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/RunnerSpatialInstancesTest.cpp

build/runner-sprite-bounds-test: tests/core/RunnerSpriteBoundsTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/RunnerSpriteBoundsTest.cpp

build/runner-place-query-test: tests/core/RunnerPlaceQueryTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/RunnerPlaceQueryTest.cpp

build/runner-spatial-journal-test: tests/core/RunnerSpatialJournalTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: test-spatial-journal
test-spatial-journal: build/runner-spatial-journal-test
	./build/runner-spatial-journal-test
test: test-spatial-journal

.PHONY: test-spatial-capacity
test: test-spatial-capacity
test-spatial-capacity: build/runner-spatial-capacity-test
	./build/runner-spatial-capacity-test
build/runner-spatial-capacity-test: tests/core/RunnerSpatialCapacityTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

build/terrain-row-selection-test: tests/core/TerrainRowSelectionTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
.PHONY: test-terrain-row-selection
test-terrain-row-selection: build/terrain-row-selection-test
	./build/terrain-row-selection-test
test: test-terrain-row-selection

build/runner-spatial-split-test: tests/core/RunnerSpatialSplitTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/RunnerSpatialSplitTest.cpp

build/runner-object-membership-test: tests/core/RunnerObjectMembershipTest.cpp src/core/RunnerObjectMembership.h src/core/RunnerRunMembership.h src/core/SlotIndex.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/RunnerObjectMembershipTest.cpp

build/runner-spatial-dirty-test: tests/core/RunnerSpatialDirtyQueueTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/RunnerSpatialDirtyQueueTest.cpp

.PHONY: verify-ordinary-time-field
verify-ordinary-time-field: build/ordinary-time-field-test
	./build/ordinary-time-field-test
	python3 tools/verify-runner-field-draw.py
	python3 tools/verify-runner-field-lookup.py

generated/downwell-time-field-mask.h: generated/time-stop-masks.json tools/verify-time-stop-masks.py
	python3 tools/verify-time-stop-masks.py --generate

generated/downwell-time-field-mask.mask: generated/downwell-time-field-mask.h
	@test -f $@ || python3 tools/verify-time-stop-masks.py --generate

build/ordinary-time-field-test: tests/core/OrdinaryTimeFieldTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/downwell-time-field-mask.h generated/downwell-time-field-mask.mask generated/downwell-goal-mask.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/OrdinaryTimeFieldTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-stop-masks
test: verify-precise-masks
.PHONY: generate-precise-masks verify-precise-masks generate-collision-masks verify-collision-masks
generate-precise-masks: generate-collision-masks
verify-precise-masks: verify-collision-masks
generate-collision-masks: generated/collision-sprite-masks.json
	python3 tools/generate-collision-masks.py

verify-collision-masks: generate-collision-masks generate-sprite-pack build/sprite-mask-pack-test build/runner-mask-pair-test
	python3 tools/generate-collision-masks.py --check
	./build/sprite-mask-pack-test generated/downwell-sprite-masks.dwsm generated/downwell-sprites.dwsc
	./build/runner-mask-pair-test generated/downwell-sprite-masks.dwsm generated/downwell-sprites.dwsc

build/runner-mask-pair-test: tests/core/RunnerMaskPairTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/RunnerMaskPairTest.cpp

build/sprite-mask-pack-test: tests/core/SpriteMaskPackTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SpriteMaskPackTest.cpp

generated/collision-sprite-masks.json: tools/export-collision-masks.csx data/data.win
	@mkdir -p generated
	@test "$$(shasum -a 256 data/data.win | awk '{print $$1}')" = c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
	DOWNWELL_DATA_WIN_SHA256=c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8 DOWNWELL_COLLISION_MASK_OUTPUT=generated/collision-sprite-masks.json "$$(tools/bootstrap-utmt.sh)" load data/data.win -s tools/export-collision-masks.csx
	@test "$$(shasum -a 256 data/data.win | awk '{print $$1}')" = c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8

verify-time-stop-masks: generated/time-stop-masks.json
	python3 tools/verify-time-stop-masks.py

generated/time-stop-masks.json: tools/export-time-stop-masks.csx data/data.win
	@mkdir -p generated
	@test "$$(shasum -a 256 data/data.win | awk '{print $$1}')" = c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
	DOWNWELL_DATA_WIN_SHA256=c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8 DOWNWELL_TIME_STOP_MASK_OUTPUT=generated/time-stop-masks.json "$$(tools/bootstrap-utmt.sh)" load data/data.win -s tools/export-time-stop-masks.csx
	@test "$$(shasum -a 256 data/data.win | awk '{print $$1}')" = c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8

test: verify-time-change-effects
.PHONY: verify-time-change-effects
test: verify-gameplay-time-change-registration
.PHONY: verify-gameplay-time-change-registration
verify-gameplay-time-change-registration: build/gameplay-time-change-registration-test
	./build/gameplay-time-change-registration-test

build/gameplay-time-change-registration-test: tests/core/GameplayTimeChangeRegistrationTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/GameplayTimeChangeRegistrationTest.cpp

test: verify-gameplay-time-shard-registration
.PHONY: verify-gameplay-time-shard-registration
verify-gameplay-time-shard-registration: build/gameplay-time-shard-registration-test
	./build/gameplay-time-shard-registration-test

build/gameplay-time-shard-registration-test: tests/core/GameplayTimeShardRegistrationTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/GameplayTimeShardRegistrationTest.cpp src/core/GameMakerRandom.cpp

verify-time-change-effects: build/time-change-effects-test
	./build/time-change-effects-test

build/time-change-effects-test: tests/core/TimeChangeEffectsTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/TimeChangeEffectsTest.cpp

test: verify-time-shard-birth verify-time-shard-lifecycle verify-time-shard-motion verify-time-shard-animation verify-time-shard-effects verify-time-shard-burst verify-point-direction
.PHONY: verify-point-direction
test: verify-portable-atan
test: verify-time-stop-effects verify-runner-destroy

.PHONY: verify-runner-destroy
verify-runner-destroy:
	python3 tools/verify-runner-destroy.py
test: verify-typical-player-damage
test: verify-player-damage
test: verify-forced-player-jump
test: verify-damage-profile
.PHONY: verify-damage-profile
verify-damage-profile: generated/smoke-trig.inc generated/downwell-goal-mask.h generated/downwell-time-field-mask.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o build/damage-profile-test tests/core/DamageProfileTest.cpp
	./build/damage-profile-test
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -DDOWNWELL_PROBES -o build/damage-profile-probes-test tests/core/DamageProfileTest.cpp
	./build/damage-profile-probes-test
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -DDOWNWELL_FPSCOUNT -o build/damage-profile-fps-test tests/core/DamageProfileTest.cpp
	./build/damage-profile-fps-test
.PHONY: verify-forced-player-jump
verify-forced-player-jump: build/forced-player-jump-test
	./build/forced-player-jump-test
build/forced-player-jump-test: tests/core/ForcedPlayerJumpTest.cpp $(wildcard src/core/*.h) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/ForcedPlayerJumpTest.cpp src/core/GameMakerRandom.cpp
test: verify-player-damage-number
test: verify-damage-number-text
.PHONY: verify-damage-number-text
verify-damage-number-text: build/damage-number-text-test
	./build/damage-number-text-test
build/damage-number-text-test: tests/core/DamageNumberTextTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/DamageNumberTextTest.cpp
test: verify-player-damage-numbers
test: verify-foreground-effect-order
.PHONY: verify-foreground-effect-order
verify-foreground-effect-order: build/foreground-effect-order-test
	python3 tools/verify-effect-depths.py
	python3 tools/verify-runner-field-draw.py
	./build/foreground-effect-order-test
build/foreground-effect-order-test: tests/core/ForegroundEffectOrderTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/downwell-goal-mask.h generated/downwell-time-field-mask.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/ForegroundEffectOrderTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-player-damage-numbers
verify-player-damage-numbers: build/player-damage-numbers-test
	./build/player-damage-numbers-test

build/player-damage-numbers-test: tests/core/PlayerDamageNumbersTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PlayerDamageNumbersTest.cpp

.PHONY: verify-player-damage-number
verify-player-damage-number: build/player-damage-number-test
	python3 tools/verify-typical-player-damage.py
	python3 tools/verify-effect-depths.py
	./build/player-damage-number-test

build/player-damage-number-test: tests/core/PlayerDamageNumberTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PlayerDamageNumberTest.cpp

.PHONY: verify-player-damage
verify-player-damage: build/player-damage-test
	python3 tools/verify-typical-player-damage.py
	./build/player-damage-test

build/player-damage-test: tests/core/PlayerDamageTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/downwell-goal-mask.h generated/downwell-time-field-mask.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PlayerDamageTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-typical-player-damage
verify-typical-player-damage: build/typical-player-damage-test
	python3 tools/verify-typical-player-damage.py
	./build/typical-player-damage-test

build/typical-player-damage-test: tests/core/TypicalPlayerDamageTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/downwell-goal-mask.h generated/downwell-time-field-mask.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TypicalPlayerDamageTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-stop-effects
verify-time-stop-effects: build/time-stop-effects-test
	./build/time-stop-effects-test

build/time-stop-effects-test: tests/core/TimeStopEffectsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeStopEffectsTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-portable-atan
verify-portable-atan: build/portable-atan-test
	./build/portable-atan-test

build/portable-atan-test: tests/core/PortableAtanTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PortableAtanTest.cpp src/core/GameMakerRandom.cpp

.PHONY: probe-runner-atan-x86-macos
generated/atan-q16.inc: tools/generate-atan-table.py
	python3 $<

.PHONY: probe-table-atan-x86-macos
probe-table-atan-x86-macos: generated/atan-q16.inc
	@mkdir -p build
	clang++ -arch x86_64 -std=c++17 -O2 -ffp-contract=off -Isrc -DDOWNWELL_PROBE_TABLE_ATAN tests/core/RunnerAtanProbe.cpp -o build/table-atan-probe
	arch -x86_64 build/table-atan-probe

build/contact-direction-table-test: $(AMIGA_SCENE_TEST_SOURCES) $(wildcard src/core/*.h) generated/atan-q16.inc
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_COMPARE_CONTACT_ATAN -o $@ $(AMIGA_SCENE_TEST_SOURCES)

# Optional numeric evidence only; not a portable/default test or Windows run.
probe-runner-atan-x86-macos:
	@mkdir -p build
	clang++ -arch x86_64 -std=c++17 -O2 -ffp-contract=off -Isrc tests/core/RunnerAtanProbe.cpp -o build/runner-atan-probe
	arch -x86_64 build/runner-atan-probe

verify-point-direction: build/point-direction-test
	python3 tools/verify-runner-point-direction.py
	./build/point-direction-test

.PHONY: verify-direction-value
test: verify-direction-value
verify-direction-value: build/direction-value-test
	python3 tools/verify-runner-direction-value.py
	./build/direction-value-test

build/direction-value-test: tests/core/RunnerDirectionValueTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ $<

build/point-direction-test: tests/core/RunnerPointDirectionTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/RunnerPointDirectionTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-shard-burst
verify-time-shard-burst: build/time-shard-burst-test
	./build/time-shard-burst-test

build/time-shard-burst-test: tests/core/TimeShardBurstTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeShardBurstTest.cpp src/core/GameMakerRandom.cpp

test: verify-limbo-shard
.PHONY: verify-limbo-shard
verify-limbo-shard: build/limbo-shard-test
	python3 tools/verify-limbo-shard.py
	./build/limbo-shard-test

build/limbo-shard-test: tests/core/LimboShardTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/LimboShardTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-shard-effects
verify-time-shard-effects: build/time-shard-effects-test
	./build/time-shard-effects-test

build/time-shard-effects-test: tests/core/TimeShardEffectsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeShardEffectsTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-shard-animation
verify-time-shard-animation: build/time-shard-animation-test
	python3 tools/verify-runner-image-fields.py
	python3 tools/verify-animation-metadata.py
	./build/time-shard-animation-test

build/time-shard-animation-test: tests/core/TimeShardAnimationTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeShardAnimationTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-shard-motion
verify-time-shard-motion: build/time-shard-motion-test
	python3 tools/verify-runner-integer-range.py
	./build/time-shard-motion-test

build/time-shard-motion-test: tests/core/TimeShardMotionTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeShardMotionTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-shard-lifecycle
test: verify-time-field-wind
.PHONY: verify-time-field-wind
test: verify-gameplay-collision-owner
.PHONY: verify-borrowed-state
test: verify-borrowed-state
verify-borrowed-state: build/runner-borrowed-state-test
	./build/runner-borrowed-state-test
build/runner-borrowed-state-test: tests/core/RunnerBorrowedStateTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<
build/gameplay-collision-owner-test build/gameplay-collision-owner-scalar-test build/gameplay-collision-owner-snapshot-test build/enemy-death-worlds-test build/amiga-terrain-ownership-test build/amiga-gameplay-draw-list-test build/amiga-gameplay-draw-list-replay-test: tests/core/CollisionOwnerFixture.h
.PHONY: verify-gameplay-collision-owner
.PHONY: verify-slot-family-map
.PHONY: verify-stationary-retirement
test: verify-stationary-retirement
verify-stationary-retirement: build/stationary-retirement-test
	./build/stationary-retirement-test
build/stationary-retirement-test: tests/core/StationaryRetirementWindowTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<
test: verify-slot-family-map
verify-slot-family-map: build/slot-family-map-test
	./build/slot-family-map-test
build/slot-family-map-test: tests/core/SlotFamilyMapTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<
.PHONY: verify-native-enemy-alarm-roster
build/native-enemy-alarm-roster-test: tests/core/NativeEnemyAlarmRosterTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
verify-native-enemy-alarm-roster: build/native-enemy-alarm-roster-test
	./build/native-enemy-alarm-roster-test
test: verify-native-enemy-alarm-roster
build/terrain-ownership-runs-test: tests/core/TerrainOwnershipRunsTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

verify-terrain-ownership-runs: build/terrain-ownership-runs-test
	./build/terrain-ownership-runs-test
.PHONY: verify-terrain-ownership-runs
.PHONY: verify-terrain-source-records
build/terrain-publication-clock-test: tests/core/TerrainPublicationClockTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
build/terrain-birth-records-test: tests/core/TerrainBirthRecordsTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
build/native-terrain-source-records-test: tests/core/NativeTerrainSourceRecordsTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
verify-terrain-source-records: build/terrain-publication-clock-test build/terrain-birth-records-test build/native-terrain-source-records-test
	./build/terrain-publication-clock-test
	./build/terrain-birth-records-test
	./build/native-terrain-source-records-test
.PHONY: verify-terrain-read-census
build/terrain-read-census-off: tests/core/TerrainReadCensusTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
build/terrain-read-census-on: tests/core/TerrainReadCensusTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_TERRAIN_READ_CENSUS -o $@ $<
verify-terrain-read-census: build/terrain-read-census-off build/terrain-read-census-on
	./build/terrain-read-census-off
	./build/terrain-read-census-on
	python3 -B tests/test_terrain_read_census.py
build/terrain-appearance-publication-test: tests/core/TerrainAppearancePublicationTest.cpp src/core/ObjectPack.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

.PHONY: verify-terrain-appearance-publication
verify-terrain-appearance-publication: build/terrain-appearance-publication-test generate-object-pack generate-sprite-pack
	./build/terrain-appearance-publication-test

test: verify-terrain-appearance-publication

build/terrain-spatial-publication-test: tests/core/TerrainSpatialPublicationTest.cpp src/core/ObjectPack.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ tests/core/TerrainSpatialPublicationTest.cpp src/core/ObjectPack.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-terrain-spatial-publication
verify-terrain-spatial-publication: build/terrain-spatial-publication-test generate-object-pack generate-sprite-pack
	./build/terrain-spatial-publication-test

test: verify-terrain-spatial-publication

build/terrain-birth-transaction-test: tests/core/TerrainBirthTransactionTest.cpp src/core/ObjectPack.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ tests/core/TerrainBirthTransactionTest.cpp src/core/ObjectPack.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-terrain-birth-transaction
verify-terrain-birth-transaction: build/terrain-birth-transaction-test generate-object-pack generate-sprite-pack
	./build/terrain-birth-transaction-test

test: verify-terrain-birth-transaction

build/runner-observed-dirty-test: tests/core/RunnerObservedDirtyQueueTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/RunnerObservedDirtyQueueTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-observed-dirty-queues
verify-observed-dirty-queues: build/runner-observed-dirty-test
	./build/runner-observed-dirty-test

test: verify-observed-dirty-queues

test: verify-terrain-ownership-runs

build/runner-logical-layer-order-test: tests/core/RunnerLogicalLayerOrderTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: verify-logical-layer-order
verify-logical-layer-order: build/runner-logical-layer-order-test
	./build/runner-logical-layer-order-test

test: verify-logical-layer-order

verify-gameplay-collision-owner: build/gameplay-collision-owner-test build/gameplay-collision-owner-scalar-test build/gameplay-collision-owner-snapshot-test build/gameplay-collision-owner-audit-test build/gameplay-collision-owner-reference-enemies-test
	DOWNWELL_OWNERSHIP_WITNESS=build/ownership-runs.txt ./build/gameplay-collision-owner-test
	DOWNWELL_OWNERSHIP_WITNESS=build/ownership-audit.txt ./build/gameplay-collision-owner-audit-test
	cmp build/ownership-runs.txt build/ownership-audit.txt
	DOWNWELL_OWNERSHIP_WITNESS=build/ownership-reference-enemies.txt ./build/gameplay-collision-owner-reference-enemies-test
	cmp build/ownership-runs.txt build/ownership-reference-enemies.txt
	DOWNWELL_OWNERSHIP_WITNESS=build/ownership-scalar.txt ./build/gameplay-collision-owner-scalar-test
	cmp build/ownership-runs.txt build/ownership-scalar.txt
	DOWNWELL_OWNERSHIP_WITNESS=build/ownership-snapshots.txt ./build/gameplay-collision-owner-snapshot-test
	cmp build/ownership-runs.txt build/ownership-snapshots.txt

build/live-collision-lifecycle-test build/gameplay-collision-owner-test build/gameplay-collision-owner-scalar-test build/gameplay-collision-owner-snapshot-test: tests/core/RegisteredSkeletonThrowChecks.h

build/gameplay-collision-owner-test: tests/core/GameplayCollisionOwnerTest.cpp tests/core/ScheduledBoxEffectsCheck.h tests/core/GeneratedSourceBirthChecks.h src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

FRAME_WORK_TEST_SOURCES = tests/core/GameplayCollisionOwnerTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp
build/frame-work-test build/frame-work-reference-test: $(FRAME_WORK_TEST_SOURCES) $(wildcard src/core/*.h tests/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_FRAME_WORK_TEST $(if $(findstring reference,$@),-DDOWNWELL_REFERENCE_FRAME_WORK) -o $@ $(FRAME_WORK_TEST_SOURCES)
.PHONY: test-frame-work
test-frame-work: build/frame-work-test build/frame-work-reference-test
	./build/frame-work-test
	./build/frame-work-reference-test

build/gameplay-frame-test build/gameplay-frame-reference-test: tests/core/GameplayFrameTest.cpp src/core/GameplayFrame.h src/core/GameplayLimits.h src/core/PlayerMotion.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) $(if $(findstring reference,$@),-DDOWNWELL_REFERENCE_FRAME_WORK) -o $@ $<
.PHONY: test-gameplay-frame
test-gameplay-frame: build/gameplay-frame-test build/gameplay-frame-reference-test
	./build/gameplay-frame-test
	./build/gameplay-frame-reference-test
test: test-gameplay-frame

# Native enemy updates cross-checked against the walks they replace, and
# the reference enemy updates they replace.
build/gameplay-collision-owner-reference-enemies-test: CXXFLAGS += -DDOWNWELL_REFERENCE_ENEMY_UPDATES
build/gameplay-collision-owner-reference-enemies-test: tests/core/GameplayCollisionOwnerTest.cpp tests/core/ScheduledBoxEffectsCheck.h tests/core/GeneratedSourceBirthChecks.h tests/core/RegisteredSkeletonThrowChecks.h src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
build/gameplay-collision-owner-audit-test: CXXFLAGS += -DDOWNWELL_AUDIT_ENEMY_UPDATES
build/gameplay-collision-owner-audit-test: tests/core/GameplayCollisionOwnerTest.cpp tests/core/ScheduledBoxEffectsCheck.h tests/core/GeneratedSourceBirthChecks.h tests/core/RegisteredSkeletonThrowChecks.h src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/gameplay-external-collision-contract-test: CXXFLAGS += -DDOWNWELL_EXTERNAL_COLLISION_BACKEND
build/gameplay-external-collision-contract-test build/gameplay-full-collision-contract-test: tests/core/GameplayCollisionContractTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
.PHONY: test-gameplay-collision-contract
test-gameplay-collision-contract: build/gameplay-external-collision-contract-test build/gameplay-full-collision-contract-test build/gameplay-scene-collision-contract-test
	build/gameplay-external-collision-contract-test
	build/gameplay-full-collision-contract-test
	build/gameplay-scene-collision-contract-test

build/gameplay-scene-collision-contract-test: tests/core/GameplayCollisionContractTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_SCENE_COLLISION_BACKEND -o $@ $(filter %.cpp,$^)
test: test-gameplay-collision-contract

build/native-collision-source-span-test: tests/core/NativeCollisionSourceSpanTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(HOST_CHECK_HEADERS) tests/core/CollisionOwnerFixture.h
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_TERRAIN_SOURCE_RECORDS -o $@ $(filter %.cpp,$^)
.PHONY: test-native-collision-source-span
test-native-collision-source-span: build/native-collision-source-span-test
	./build/native-collision-source-span-test
test: test-native-collision-source-span

build/gameplay-room-views-reference-test: tests/core/GameplayCollisionOwnerTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_REFERENCE_ROOM_VIEWS -o $@ $(filter %.cpp,$^)
.PHONY: verify-room-views
verify-room-views: build/gameplay-collision-owner-test build/gameplay-room-views-reference-test
	DOWNWELL_OWNERSHIP_WITNESS=build/room-views-default.txt ./build/gameplay-collision-owner-test
	DOWNWELL_OWNERSHIP_WITNESS=build/room-views-reference.txt ./build/gameplay-room-views-reference-test
	cmp build/room-views-default.txt build/room-views-reference.txt
test: verify-room-views

build/gameplay-local-draw-capture-test: tests/core/GameplayCollisionOwnerTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_LOCAL_DRAW_CAPTURE=2 -o $@ $(filter %.cpp,$^)
build/native-draw-capture-test: tests/core/NativeDrawCaptureTest.cpp src/core/NativeInstanceTable.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_LOCAL_DRAW_CAPTURE=2 -o $@ $<
.PHONY: verify-local-draw-capture
verify-local-draw-capture: build/native-draw-capture-test build/gameplay-local-draw-capture-test
	./build/native-draw-capture-test
	./build/gameplay-local-draw-capture-test

build/gameplay-collision-owner-scalar-test: tests/core/GameplayCollisionOwnerTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_SCALAR_TERRAIN_OWNERSHIP=1 -o $@ $(filter %.cpp,$^)

build/gameplay-collision-owner-snapshot-test: tests/core/GameplayCollisionOwnerTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_TERRAIN_SNAPSHOT_CALLBACKS=1 -o $@ $(filter %.cpp,$^)

verify-time-field-wind: verify-sound-play build/time-field-wind-test
	./build/time-field-wind-test

test: verify-sound-play
.PHONY: verify-sound-play
verify-sound-play: build/sound-play-script-test
	python3 tools/verify-sound-play.py
	./build/sound-play-script-test

build/sound-play-script-test: tests/core/SoundPlayScriptTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/time-field-wind-test: tests/core/TimeFieldWindTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

verify-time-shard-lifecycle: build/time-shard-lifecycle-test
	./build/time-shard-lifecycle-test

build/time-shard-lifecycle-test: tests/core/TimeShardLifecycleTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeShardLifecycleTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-shard-birth
verify-time-shard-birth: build/time-shard-birth-test
	./build/time-shard-birth-test

build/time-shard-birth-test: tests/core/TimeShardBirthTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/TimeShardBirthTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-time-change
test: verify-entry-dither
test: verify-dither-above
.PHONY: verify-dither-above
verify-dither-above: build/dither-above-step-test
	python3 tools/verify-runner-instance-ids.py
	./build/dither-above-step-test

build/dither-above-step-test: tests/core/DitherAboveStepTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/DitherAboveStepTest.cpp

.PHONY: verify-entry-dither
verify-entry-dither: build/entry-dither-test
	python3 tools/verify-runner-instance-ids.py
	./build/entry-dither-test

test: verify-stored-entry-dither
.PHONY: verify-stored-entry-dither
verify-stored-entry-dither: build/stored-entry-dither-test
	./build/stored-entry-dither-test

build/stored-entry-dither-test: tests/core/StoredEntryDitherTest.cpp $(wildcard src/core/*.h) src/core/ObjectPack.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/StoredEntryDitherTest.cpp src/core/ObjectPack.cpp

test: verify-entry-mask-offset
.PHONY: verify-entry-mask-offset
verify-entry-mask-offset: build/amiga-entry-mask-test
	./build/amiga-entry-mask-test

build/amiga-entry-mask-test: tests/platform/amiga/AmigaEntryMaskTest.cpp src/platform/amiga/AmigaEntryMask.h src/core/EntryDitherMask.h src/platform/amiga/AmigaExitMask.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/platform/amiga/AmigaEntryMaskTest.cpp

build/entry-dither-test: tests/core/EntryDitherTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/EntryDitherTest.cpp

verify-time-change: build/time-change-test
	./build/time-change-test

build/time-change-test: tests/core/TimeChangeEffectTest.cpp src/core/TimeChangeEffect.h src/core/AlarmBank.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/TimeChangeEffectTest.cpp

.PHONY: verify-player-time-stop
verify-player-time-stop: build/player-time-stop-test
	./build/player-time-stop-test

build/player-time-stop-test: tests/core/PlayerTimeStopTest.cpp src/core/PlayerTimeStop.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/PlayerTimeStopTest.cpp

.PHONY: verify-player-horizontal-binary64
test: verify-player-horizontal-binary64
verify-player-horizontal-binary64: build/player-horizontal-binary64-test
	./build/player-horizontal-binary64-test

build/player-horizontal-binary64-test: tests/core/PlayerHorizontalBinary64Test.cpp src/core/PlayerHorizontalBinary64.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PlayerHorizontalBinary64Test.cpp

.PHONY: verify-player-solid-collision-binary64
test: verify-player-solid-collision-binary64
verify-player-solid-collision-binary64: build/player-solid-collision-binary64-test
	./build/player-solid-collision-binary64-test

build/player-solid-collision-binary64-test: tests/core/PlayerSolidCollisionBinary64Test.cpp src/core/PlayerSolidCollisionBinary64.h src/core/SolidCollision.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PlayerSolidCollisionBinary64Test.cpp

.PHONY: verify-player-wall-response-binary64
test: verify-player-wall-response-binary64
verify-player-wall-response-binary64: build/player-wall-response-binary64-test
	./build/player-wall-response-binary64-test

build/player-wall-response-binary64-test: tests/core/PlayerWallResponseBinary64Test.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PlayerWallResponseBinary64Test.cpp

.PHONY: verify-player-gravity-binary64
test: verify-player-gravity-binary64
verify-player-gravity-binary64: build/player-gravity-binary64-test
	./build/player-gravity-binary64-test

build/player-gravity-binary64-test: tests/core/PlayerGravityBinary64Test.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/PlayerGravityBinary64Test.cpp

test: verify-goal-field-bounds
.PHONY: verify-goal-field-bounds
verify-goal-field-bounds: build/goal-field-bounds-test
	./build/goal-field-bounds-test

build/goal-field-bounds-test: tests/core/GoalFieldBoundsTest.cpp $(wildcard src/core/*.h) generated/downwell-goal-mask.h generated/downwell-goal-mask.mask
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/GoalFieldBoundsTest.cpp

.PHONY: verify-time-stop-bound
verify-time-stop-bound: build/time-stop-bound-test
	./build/time-stop-bound-test

build/time-stop-bound-test: tests/core/TimeStopBoundTest.cpp src/core/TimeStopBound.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc -o $@ tests/core/TimeStopBoundTest.cpp

test: verify-blood-owner
.PHONY: verify-blood-owner
verify-blood-owner: build/blood-owner-test
	./build/blood-owner-test

build/blood-owner-test: tests/core/BloodEffectsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/BloodEffectsTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-blood-effect
verify-blood-effect: build/blood-effect-test
	python3 tools/verify-blood-events.py
	python3 tests/blood-angle-art-test.py
	./build/blood-effect-test

build/blood-effect-test: tests/core/BloodEffectTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/BloodEffectTest.cpp src/core/GameMakerRandom.cpp

verify-smoke-effect: build/smoke-effect-test
	./build/smoke-effect-test

build/smoke-effect-test: tests/core/SmokeEffectTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SmokeEffectTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-smoke-motion
verify-smoke-motion: build/smoke-motion-test
	./build/smoke-motion-test

build/smoke-motion-test: tests/core/SmokeMotionTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SmokeMotionTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-smoke-birth
verify-smoke-birth: build/smoke-birth-test
	./build/smoke-birth-test

build/smoke-birth-test: tests/core/SmokeBirthTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -Isrc -o $@ tests/core/SmokeBirthTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-flash-effects
verify-flash-effects: build/flash-effects-test
	./build/flash-effects-test

build/flash-effects-test: tests/core/FlashBallEffectsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ $< src/core/GameMakerRandom.cpp

test: verify-skeleton-effect-bursts
.PHONY: verify-skeleton-effect-bursts
verify-skeleton-effect-bursts: build/skeleton-effect-bursts-test
	./build/skeleton-effect-bursts-test

build/skeleton-effect-bursts-test: tests/core/SkeletonEffectBurstsTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ $< src/core/GameMakerRandom.cpp

test: verify-skeleton-encounter
.PHONY: verify-skeleton-encounter
verify-skeleton-encounter: build/skeleton-encounter-test
	./build/skeleton-encounter-test

build/skeleton-encounter-test: tests/core/SkeletonEncounterTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ $< src/core/GameMakerRandom.cpp

verify-skeleton-actor: build/skeleton-actor-test
	./build/skeleton-actor-test

.PHONY: verify-skeleton-publication
test: verify-skeleton-publication
verify-skeleton-publication: build/skeleton-publication-test
	python3 tools/verify-bone-events.py
	./build/skeleton-publication-test

build/skeleton-publication-test: tests/core/SkeletonPublicationTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ $<

build/live-collision-lifecycle-test build/gameplay-collision-owner-test build/gameplay-collision-owner-scalar-test build/gameplay-collision-owner-snapshot-test: tests/core/QuietSkeletonEffects.h tests/core/ScheduledBoxEffectsCheck.h tests/core/SkeletonSoundChecks.h

build/skeleton-actor-test: tests/core/SkeletonActorTest.cpp tests/core/SkeletonSoundChecks.h $(wildcard src/core/*.h) src/core/PlayerMotion.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ $< src/core/PlayerMotion.cpp

.PHONY: verify-blast-actor
test: verify-blast-actor
verify-blast-actor: build/blast-actor-test
	python3 tools/verify-blast-actor.py
	./build/blast-actor-test

build/blast-actor-test: tests/core/BlastActorTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ $<

.PHONY: verify-skeleton-decisions
verify-skeleton-decisions: build/skeleton-decisions-test
	./build/skeleton-decisions-test

build/skeleton-decisions-test: tests/core/SkeletonDecisionsTest.cpp src/core/SkeletonDecisions.h src/core/SkeletonAnimation.h src/core/SkeletonTiming.h src/core/UnitSequenceFrames.h src/core/SequenceTimeline.h src/core/RunnerImageValue.h src/core/MotionBinary64.h src/core/AlarmBank.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/SkeletonDecisionsTest.cpp

verify-skeleton-animation: build/skeleton-animation-test
	./build/skeleton-animation-test

build/skeleton-animation-test: tests/core/SkeletonAnimationTest.cpp src/core/RunnerSpriteChange.h src/core/SkeletonAnimation.h src/core/SkeletonTiming.h src/core/UnitSequenceFrames.h src/core/SequenceTimeline.h src/core/RunnerImageValue.h src/core/MotionBinary64.h src/core/AlarmBank.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/SkeletonAnimationTest.cpp

verify-unit-sequence-frames: build/unit-sequence-frames-test
	./build/unit-sequence-frames-test

build/unit-sequence-frames-test: tests/core/UnitSequenceFramesTest.cpp src/core/UnitSequenceFrames.h src/core/SequenceTimeline.h src/core/RunnerImageValue.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/UnitSequenceFramesTest.cpp

verify-sequence-timeline: build/sequence-timeline-test
	./build/sequence-timeline-test

build/sequence-timeline-test: tests/core/SequenceTimelineTest.cpp src/core/SequenceTimeline.h src/core/RunnerImageValue.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/core/SequenceTimelineTest.cpp

.PHONY: verify-animation-metadata
verify-animation-metadata:
	bash tools/export-animation-metadata.sh
	python3 tools/verify-animation-metadata.py

verify-legacy-image-animation: build/legacy-image-animation-test
	./build/legacy-image-animation-test

build/legacy-image-animation-test: tests/core/LegacyImageAnimationTest.cpp src/core/LegacyImageAnimation.h src/core/RunnerImageValue.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/core/LegacyImageAnimationTest.cpp

verify-runner-image-value: build/runner-image-value-test
	./build/runner-image-value-test

build/runner-image-value-test: tests/core/RunnerImageValueTest.cpp src/core/RunnerImageValue.h src/core/RunnerIntegerValueVerification.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/core/RunnerImageValueTest.cpp

verify-skeleton-timing: build/skeleton-timing-test
	./build/skeleton-timing-test

build/skeleton-timing-test: tests/core/SkeletonTimingTest.cpp src/core/SkeletonTiming.h src/core/MotionBinary64.h src/core/AlarmBank.h src/core/RunnerImageValue.h src/core/LegacyImageAnimation.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/core/SkeletonTimingTest.cpp

.PHONY: verify-bone-reference
.PHONY: verify-motion-binary64
.PHONY: verify-bone-motion
.PHONY: verify-bone-projectile
.PHONY: verify-bone-publication
test: verify-bone-publication
verify-bone-publication: build/bone-publication-test
	python3 tools/verify-bone-events.py
	./build/bone-publication-test

build/bone-publication-test: tests/core/BonePublicationTest.cpp $(wildcard src/core/*.h) src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/BonePublicationTest.cpp src/core/GameMakerRandom.cpp

.PHONY: verify-random-binary64
verify-random-binary64: build/random-binary64-test
	./build/random-binary64-test

build/random-binary64-test: tests/core/RandomBinary64Test.cpp src/core/RandomBinary64.h src/core/MotionBinary64.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/RandomBinary64Test.cpp

verify-bone-projectile: build/bone-projectile-test
	./build/bone-projectile-test

build/bone-projectile-test: tests/core/BoneProjectileTest.cpp src/core/BoneProjectile.h src/core/BoneMotion.h src/core/RandomBinary64.h src/core/MotionBinary64.h src/core/MotionBinary64Verification.h src/core/BoneProjectileReference.h src/core/GameMakerRounding.h src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/BoneProjectileTest.cpp src/core/GameMakerRandom.cpp

verify-bone-motion: build/bone-motion-test
	./build/bone-motion-test

build/bone-motion-test: tests/core/BoneMotionTest.cpp src/core/BoneMotion.h src/core/MotionBinary64.h src/core/MotionBinary64Verification.h src/core/BoneProjectileReference.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/BoneMotionTest.cpp

verify-motion-binary64: build/motion-binary64-test
	./build/motion-binary64-test

build/motion-binary64-test: tests/core/MotionBinary64Test.cpp src/core/MotionBinary64.h src/core/MotionBinary64Verification.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/MotionBinary64Test.cpp

verify-bone-reference: build/bone-reference-test
	./build/bone-reference-test

build/bone-reference-test: tests/core/BoneProjectileReferenceTest.cpp src/core/BoneProjectileReference.h src/core/GameMakerRandom.cpp src/core/GameMakerRandom.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/BoneProjectileReferenceTest.cpp src/core/GameMakerRandom.cpp

verify-skeleton-throw: build/skeleton-throw-test
	./build/skeleton-throw-test

build/skeleton-throw-test: tests/core/SkeletonThrowTest.cpp src/core/SkeletonThrow.h src/core/BoneProjectile.h src/core/BoneMotion.h src/core/RandomBinary64.h src/core/MotionBinary64.h src/core/BoneProjectileReference.h src/core/GameMakerRounding.h src/core/GameMakerRandom.cpp src/core/GameMakerRandom.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -ffp-contract=off -Isrc -o $@ tests/core/SkeletonThrowTest.cpp src/core/GameMakerRandom.cpp

verify-catacomb-symbols: build/catacomb-symbols-test build/builder-common-dispatch-test build/stored-builder-publication-test build/box-create-script-test
	./build/catacomb-symbols-test
	./build/builder-common-dispatch-test
	./build/stored-builder-publication-test
	./build/box-create-script-test

build/box-create-script-test: tests/core/BoxCreateScriptTest.cpp src/core/GameMakerRandom.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/stored-builder-publication-test: tests/core/StoredBuilderPublicationTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

build/builder-common-dispatch-test: tests/core/BuilderCommonDispatchTest.cpp src/core/BuilderCommonDispatch.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

build/catacomb-symbols-test: tests/core/CatacombBuilderSymbolsTest.cpp src/core/CatacombBuilderSymbols.h src/core/CatacombBuilderDispatch.h src/core/GameMakerRandom.cpp src/core/GameMakerRandom.h src/core/RandomBinary64.h src/core/RunnerRealComparison.h
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc $< src/core/GameMakerRandom.cpp -o $@

generate-area-two-patterns:
	python3 tools/export-area-two-patterns.py

build/area-two-pattern-pack-test: tests/core/AreaTwoPatternPackTest.cpp src/core/BuilderPatternPack.cpp src/core/BuilderPatternPack.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

build/builder-string-traversal-test: tests/core/BuilderStringTraversalTest.cpp src/core/BuilderStringTraversal.h src/core/BuilderCommonDispatch.h src/core/CatacombBuilderDispatch.h src/core/CatacombBuilderSymbols.h src/core/BuilderPatternPack.cpp src/core/BuilderPatternPack.h src/core/GameMakerRandom.cpp src/core/GameMakerRandom.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

verify-area-two-patterns: generate-area-two-patterns build/area-two-pattern-pack-test build/builder-string-traversal-test
	python3 tests/area-two-patterns-test.py
	python3 tools/export-area-two-patterns.py --check

.PHONY: verify-natural-exit verify-route-sweep verify-gameplay-death-state
test: verify-gameplay-death-state
verify-route-sweep: generate-room-pack generate-object-pack generate-builder-pattern-pack $(AMIGA_SCENE_TEST_TARGET)
	DOWNWELL_VERIFY_ROUTE_SWEEP=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

verify-natural-exit: generate-room-pack generate-object-pack generate-builder-pattern-pack $(AMIGA_SCENE_TEST_TARGET)
	DOWNWELL_VERIFY_NATURAL_EXIT=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

verify-gameplay-death-state: generate-room-pack generate-object-pack generate-builder-pattern-pack $(AMIGA_SCENE_TEST_TARGET)
	DOWNWELL_COMBAT_TICKS=6000 DOWNWELL_COMBAT_FIRE_PHASE=4 DOWNWELL_VERIFY_COMBAT=1 DOWNWELL_VERIFY_DEATH=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_COMBAT_TICKS=6000 DOWNWELL_COMBAT_FIRE_PHASE=5 DOWNWELL_VERIFY_COMBAT=1 DOWNWELL_VERIFY_SURVIVAL=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

# These executables compile multiple translation units in one command. The
# compiler's single .d file only describes the final input, so explicitly
# invalidate them on shared header changes rather than trusting stale tools.
$(OBJECTS) $(TEST_TARGETS) $(PORT_REPLAY_REFERENCE_TARGET) $(AREA_ONE_TRACE_TARGET) $(AREA_ONE_BUILDER_TEST_TARGET) $(AMIGA_LIVE_TERRAIN_TEST_TARGET) build/gameplay-player-step-test build/gem-homing-test build/frog-behavior-test build/bat-motion-test build/enemy-contact-test: generated/downwell-goal-mask.h
build/goal-time-field-test build/shop-world-test build/level-clear-session-test build/combo-state-test: generated/downwell-goal-mask.h

generated/downwell-goal-mask.h: tools/generate-goal-mask.py tools/export-goal-mask.csx data/data.win generated/downwell-time-field-mask.h
	python3 tools/generate-goal-mask.py

generated/downwell-goal-mask.mask: generated/downwell-goal-mask.h
	@test -f $@ || python3 tools/generate-goal-mask.py
$(TEST_TARGETS) $(PORT_REPLAY_REFERENCE_TARGET) $(AREA_ONE_BUILDER_TEST_TARGET) $(AMIGA_LIVE_TERRAIN_TEST_TARGET) build/gem-homing-test build/frog-behavior-test build/bat-motion-test build/enemy-contact-test build/gameplay-player-step-test: $(HOST_CHECK_HEADERS)

.PHONY: test-host-check-dependencies
test: test-host-check-dependencies
test-host-check-dependencies:
	python3 tests/host-check-dependencies-test.py

test: build/gem-homing-test test-gem-position

.PHONY: test-gem-position
test-gem-position: build/gem-position-test
	./build/gem-position-test
build/gem-position-test: tests/core/GemPositionTest.cpp $(HOST_CHECK_HEADERS) generated/downwell-goal-mask.h
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc -o $@ $<
test: build/main-camera-test
test: build/main-room-activation-test
test: build/player-death-sequence-test build/run-time-parts-test
test: build/death-menu-sequence-test
test: build/death-menu-progress-test build/combo-state-test
test: verify-amiga-results
test: build/player-death-verification-test
test: build/standard-bullet-test
test: build/crawler-motion-test
test: build/snail-motion-test
test: build/walking-enemy-test
test: build/enemy-motion-math-test

test: test-gameplay-capture-comparison
.PHONY: test-gameplay-capture-comparison
test-gameplay-capture-comparison:
	python3 tests/compare-gameplay-captures-test.py

build/walking-enemy-test: tests/core/WalkingEnemyMotionTest.cpp src/core/WalkingEnemyMotion.h src/core/SolidCollision.h src/core/EnemyContact.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/WalkingEnemyMotionTest.cpp

build/snail-motion-test: tests/core/SnailMotionTest.cpp src/core/SnailMotion.h src/core/SnailHitState.h src/core/AlarmBank.h src/core/SolidCollision.h src/core/EnemyContact.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/SnailMotionTest.cpp

build/crawler-motion-test: tests/core/CrawlerMotionTest.cpp src/core/CrawlerMotion.h src/core/SolidCollision.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/CrawlerMotionTest.cpp

build/enemy-motion-math-test: tests/core/EnemyMotionMathTest.cpp src/core/PixelCollisionMath.h src/core/CrawlerMotion.h src/core/SnailMotion.h src/core/WalkingEnemyMotion.h src/core/SolidCollision.h src/core/EnemyContact.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/EnemyMotionMathTest.cpp

build/standard-bullet-test: tests/core/StandardBulletStepTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/StandardBulletStepTest.cpp

.PHONY: test-machine-gun-trajectory
test: test-machine-gun-trajectory
test-machine-gun-trajectory: build/machine-gun-trajectory-test
	./build/machine-gun-trajectory-test

.PHONY: test-projectile-segment
.PHONY: test-laser-sight-ray
test: test-laser-sight-ray
test-laser-sight-ray: build/laser-sight-ray-test
	./build/laser-sight-ray-test

build/laser-sight-ray-test: tests/core/LaserSightRayTest.cpp src/core/LaserSightRay.h $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/LaserSightRayTest.cpp

test: test-projectile-segment
test-projectile-segment: build/projectile-segment-test
	./build/projectile-segment-test

build/projectile-segment-test: tests/core/ProjectileSegmentTest.cpp src/core/ProjectileSegment.h $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/ProjectileSegmentTest.cpp

build/machine-gun-trajectory-test: tests/core/MachineGunTrajectoryTest.cpp src/core/MachineGunTrajectory.h src/core/MachineGunEmitter.h src/core/MachineGunProfile.h src/core/LevelClearTrig.h src/core/HudReal.h src/core/GameMakerRandom.h src/core/GameMakerRandom.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/MachineGunTrajectoryTest.cpp src/core/GameMakerRandom.cpp

build/main-camera-test: tests/core/MainCameraMotionTest.cpp src/core/MainCameraMotion.h src/core/SideCameraLocker.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/MainCameraMotionTest.cpp

build/main-room-activation-test: tests/core/MainRoomActivationTest.cpp src/core/MainRoomActivation.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/MainRoomActivationTest.cpp

build/death-menu-sequence-test: tests/core/DeathMenuSequenceTest.cpp src/core/DeathMenuSequence.h src/core/AlarmBank.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/DeathMenuSequenceTest.cpp

build/combo-state-test: tests/core/ComboStateTest.cpp src/core/ComboState.h src/core/ComboGrants.h src/core/RunCombo.h src/core/CurrencyAccount.h src/core/PlayerMotion.cpp src/core/PlayerMotion.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/ComboStateTest.cpp src/core/PlayerMotion.cpp

build/death-menu-progress-test: tests/core/DeathMenuProgressTest.cpp src/core/DeathMenuProgress.h src/core/DeathMenuSequence.h src/core/RunGemStatistics.h src/core/DeathResults.h src/core/DeathResultsProgressBar.h src/core/DeathResultsProgressLabels.h src/core/Input.h src/core/AlarmBank.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/DeathMenuProgressTest.cpp

build/player-death-sequence-test: tests/core/PlayerDeathSequenceTest.cpp src/core/PlayerDeathSequence.h src/core/RunClock.h src/core/AlarmBank.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/PlayerDeathSequenceTest.cpp

build/run-time-parts-test: tests/core/RunTimePartsTest.cpp src/core/RunTimeParts.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/RunTimePartsTest.cpp

build/player-death-verification-test: tests/core/PlayerDeathVerificationTest.cpp src/core/PlayerMotion.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/PlayerDeathVerificationTest.cpp src/core/PlayerMotion.cpp

build/gem-homing-test: tests/core/GemHomingTest.cpp src/core/GemHoming.h src/core/GemMotion.h src/core/GemLifetime.h src/core/GemAnimation.h src/core/BatMotion.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/GemHomingTest.cpp

build/frog-behavior-test: tests/core/FrogBehaviorTest.cpp src/core/FrogBehavior.h src/core/FrogMotion.h src/core/FrogHitState.h src/core/FractionalMotion.h src/core/FrogHorizontalMotion.h src/core/FrogVerticalMotion.h src/core/EnemyContact.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/FrogBehaviorTest.cpp

build/enemy-contact-test: tests/core/EnemyContactTest.cpp src/core/EnemyContact.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/EnemyContactTest.cpp

build/bat-motion-test: tests/core/BatMotionTest.cpp src/core/BatMotion.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/BatMotionTest.cpp

build/sdl-audio-test: tests/platform/SDLAudioTest.cpp src/platform/sdl/AudioSDL.cpp src/platform/sdl/AudioSDL.h src/core/RunnerAudioGain.h src/core/AudioMasterGain.h
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(filter %.cpp,$^) $(LDFLAGS)
.PHONY: verify-sdl-audio
test: verify-sdl-audio
verify-sdl-audio: build/sdl-audio-test
	python3 tools/verify-runner-audio-gain.py
	SDL_AUDIODRIVER=dummy ./build/sdl-audio-test

build/sdl-audio-test: src/core/ContactDamageScripts.h src/core/PlayerDamage.h src/core/TypicalPlayerDamage.h src/core/SoundPlayScript.h src/core/MusicDuck.h

build/encounter-boundary-test: tests/core/EncounterBoundaryTest.cpp src/core/EncounterBoundary.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

SESSION_TEST_SOURCES := tests/platform/StoredEncounterSessionTest.cpp src/platform/sdl/AudioSDL.cpp $(filter-out tests/core/GameRenderTest.cpp,$(GAME_RENDER_TEST_SOURCES))
SESSION_TEST_CPPFLAGS := $(filter-out -DDOWNWELL_SKIP_INTRO -DDOWNWELL_SKIP_TO_GAME -DDOWNWELL_SKIP_LEVEL,$(CPPFLAGS))
build/stored-encounter-session-test: $(SESSION_TEST_SOURCES) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(SESSION_TEST_CPPFLAGS) $(CXXFLAGS) -o $@ $(SESSION_TEST_SOURCES) $(LDFLAGS)
build/stored-encounter-session-skip-test: $(SESSION_TEST_SOURCES) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(SESSION_TEST_CPPFLAGS) $(CXXFLAGS) -DDOWNWELL_SKIP_TO_GAME -o $@ $(SESSION_TEST_SOURCES) $(LDFLAGS)
.PHONY: test-encounter-session
test: test-encounter-session
test-encounter-session: build/encounter-boundary-test build/stored-encounter-session-test build/stored-encounter-session-skip-test
	./build/encounter-boundary-test
	SDL_AUDIODRIVER=dummy ./build/stored-encounter-session-test
	SDL_AUDIODRIVER=dummy ./build/stored-encounter-session-skip-test

build/sdl-screenshot-test: tests/platform/SDLScreenshotTest.cpp src/platform/sdl/PlatformSDL.cpp src/core/SpritePack.cpp src/core/FontPack.cpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/sdl-input-test: tests/platform/SDLInputTest.cpp src/platform/sdl/PlatformSDL.cpp src/core/SpritePack.cpp src/core/FontPack.cpp src/core/Replay.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(filter %.cpp,$^) $(LDFLAGS)
.PHONY: test-sdl-input
test: test-sdl-input
test-sdl-input: build/sdl-input-test
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/sdl-input-test

build/amiga-input-test: tests/platform/amiga/AmigaInputTest.cpp src/platform/amiga/AmigaInput.cpp src/core/Replay.cpp $(wildcard tests/platform/amiga/input-compat/*/*.h) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/input-compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
.PHONY: test-amiga-input
test: test-amiga-input
test-amiga-input: build/amiga-input-test
	./build/amiga-input-test

build/amiga-keyboard-test: tests/platform/amiga/AmigaKeyboardTest.cpp src/platform/amiga/AmigaKeyboard.cpp $(wildcard tests/platform/amiga/input-compat/*/*.h) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/input-compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
.PHONY: test-amiga-keyboard
test: test-amiga-keyboard
test-amiga-keyboard: build/amiga-keyboard-test
	./build/amiga-keyboard-test

build/sdl-flash-raster-test: tests/platform/SDLFlashRasterTest.cpp src/platform/sdl/PlatformSDL.cpp src/core/SpritePack.cpp src/core/FontPack.cpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/effect-publish-test: tests/platform/EffectPublishTest.cpp src/platform/sdl/PlatformSDL.cpp src/platform/amiga/AmigaRenderer.c src/core/SpritePack.cpp src/core/FontPack.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(CPPFLAGS) -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp %.c,$^) $(LDFLAGS)
.PHONY: verify-effect-publish
test: verify-effect-publish
verify-effect-publish: build/effect-publish-test
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/effect-publish-test

.PHONY: verify-sdl-flash-raster
verify-sdl-flash-raster: build/sdl-flash-raster-test generate-sprite-pack
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/sdl-flash-raster-test

.PHONY: verify-flash-render-paths verify-effect-depths
build/effect-mask-assets-test: tests/core/EffectMaskAssetsTest.cpp src/core/EffectMaskAssets.h src/core/BinaryEffectSurface.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: verify-effect-mask-assets
verify-effect-mask-assets: build/effect-mask-assets-test
	./build/effect-mask-assets-test
test: verify-effect-mask-assets

verify-effect-depths:
	python3 tools/verify-effect-depths.py
test: verify-effect-depths
verify-flash-render-paths: verify-effect-depths build/game-render-test test-amiga-gameplay-draw-list verify-sdl-flash-raster
	DOWNWELL_FLASH_RENDER_ONLY=1 ./build/game-render-test

.PHONY: verify-damage-render-path
verify-damage-render-path: build/game-render-test verify-contact-profile
	DOWNWELL_DAMAGE_RENDER_ONLY=1 ./build/game-render-test

build/skip-intro-test: tests/core/SkipIntroTest.cpp $(filter-out tests/core/GameRenderTest.cpp,$(GAME_RENDER_TEST_SOURCES)) src/platform/amiga/AmigaSceneRuntime.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_SKIP_INTRO -o $@ $^

build/skip-to-game-test: tests/core/SkipToGameTest.cpp $(filter-out tests/core/GameRenderTest.cpp,$(GAME_RENDER_TEST_SOURCES)) src/platform/amiga/AmigaSceneRuntime.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_SKIP_INTRO -DDOWNWELL_SKIP_TO_GAME -o $@ $^

build/skip-level-test: tests/core/SkipLevelTest.cpp $(filter-out tests/core/GameRenderTest.cpp,$(GAME_RENDER_TEST_SOURCES)) src/platform/amiga/AmigaSceneRuntime.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_SKIP_INTRO -DDOWNWELL_SKIP_TO_GAME -DDOWNWELL_SKIP_LEVEL -o $@ $(filter %.cpp,$^)

.PHONY: test-skip-level
test-skip-level: build/skip-level-test
	./build/skip-level-test
test: test-skip-level

.PHONY: FORCE_STARTUP_OPTIONS
build/ammo-dividers-test: tests/core/AmmoDividersTest.cpp src/core/AmmoDividers.h src/core/HudReal.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/AmmoDividersTest.cpp

test: test-ammo-dividers test-ammo-fill
test: test-shop-inventory
test: test-shop-interaction test-shop-menu-layout
test: test-level-exit
test: test-level-upgrade-offers
test: test-level-clear-selection
test: test-upgrade-inventory
test: test-level-clear-controller
test: test-level-clear-motion
test: test-level-clear-session
test: test-level-clear-presentation
.PHONY: test-level-clear-presentation
build/level-clear-presentation-test: tests/core/LevelClearPresentationTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-level-clear-presentation: build/level-clear-presentation-test
	./build/level-clear-presentation-test
.PHONY: test-level-clear-session
build/level-clear-session-test: tests/core/LevelClearSessionTest.cpp src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/LevelClearSessionTest.cpp src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp
test-level-clear-session: build/level-clear-session-test
	./build/level-clear-session-test
.PHONY: test-level-clear-motion
build/level-clear-motion-test: tests/core/LevelClearMotionTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-level-clear-motion: build/level-clear-motion-test
	./build/level-clear-motion-test
.PHONY: test-level-clear-controller
build/level-clear-controller-test: tests/core/LevelClearControllerTest.cpp src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/LevelClearControllerTest.cpp src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp
test-level-clear-controller: build/level-clear-controller-test
	./build/level-clear-controller-test
.PHONY: test-upgrade-inventory
build/upgrade-inventory-test: tests/core/UpgradeInventoryTest.cpp src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/UpgradeInventoryTest.cpp src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp
test-upgrade-inventory: build/upgrade-inventory-test
	./build/upgrade-inventory-test
.PHONY: test-level-clear-selection
build/level-clear-selection-test: tests/core/LevelClearSelectionTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-level-clear-selection: build/level-clear-selection-test
	./build/level-clear-selection-test
.PHONY: test-level-upgrade-offers
build/level-upgrade-offers-test: tests/core/LevelUpgradeOffersTest.cpp src/core/GameMakerRandom.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/LevelUpgradeOffersTest.cpp src/core/GameMakerRandom.cpp
test-level-upgrade-offers: build/level-upgrade-offers-test
	./build/level-upgrade-offers-test
test: test-goal-time-field
.PHONY: test-goal-time-field
build/goal-time-field-test: tests/core/GoalTimeFieldTest.cpp src/core/GameMakerRandom.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/GoalTimeFieldTest.cpp src/core/GameMakerRandom.cpp
test-goal-time-field: build/goal-time-field-test
	./build/goal-time-field-test
.PHONY: test-level-exit
build/level-exit-test: tests/core/LevelExitTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-level-exit: build/level-exit-test
	./build/level-exit-test
.PHONY: test-shop-menu-layout
build/shop-menu-layout-test: tests/core/ShopMenuLayoutTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-shop-menu-layout: build/shop-menu-layout-test
	./build/shop-menu-layout-test
test: test-shop-world
test: test-shop-keeper
.PHONY: test-shop-keeper
build/shop-keeper-test: tests/core/ShopKeeperTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-shop-keeper: build/shop-keeper-test
	./build/shop-keeper-test
.PHONY: test-shop-world
build/shop-world-test: tests/core/ShopWorldTest.cpp src/core/PlayerMotion.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-shop-world: build/shop-world-test
	./build/shop-world-test

build/shop-input-binding-test: tests/core/ShopInputBindingTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

.PHONY: test-shop-input-binding
test: test-shop-input-binding
test-shop-input-binding: build/shop-input-binding-test
	./build/shop-input-binding-test

build/gameplay-encounter-session-test: tests/core/GameplayEncounterSessionTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

.PHONY: test-gameplay-encounter-session
test: test-gameplay-encounter-session
test-gameplay-encounter-session: build/gameplay-encounter-session-test
	./build/gameplay-encounter-session-test

build/gameplay-dither-pass-test: tests/core/GameplayDitherCollisionPassTest.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
.PHONY: test-gameplay-dither-pass
test: test-gameplay-dither-pass
test-gameplay-dither-pass: build/gameplay-dither-pass-test
	./build/gameplay-dither-pass-test

generated/downwell-encounter-masks.dwsm: tools/generate-encounter-masks.py generated/downwell-sprite-masks.dwsm generated/downwell-sprite-masks.manifest.json extracted/utmt-0.9.2.0/manifest.json
	python3 tools/generate-encounter-masks.py
.PHONY: test-encounter-masks
test: test-encounter-masks
test-encounter-masks: generated/downwell-encounter-masks.dwsm build/gameplay-dither-pass-test
	python3 tools/generate-encounter-masks.py --check
	python3 tests/encounter-masks-test.py
	./build/gameplay-dither-pass-test generated/downwell-encounter-masks.dwsm

generated/amiga-wind.bin: tools/generate-amiga-wind-bank.py extracted/audio/manifest.json data/data.win $(wildcard extracted/audio/sound-effects/349_sfxMagicWind*.ogg extracted/audio/sound-effects/35[0-5]_sfxMagicWind*.ogg)
	python3 tools/generate-amiga-wind-bank.py

build/amiga-wind-bank-test: tests/platform/amiga/WindBankTest.cpp src/platform/amiga/WindBank.h
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc tests/platform/amiga/WindBankTest.cpp -o $@

.PHONY: test-amiga-wind-bank
test-amiga-wind-bank: generated/amiga-wind.bin build/amiga-wind-bank-test
	python3 tests/amiga-wind-bank-test.py
	./build/amiga-wind-bank-test generated/amiga-wind.bin

.PHONY: verify-amiga-wind-audio
verify-amiga-wind-audio: test-amiga-wind-bank
	. amiga/env.sh && $(MAKE) -C amiga -j4 wind-audio-probe
	VERIFY_PROGRAM=out/WindAudioProbe VERIFY_TIMEOUT_SECONDS=180 GDB_SOURCE=verify_wind_audio.gdb READY_MARKER=DOWNWELL_WIND_AUDIO_READY READY_DESCRIPTION='wind audio DMA stream' bash amiga/verify_startup.sh
build/stored-shop-child-test: tests/core/StoredShopChildTest.cpp src/core/ObjectPack.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-stored-shop-child: build/stored-shop-child-test
	./build/stored-shop-child-test
.PHONY: test-stored-shop-child
build/registered-shop-runtime-test: tests/core/RegisteredShopRuntimeTest.cpp $(filter-out tests/core/StartupRuntimeTest.cpp,$(STARTUP_TEST_SOURCES)) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-registered-shop-runtime: build/registered-shop-runtime-test
	./build/registered-shop-runtime-test
.PHONY: test-registered-shop-runtime
.PHONY: test-shop-interaction
build/shop-interaction-test: tests/core/ShopInteractionTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-shop-interaction: build/shop-interaction-test
	./build/shop-interaction-test
test: test-consumables
.PHONY: test-consumables
build/consumables-test: tests/core/ConsumablesTest.cpp src/core/PlayerMotion.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-consumables: build/consumables-test
	./build/consumables-test
.PHONY: test-shop-inventory
build/shop-inventory-test: tests/core/ShopInventoryTest.cpp tests/core/ShopPurchaseChecks.h $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-shop-inventory: build/shop-inventory-test
	./build/shop-inventory-test
.PHONY: test-ammo-fill
build/ammo-fill-test: tests/core/AmmoFillTest.cpp src/core/AmmoFill.h src/core/HudReal.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-ammo-fill: build/ammo-fill-test
	./build/ammo-fill-test
.PHONY: test-ammo-dividers
test-ammo-dividers: build/ammo-dividers-test
	./build/ammo-dividers-test

build/startup-options: FORCE_STARTUP_OPTIONS
	@mkdir -p build
	@printf '%s\n' '$(SKIP_INTRO)' '$(SKIP_TO_GAME)' '$(SKIP_LEVEL)' '$(CXXFLAGS)' > $@.tmp
	@cmp -s $@.tmp $@ || cp $@.tmp $@

$(OBJECTS): build/startup-options

build/generate-field-spans: tools/generate-field-spans.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Isrc -Wall -Wextra -Wpedantic $< -o $@

generated/downwell-field-spans.h: build/generate-field-spans
	@mkdir -p generated
	./build/generate-field-spans $@

build/generate-field-images: tools/generate-field-images.cpp generated/downwell-field-spans.h $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc -std=c++17 -O2 -ffp-contract=off -o $@ $<

generated/amiga-field-images.bin: build/generate-field-images tools/generate-field-images.py
	python3 tools/generate-field-images.py

.PHONY: verify-prepared-time-fields
build/generate-field-columns: tools/generate-field-columns.cpp $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc -std=c++17 -O2 -ffp-contract=off -o $@ $<

generated/downwell-field-columns.h: build/generate-field-columns tools/generate-field-columns.py
	python3 tools/generate-field-columns.py

generated/amiga-field-bands.bin generated/amiga-field-columns.bin: generated/downwell-field-columns.h
	@test -f $@ || python3 tools/generate-field-columns.py

.PHONY: verify-field-columns
.PHONY: verify-field-band-bank
build/field-band-bank-test: tests/platform/amiga/AmigaFieldBandBankTest.cpp src/platform/amiga/AmigaFieldBandBank.h generated/downwell-field-columns.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
verify-field-band-bank: build/field-band-bank-test generated/amiga-field-bands.bin
	./build/field-band-bank-test generated/amiga-field-bands.bin
build/field-column-bank-test: tests/platform/amiga/AmigaFieldColumnBankTest.cpp src/platform/amiga/AmigaFieldColumnBank.h generated/downwell-field-columns.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: verify-field-column-bank
verify-field-column-bank: build/field-column-bank-test generated/amiga-field-columns.bin
	./build/field-column-bank-test generated/amiga-field-columns.bin
test: verify-field-band-bank verify-field-column-bank
build/bitmap-copy-plan-test: tests/platform/amiga/BitmapCopyPlanTest.cpp src/platform/amiga/framework/BitmapCopyPlan.h
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: verify-bitmap-copy-plan
verify-bitmap-copy-plan: build/bitmap-copy-plan-test
	./build/bitmap-copy-plan-test
build/field-columns-test: tests/platform/amiga/AmigaFieldColumnsTest.cpp $(wildcard src/core/*.h) $(wildcard src/platform/amiga/*.h) generated/downwell-field-columns.h
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $<
verify-field-columns: build/field-columns-test generated/amiga-field-columns.bin
	./build/field-columns-test generated/amiga-field-columns.bin
build/field-composite-test: tests/platform/amiga/AmigaFieldCompositeTest.cpp $(wildcard src/core/*.h) $(wildcard src/platform/amiga/*.h) generated/downwell-field-columns.h
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $<
.PHONY: verify-field-composite
verify-field-composite: build/field-composite-test
	./build/field-composite-test

.PHONY: analyze-field-layouts
build/analyze-field-layouts: tools/analyze-field-layouts.cpp generated/downwell-field-columns.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -o $@ $<
analyze-field-layouts: build/analyze-field-layouts
	./build/analyze-field-layouts

build/prepared-time-fields-test: tests/platform/amiga/AmigaPreparedTimeFieldsTest.cpp $(wildcard src/core/*.h) $(wildcard src/platform/amiga/*.h) generated/amiga-field-images.bin generated/downwell-field-columns.h
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $<
verify-prepared-time-fields: build/prepared-time-fields-test
	./build/prepared-time-fields-test

$(AMIGA_RENDERER_TEST_TARGET): generated/downwell-field-spans.h src/platform/amiga/AmigaFieldSpans.h src/platform/amiga/AmigaFieldWords.h src/platform/amiga/AmigaGoalField.h

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

generated/amiga-shop.text.h: tools/generate-amiga-hud.py data/language.ini
	tools/generate-amiga-hud.py --profile shop

build/src/core/Game.o $(GAME_RENDER_TEST_TARGET): generated/amiga-shop.text.h
generated/amiga-clear.text.h: tools/generate-amiga-hud.py tools/amiga_scene_contract.py data/language.ini
	tools/generate-amiga-hud.py --profile clear

build/src/core/Game.o $(GAME_RENDER_TEST_TARGET): generated/amiga-clear.text.h

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

run: generate-room-pack generate-object-pack generate-sprite-pack generate-collision-masks generate-font-pack generate-builder-pattern-pack generate-amiga-game $(TARGET)
	./$(TARGET)

$(STARTUP_TEST_TARGET): $(STARTUP_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(STARTUP_TEST_SOURCES)

ROOM_LAYER_CATALOG_SOURCES := tests/core/RoomLayerCatalogTest.cpp $(filter-out tests/core/StartupRuntimeTest.cpp,$(STARTUP_TEST_SOURCES))
build/room-layer-catalog-test: $(ROOM_LAYER_CATALOG_SOURCES) $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(ROOM_LAYER_CATALOG_SOURCES)
.PHONY: verify-room-layer-catalog
build/prepared-empty-room-test: tests/core/PreparedEmptyRoomLayersTest.cpp src/core/RoomPack.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
.PHONY: verify-prepared-empty-room
verify-prepared-empty-room: build/prepared-empty-room-test
	./$< generated/downwell-rooms.dwrm
test: verify-prepared-empty-room
.PHONY: verify-room-layer-metadata
verify-room-layer-metadata:
	bash tools/export-room-layer-metadata.sh
	python3 tools/verify-room-layer-metadata.py

verify-room-layer-catalog: build/room-layer-catalog-test generate-room-pack
	./build/room-layer-catalog-test generated/downwell-rooms.dwrm
test: verify-room-layer-catalog

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

$(ROOM_PACK_TEST_TARGET): $(ROOM_PACK_TEST_SOURCES) $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(ROOM_PACK_TEST_SOURCES)

$(OBJECT_PACK_TEST_TARGET): $(OBJECT_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(OBJECT_PACK_TEST_SOURCES)

$(SPRITE_PACK_TEST_TARGET): $(SPRITE_PACK_TEST_SOURCES) $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(SPRITE_PACK_TEST_SOURCES)

$(FONT_PACK_TEST_TARGET): $(FONT_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(FONT_PACK_TEST_SOURCES)

$(BUILDER_PATTERN_PACK_TEST_TARGET): $(BUILDER_PATTERN_PACK_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(BUILDER_PATTERN_PACK_TEST_SOURCES)

$(AREA_ONE_BUILDER_TEST_TARGET): $(AREA_ONE_BUILDER_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AREA_ONE_BUILDER_TEST_SOURCES)

$(AMIGA_LIVE_TERRAIN_TEST_TARGET): $(AMIGA_LIVE_TERRAIN_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_LIVE_TERRAIN_TEST_SOURCES)

build/amiga-room-terrain-plan-test: tests/platform/amiga/AmigaRoomTerrainPlanTest.cpp $(filter-out tests/platform/amiga/AmigaLiveTerrainTest.cpp,$(AMIGA_LIVE_TERRAIN_TEST_SOURCES)) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaRoomTerrainPlanTest.cpp $(filter-out tests/platform/amiga/AmigaLiveTerrainTest.cpp,$(AMIGA_LIVE_TERRAIN_TEST_SOURCES))
.PHONY: verify-amiga-room-terrain-plan
generated/amiga-room-terrain-plan.inc: tools/generate-room-terrain-plan.py extracted/utmt-0.9.2.0/manifest.json
	python3 tools/generate-room-terrain-plan.py
src/platform/amiga/AmigaRoomTerrainPlan.h: | generated/amiga-room-terrain-plan.inc
build/amiga-room-terrain-plan-test: generated/amiga-room-terrain-plan.inc
verify-amiga-room-terrain-plan: build/amiga-room-terrain-plan-test generate-room-pack
	python3 tests/room-terrain-plan-test.py
	./build/amiga-room-terrain-plan-test generated/downwell-rooms.dwrm
	python3 tools/generate-room-terrain-plan.py --check
test: verify-amiga-room-terrain-plan

$(DEFERRED_INSTANCE_TEST_TARGET): $(DEFERRED_INSTANCE_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(DEFERRED_INSTANCE_TEST_SOURCES)

$(RANDOM_TEST_TARGET): $(RANDOM_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(RANDOM_TEST_SOURCES)

$(PLAYER_CONTROLS_TEST_TARGET): tests/core/PlayerControlsTest.cpp src/core/PlayerControls.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/PlayerControlsTest.cpp

build/gameplay-player-step-test: tests/core/GameplayPlayerStepTest.cpp $(wildcard src/core/*.h) src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp generated/downwell-goal-mask.mask
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/GameplayPlayerStepTest.cpp src/core/PlayerMotion.cpp src/core/GameMakerRandom.cpp

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

$(AMIGA_SCENE_TEST_TARGET): $(AMIGA_SCENE_TEST_SOURCES) tests/platform/amiga/GameplayEncounterCoverage.h
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_SCENE_TEST_SOURCES)

$(AMIGA_MENU_TEST_TARGET): $(AMIGA_MENU_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_MENU_TEST_SOURCES)

$(AMIGA_MENU_SCROLL_TEST_TARGET): $(AMIGA_MENU_SCROLL_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_MENU_SCROLL_TEST_SOURCES)

$(AMIGA_RENDERER_TEST_TARGET): $(AMIGA_RENDERER_TEST_SOURCES) src/platform/amiga/AmigaActorFrames.h generated/amiga-game-planes.h generated/amiga-scene-banks.h
	@mkdir -p $(@D)
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(AMIGA_RENDERER_TEST_SOURCES)

.PHONY: test-amiga-terrain-batch
test: test-amiga-terrain-batch
test-amiga-terrain-batch: generate-amiga-game build/amiga-terrain-batch-test build/amiga-terrain-painter-test
	python3 tools/generate-terrain-phase.py
	./build/amiga-terrain-batch-test generated/amiga-game.dwap
	./build/amiga-terrain-painter-test generated/amiga-game.dwap generated/amiga-terrain-phase6.bin
build/amiga-terrain-batch-test: tests/platform/amiga/AmigaTerrainBatchTest.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp %.c,$^)
generated/amiga-cavern.dwap: generated/amiga-game.dwap tools/amiga_scene_contract.py
	tools/generate-amiga-hud.py --profile game

generated/amiga-game.planes: generated/amiga-game.dwap generated/amiga-cavern.dwap tools/generate-amiga-plane-bank.py tools/amiga_scene_contract.py
	python3 tools/generate-amiga-plane-bank.py

generated/amiga-game-planes.h: generated/amiga-game.planes
	@test -f $@ || python3 tools/generate-amiga-plane-bank.py

generated/amiga-cavern-planes.h: generated/amiga-game.planes
	@test -f $@ || python3 tools/generate-amiga-plane-bank.py

build/amiga-plane-bank-test: tests/platform/amiga/AmigaPlaneBankTest.cpp src/platform/amiga/AmigaPlaneBank.h src/platform/amiga/AmigaGameplayAssetAdmission.h generated/amiga-game-planes.h generated/amiga-cavern-planes.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<

.PHONY: test-amiga-plane-bank
test-amiga-plane-bank: build/amiga-plane-bank-test
	./build/amiga-plane-bank-test

test: test-amiga-plane-bank

build/amiga-terrain-painter-test: tests/platform/amiga/AmigaTerrainPainterTest.cpp src/platform/amiga/AmigaTerrainPainter.cpp src/platform/amiga/AmigaRenderer.c $(filter-out tests/platform/amiga/AmigaLiveTerrainTest.cpp,$(AMIGA_LIVE_TERRAIN_TEST_SOURCES)) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_TERRAIN_BATCH -o $@ $(filter %.cpp %.c,$^)

build/prepare-initial-terrain: tools/prepare-initial-terrain.cpp src/platform/amiga/AmigaTerrainPainter.cpp src/platform/amiga/AmigaRenderer.c $(filter-out tests/platform/amiga/AmigaLiveTerrainTest.cpp,$(AMIGA_LIVE_TERRAIN_TEST_SOURCES)) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_TERRAIN_BATCH -o $@ $(filter %.cpp %.c,$^)
.PHONY: generate-initial-terrain
generate-initial-terrain: generate-amiga-game generate-room-pack build/prepare-initial-terrain
	python3 tools/generate-terrain-phase.py
	python3 tools/generate-initial-terrain.py

build/amiga-pattern-terrain-test: tests/platform/amiga/AmigaPatternTerrainTest.cpp src/platform/amiga/AmigaTerrainPainter.cpp src/platform/amiga/AmigaTerrainRaster.cpp src/platform/amiga/AmigaGameplayRows.cpp src/platform/amiga/AmigaRenderer.c $(filter-out tests/platform/amiga/AmigaLiveTerrainTest.cpp,$(AMIGA_LIVE_TERRAIN_TEST_SOURCES)) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_TERRAIN_BATCH -o $@ $(filter %.cpp %.c,$^)

.PHONY: generate-pattern-terrain test-pattern-terrain
build/amiga-pattern-row-test: tests/platform/amiga/AmigaPatternRowTest.cpp src/platform/amiga/AmigaPatternRow.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: test-amiga-pattern-row
test-amiga-pattern-row: build/amiga-pattern-row-test
	./build/amiga-pattern-row-test
test: test-amiga-pattern-row
generate-pattern-terrain: generate-amiga-game generate-builder-pattern-pack
	python3 tools/generate-pattern-terrain.py
test-pattern-terrain: generate-pattern-terrain build/amiga-pattern-terrain-test
	./build/amiga-pattern-terrain-test generated/amiga-game.dwap generated/amiga-pattern-terrain.bin

.PHONY: test-amiga-coverage-word
test: test-amiga-coverage-word
test-amiga-coverage-word: build/amiga-coverage-word-test
	./build/amiga-coverage-word-test
.PHONY: test-amiga-sprite-residency
test: test-amiga-sprite-residency
test-amiga-sprite-residency: build/amiga-sprite-residency-test
	./build/amiga-sprite-residency-test

build/amiga-sprite-residency-test: tests/platform/amiga/AmigaSpriteResidencyTest.cpp src/platform/amiga/AmigaRenderer.c src/platform/amiga/AmigaRenderer.h src/core/ProfileCounters.h
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_TEST_SPRITE_CACHE -DDOWNWELL_PROBES -o $@ tests/platform/amiga/AmigaSpriteResidencyTest.cpp src/platform/amiga/AmigaRenderer.c

build/amiga-coverage-word-test: tests/platform/amiga/AmigaCoverageWordTest.cpp src/platform/amiga/AmigaRenderer.c src/platform/amiga/AmigaRenderer.h src/platform/amiga/AmigaCoverageWordVerification.h
	@mkdir -p $(@D)
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaCoverageWordTest.cpp src/platform/amiga/AmigaRenderer.c

$(PORT_REPLAY_REFERENCE_TARGET): $(PORT_REPLAY_REFERENCE_SOURCES) $(wildcard src/core/*.h)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(PORT_REPLAY_REFERENCE_SOURCES)

$(AREA_ONE_TRACE_TARGET): $(AREA_ONE_TRACE_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(filter-out -O0,$(CXXFLAGS)) -O2 \
		-DDOWNWELL_TRACE_AREA_ONE -o $@ $(AREA_ONE_TRACE_SOURCES)

trace-area-one: generate-room-pack generate-object-pack generate-builder-pattern-pack $(AREA_ONE_TRACE_TARGET)
	./$(AREA_ONE_TRACE_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

test: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack generate-builder-pattern-pack generate-amiga-game generate-amiga-hud $(TEST_TARGETS) $(PLAYER_CONTROLS_TEST_TARGET) $(AREA_ONE_BUILDER_TEST_TARGET) $(AMIGA_LIVE_TERRAIN_TEST_TARGET) build/bat-motion-test build/enemy-contact-test build/frog-behavior-test build/gameplay-player-step-test
	./build/frog-behavior-test
	python3 tools/verify-enemy-contact.py
	python3 tools/verify-enemy-create.py
	python3 tools/verify-core-world.py "$(CXX)"
	python3 tools/verify-machine-gun.py
	./build/enemy-contact-test
	./build/bat-motion-test
	./build/gem-homing-test
	./build/main-camera-test
	./build/main-room-activation-test
	./build/player-death-sequence-test
	./build/run-time-parts-test
	./build/death-menu-sequence-test
	./build/death-menu-progress-test
	./build/combo-state-test
	./build/player-death-verification-test
	./build/standard-bullet-test
	./build/crawler-motion-test
	./build/snail-motion-test
	./build/walking-enemy-test
	./build/enemy-motion-math-test
	python3 tools/verify-amiga-style-mask.py
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	./$(REPLAY_TEST_TARGET)
	./$(ALARM_TEST_TARGET)
	./$(INSTANCE_TEST_TARGET)
	./$(EVENT_TEST_TARGET)
	./$(ROOM_TEST_TARGET)
	./$(ROOM_PACK_TEST_TARGET) generated/downwell-rooms.dwrm
	./$(OBJECT_PACK_TEST_TARGET) generated/downwell-objects.dwob
	./$(SPRITE_PACK_TEST_TARGET) generated/downwell-sprites.dwsp generated/downwell-sprite-masks.dwsm generated/downwell-objects.dwob
	./$(FONT_PACK_TEST_TARGET) generated/downwell-fonts.dwfn
	./$(BUILDER_PATTERN_PACK_TEST_TARGET) generated/downwell-builder-patterns.dwbp
	./$(AREA_ONE_BUILDER_TEST_TARGET) generated/downwell-builder-patterns.dwbp
	./$(AMIGA_LIVE_TERRAIN_TEST_TARGET) generated/downwell-builder-patterns.dwbp
	./$(DEFERRED_INSTANCE_TEST_TARGET)
	./$(RANDOM_TEST_TARGET)
	./$(PLAYER_COLLISION_TEST_TARGET)
	./$(PLAYER_CONTROLS_TEST_TARGET)
	./build/gameplay-player-step-test
	./$(GAME_RENDER_TEST_TARGET)
	./$(HUD_NUMBER_TEST_TARGET)
	./$(AMIGA_PLAYER_TEST_TARGET)
	./$(AMIGA_PORT_TEST_TARGET)
	DOWNWELL_VERIFY_COMBAT=1 DOWNWELL_VERIFY_FROG_MOTION=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_COMBAT_FIRE_PHASE=5 DOWNWELL_VERIFY_COMBAT=1 DOWNWELL_VERIFY_FROG_MOTION=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_VERIFY_ATTRACTOR=1 DOWNWELL_VERIFY_COMBAT=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_COMBAT_TICKS=6000 DOWNWELL_COMBAT_FIRE_PHASE=4 DOWNWELL_VERIFY_CANDLE=1 DOWNWELL_VERIFY_COMBAT=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_VERIFY_LASER_SIGHT=1 DOWNWELL_VERIFY_COMBAT=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_VERIFY_STREAK_UPGRADE=1 DOWNWELL_VERIFY_COMBAT=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_VERIFY_GEM_POWERED=1 DOWNWELL_VERIFY_COMBAT=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_VERIFY_GEM_HIGH=1 DOWNWELL_VERIFY_COMBAT=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	./$(AMIGA_MENU_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/amiga-front.dwap
	./$(AMIGA_MENU_SCROLL_TEST_TARGET)
	./$(AMIGA_RENDERER_TEST_TARGET) generated/amiga-game.dwap generated/amiga-hud.dwap

extract:
	tools/extract.sh
	tools/verify-extraction.sh

verify-extraction:
	tools/verify-extraction.sh

generated/room-layer-metadata.json: data/data.win tools/export-room-layer-metadata.csx tools/export-room-layer-metadata.sh
	bash tools/export-room-layer-metadata.sh

generate-room-pack: generated/room-layer-metadata.json
	python3 tools/verify-runner-instance-ids.py
	python3 tools/verify-room-layer-metadata.py
	tools/generate-room-pack.py
	tools/generate-room-pack.py --runtime
	python3 tests/runtime-room-admission-test.py

verify-room-pack: $(ROOM_PACK_TEST_TARGET) $(STARTUP_TEST_TARGET)
	python3 tools/verify-runner-instance-ids.py
	tools/generate-room-pack.py --check
	./$(ROOM_PACK_TEST_TARGET) generated/downwell-rooms.dwrm
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

generate-object-pack:
	tools/generate-object-pack.py

verify-object-pack: $(OBJECT_PACK_TEST_TARGET)
	tools/generate-object-pack.py --check
	./$(OBJECT_PACK_TEST_TARGET) generated/downwell-objects.dwob

generate-sprite-pack:
	tools/generate-sprite-pack.py

verify-sprite-pack: $(SPRITE_PACK_TEST_TARGET) generate-collision-masks generate-object-pack
	tools/generate-sprite-pack.py --check
	./$(SPRITE_PACK_TEST_TARGET) generated/downwell-sprites.dwsp generated/downwell-sprite-masks.dwsm generated/downwell-objects.dwob

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

generated/amiga-ammo-dividers.h: tools/generate-ammo-dividers.py generated/amiga-hud.dwap
	python3 tools/generate-ammo-dividers.py

generated/amiga-hud.dwap: tools/generate-amiga-hud.py
	tools/generate-amiga-hud.py

.PHONY: verify-prepared-ammo-dividers
build/prepared-ammo-renderer-test: $(AMIGA_RENDERER_TEST_SOURCES) $(wildcard src/platform/amiga/*.h) $(wildcard src/core/*.h) generated/amiga-ammo-dividers.h generated/downwell-field-spans.h
	@mkdir -p $(@D)
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_AMMO_DAMAGE_RANGE -DDOWNWELL_PREPARED_AMMO_DIVIDERS -o $@ $(AMIGA_RENDERER_TEST_SOURCES)
verify-prepared-ammo-dividers: build/prepared-ammo-renderer-test generate-amiga-game
	./build/prepared-ammo-renderer-test generated/amiga-game.dwap generated/amiga-hud.dwap

# Host-only feasibility/oracle for a rejected native interior-only experiment.
generated/amiga-ammo-segments-probe.h: tools/probe-ammo-segments.py generated/amiga-hud.dwap
	python3 tools/probe-ammo-segments.py
build/ammo-segments-test: tests/platform/amiga/AmigaAmmoSegmentsTest.cpp src/platform/amiga/AmigaRenderer.c $(wildcard src/core/Ammo*.h) src/platform/amiga/AmigaRenderer.h generated/amiga-ammo-segments-probe.h
	@mkdir -p $(@D)
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaAmmoSegmentsTest.cpp src/platform/amiga/AmigaRenderer.c
.PHONY: verify-ammo-segment-probe
verify-ammo-segment-probe: build/ammo-segments-test
	./build/ammo-segments-test generated/amiga-hud.dwap

# Host-only full-gauge representation budget; deliberately not a native bank.
generated/amiga-ammo-complete-probe.json: tools/probe-ammo-complete.py generated/amiga-hud.dwap
	python3 tools/probe-ammo-complete.py
build/ammo-complete-witness: tests/platform/amiga/AmigaAmmoCompleteWitness.cpp src/platform/amiga/AmigaRenderer.c $(wildcard src/core/Ammo*.h) src/platform/amiga/AmigaRenderer.h
	@mkdir -p $(@D)
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaAmmoCompleteWitness.cpp src/platform/amiga/AmigaRenderer.c
.PHONY: verify-ammo-complete-probe
verify-ammo-complete-probe: build/ammo-complete-witness generated/amiga-ammo-complete-probe.json
	./build/ammo-complete-witness generated/amiga-hud.dwap > build/ammo-complete-witness.txt
	python3 tools/probe-ammo-complete.py --verify build/ammo-complete-witness.txt

generated/amiga-ammo-factored.h: tools/probe-ammo-complete.py generated/amiga-hud.dwap
	python3 tools/probe-ammo-complete.py --factored
build/factored-ammo-renderer-test: $(AMIGA_RENDERER_TEST_SOURCES) $(wildcard src/platform/amiga/*.h) $(wildcard src/core/*.h) generated/amiga-ammo-dividers.h generated/downwell-field-spans.h generated/amiga-ammo-factored.h
	@mkdir -p $(@D)
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_AMMO_DAMAGE_RANGE -DDOWNWELL_PREPARED_AMMO_DIVIDERS -DDOWNWELL_AMMO_FACTORED=2 -o $@ $(AMIGA_RENDERER_TEST_SOURCES)
.PHONY: verify-factored-ammo-hud
verify-factored-ammo-hud: build/factored-ammo-renderer-test
	./build/factored-ammo-renderer-test generated/amiga-game.dwap generated/amiga-hud.dwap

.PHONY: generate-amiga-results verify-amiga-results
.PHONY: generate-amiga-shop verify-amiga-shop
generate-amiga-shop:
	tools/generate-amiga-hud.py --profile shop
verify-amiga-shop: generate-amiga-shop
	tools/generate-amiga-hud.py --profile shop --check
	python3 tests/shop-pack-test.py
test: verify-amiga-shop
generate-amiga-results:
	tools/generate-amiga-hud.py --profile results

verify-amiga-results: generate-amiga-results
	tools/generate-amiga-hud.py --profile results --check
	python3 tests/results-pack-test.py

verify-amiga-hud: generate-amiga-hud
	tools/generate-amiga-hud.py --check

generate-amiga-game: generate-amiga-clear
	python3 tests/projectile-art-test.py
	tools/generate-amiga-hud.py --profile game
	python3 tests/scene-asset-admission-test.py

.PHONY: test-scene-asset-admission
.PHONY: test-quiet-fall-witness
test-quiet-fall-witness:
	python3 tests/quiet-fall-witness-test.py
test: test-quiet-fall-witness

.PHONY: test-quiet-window-guard
test-quiet-window-guard:
	python3 tests/quiet-window-guard-test.py
test: test-quiet-window-guard
test-scene-asset-admission: generate-amiga-game generate-amiga-clear
	python3 tests/scene-asset-admission-test.py
test: test-scene-asset-admission

.PHONY: generate-amiga-clear verify-amiga-clear
generate-amiga-clear:
	tools/generate-amiga-hud.py --profile clear
verify-amiga-clear: generate-amiga-clear
	tools/generate-amiga-hud.py --profile clear --check
	python3 tests/clear-pack-test.py

build/amiga-clear-bank-test: tests/platform/amiga/AmigaClearBankTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: test-amiga-clear-bank
test-amiga-clear-bank: generate-amiga-clear build/amiga-clear-bank-test
	./build/amiga-clear-bank-test generated/amiga-clear.dwap
test: test-amiga-clear-bank
test: test-amiga-clear-border-cache
test: test-amiga-clear-overlay
test: test-level-clear-trig
test: test-level-clear-raster
test: test-level-clear-heading-pixels
.PHONY: test-level-clear-heading-pixels
build/level-clear-heading-pixels-test: tests/core/LevelClearHeadingPixelsTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-level-clear-heading-pixels: build/level-clear-heading-pixels-test
	./build/level-clear-heading-pixels-test
.PHONY: test-level-clear-raster
build/level-clear-raster-test: tests/core/LevelClearRasterTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-level-clear-raster: build/level-clear-raster-test
	./build/level-clear-raster-test
.PHONY: test-level-clear-trig
build/level-clear-trig-test: tests/core/LevelClearTrigTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-level-clear-trig: build/level-clear-trig-test
	./build/level-clear-trig-test
test: test-amiga-clear-scene-assets
.PHONY: test-amiga-clear-scene-assets
build/amiga-clear-scene-assets-test: tests/platform/amiga/AmigaClearSceneAssetsTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-amiga-clear-scene-assets: generate-amiga-clear generated/amiga-game.exit-mask generated/amiga-game.selection-mask generated/amiga-hud.dwap build/amiga-clear-scene-assets-test
	./build/amiga-clear-scene-assets-test generated/amiga-clear.dwap generated/amiga-game.exit-mask generated/amiga-hud.dwap
	./build/amiga-clear-scene-assets-test generated/amiga-clear.dwap generated/amiga-game.selection-mask generated/amiga-hud.dwap
generated/amiga-game.exit-mask: tools/generate-amiga-hud.py
	tools/generate-amiga-hud.py --profile game
generated/amiga-game.selection-mask: generated/amiga-game.exit-mask
	@test -f $@ || tools/generate-amiga-hud.py --profile game
test: test-amiga-pen-blit
.PHONY: test-amiga-pen-blit
build/amiga-pen-blit-test: tests/platform/amiga/AmigaPenBlitTest.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaPenBlitTest.cpp src/platform/amiga/AmigaRenderer.c
test-amiga-pen-blit: build/amiga-pen-blit-test
	./build/amiga-pen-blit-test
.PHONY: test-amiga-clear-overlay
build/amiga-selection-layer-test: tests/platform/amiga/AmigaSelectionLayerTest.cpp tests/platform/amiga/SelectionBitmapStub.cpp src/platform/amiga/AmigaSelectionLayer.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp %.c,$^)
.PHONY: test-amiga-selection-layer
test-amiga-selection-layer: generate-amiga-clear build/amiga-selection-layer-test
	./build/amiga-selection-layer-test generated/amiga-clear.dwap
test: test-amiga-selection-layer

.PHONY: test-amiga-style-scene
build/amiga-style-scene-test: tests/platform/amiga/AmigaStyleSceneTest.cpp src/platform/amiga/AmigaStyleScene.cpp src/platform/amiga/AmigaSceneRuntime.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_REFERENCE_STYLE_SPRITES -o $@ $(filter %.cpp,$^)
test-amiga-style-scene: build/amiga-style-scene-test
	./build/amiga-style-scene-test
test: test-amiga-style-scene
.PHONY: test-amiga-style-dma
test-amiga-style-dma: generate-amiga-game
	python3 tests/amiga-style-dma-test.py
test: test-amiga-style-dma

.PHONY: test-amiga-splash-scenes
build/amiga-splash-scenes-test: tests/platform/amiga/AmigaSplashScenesTest.cpp tests/platform/amiga/SelectionBitmapStub.cpp src/platform/amiga/AmigaPublisherScene.cpp src/platform/amiga/AmigaCreditsScene.cpp src/platform/amiga/AmigaControlsScene.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-amiga-splash-scenes: build/amiga-splash-scenes-test
	./build/amiga-splash-scenes-test
test: test-amiga-splash-scenes

.PHONY: test-amiga-garden-scene
build/amiga-garden-scene-test: tests/platform/amiga/AmigaGardenSceneTest.cpp src/platform/amiga/AmigaGardenScene.cpp src/platform/amiga/AmigaSceneRuntime.cpp src/platform/amiga/AmigaMenuScroll.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-amiga-garden-scene: build/amiga-garden-scene-test
	./build/amiga-garden-scene-test
test: test-amiga-garden-scene

.PHONY: test-amiga-gameplay-rows
build/amiga-gameplay-effects-test: tests/platform/amiga/AmigaGameplayEffectsTest.cpp src/platform/amiga/AmigaGameplayEffects.cpp src/platform/amiga/AmigaGameplayRows.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp %.c,$^)
.PHONY: test-amiga-gameplay-effects
test-amiga-gameplay-effects: build/amiga-gameplay-effects-test
	./build/amiga-gameplay-effects-test
test: test-amiga-gameplay-effects

build/amiga-gameplay-rows-test: CXXFLAGS += -DDOWNWELL_ROW_SYNC_AUDIT
build/amiga-gameplay-rows-test: tests/platform/amiga/AmigaGameplayRowsTest.cpp src/platform/amiga/AmigaGameplayRows.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-amiga-gameplay-rows: build/amiga-gameplay-rows-test
	./build/amiga-gameplay-rows-test
test: test-amiga-gameplay-rows

.PHONY: test-amiga-gameplay-scene
build/amiga-frame-budget-test: tests/platform/amiga/AmigaFrameBudgetTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: test-amiga-frame-budget
test-amiga-frame-budget: build/amiga-frame-budget-test
	./build/amiga-frame-budget-test
test: test-amiga-frame-budget
build/amiga-gameplay-scene-test: tests/platform/amiga/AmigaGameplaySceneTest.cpp src/platform/amiga/AmigaGameplayScene.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-amiga-gameplay-scene: build/amiga-gameplay-scene-test
	./build/amiga-gameplay-scene-test
test: test-amiga-gameplay-scene

.PHONY: test-amiga-upgrade-display
build/amiga-upgrade-display-test: tests/platform/amiga/AmigaUpgradeDisplayTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $<
test-amiga-upgrade-display: build/amiga-upgrade-display-test
	./build/amiga-upgrade-display-test
test: test-amiga-upgrade-display

.PHONY: test-generation-probes
build/generation-probes-off-test: tests/core/ProfileGenerationTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
build/generation-probes-coarse-test: tests/core/ProfileGenerationTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_PROBES -o $@ $<
build/generation-probes-detail-test: tests/core/ProfileGenerationTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_PROBES -DDOWNWELL_GENERATION_DETAIL -o $@ $<
test-generation-probes: build/generation-probes-off-test build/generation-probes-coarse-test build/generation-probes-detail-test
	./build/generation-probes-off-test
	./build/generation-probes-coarse-test
	./build/generation-probes-detail-test
test: test-generation-probes

.PHONY: test-amiga-gameplay-actors
build/amiga-gameplay-actors-test: tests/platform/amiga/AmigaGameplayActorsTest.cpp tests/platform/amiga/SelectionBitmapStub.cpp src/platform/amiga/AmigaGameplayActors.cpp src/platform/amiga/AmigaGameplayRows.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp %.c,$^)
test-amiga-gameplay-actors: build/amiga-gameplay-actors-test
	./build/amiga-gameplay-actors-test
test: test-amiga-gameplay-actors

.PHONY: test-amiga-damage-text
build/amiga-hud-text-test: tests/platform/amiga/AmigaHudTextTest.cpp src/platform/amiga/AmigaRenderer.c src/platform/amiga/AmigaHudText.h $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaHudTextTest.cpp src/platform/amiga/AmigaRenderer.c
.PHONY: test-amiga-hud-text
test-amiga-hud-text: build/amiga-hud-text-test generated/amiga-hud.dwap generated/amiga-game.dwap
	./build/amiga-hud-text-test generated/amiga-hud.dwap generated/amiga-game.dwap
test: test-amiga-hud-text

build/generate-amiga-health-art: tools/generate-amiga-health-art.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tools/generate-amiga-health-art.cpp src/platform/amiga/AmigaRenderer.c
generated/amiga-health-art.h: build/generate-amiga-health-art tools/generate-amiga-health-art.py generated/amiga-hud.dwap
	python3 tools/generate-amiga-health-art.py
$(AMIGA_RENDERER_TEST_TARGET): generated/amiga-health-art.h
.PHONY: test-amiga-health-art
test-amiga-health-art: $(AMIGA_RENDERER_TEST_TARGET) generated/amiga-game.dwap generated/amiga-hud.dwap
	./$(AMIGA_RENDERER_TEST_TARGET) generated/amiga-game.dwap generated/amiga-hud.dwap
	python3 tools/generate-amiga-health-art.py --check
test: test-amiga-health-art

build/generate-amiga-hud-text: tools/generate-amiga-hud-text.cpp src/platform/amiga/AmigaHudText.h src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tools/generate-amiga-hud-text.cpp src/platform/amiga/AmigaRenderer.c
generated/amiga-hud-text.h: build/generate-amiga-hud-text tools/generate-amiga-hud-text.py generated/amiga-hud.dwap generated/amiga-game.dwap
	python3 tools/generate-amiga-hud-text.py
build/amiga-prepared-hud-test: tests/platform/amiga/AmigaPreparedHudTextTest.cpp src/platform/amiga/AmigaPreparedHudText.h generated/amiga-hud-text.h $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaPreparedHudTextTest.cpp src/platform/amiga/AmigaRenderer.c
.PHONY: test-amiga-prepared-hud
test-amiga-prepared-hud: build/amiga-prepared-hud-test
	./build/amiga-prepared-hud-test generated/amiga-hud.dwap generated/amiga-game.dwap
	python3 tools/generate-amiga-hud-text.py --check
test: test-amiga-prepared-hud

generated/amiga-damage-text.h: tools/generate-amiga-damage-text.py generated/amiga-game.dwap
	python3 tools/generate-amiga-damage-text.py
build/amiga-damage-text-test: tests/platform/amiga/AmigaDamageTextTest.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS) generated/amiga-damage-text.h
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp %.c,$^)
test-amiga-damage-text: build/amiga-damage-text-test generated/amiga-game.dwap
	./build/amiga-damage-text-test generated/amiga-game.dwap
	python3 tools/generate-amiga-damage-text.py --check
test: test-amiga-damage-text

.PHONY: test-amiga-gameplay-sprite-assignment
.PHONY: test-amiga-frame-decoder
test-amiga-frame-decoder:
	python3 tests/amiga-frame-decoder-test.py
test: test-amiga-frame-decoder
build/amiga-gameplay-sprite-assignment-test: tests/platform/amiga/AmigaGameplaySpriteAssignmentTest.cpp src/platform/amiga/AmigaGameplaySpriteAssignment.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-amiga-gameplay-sprite-assignment: build/amiga-gameplay-sprite-assignment-test
	./build/amiga-gameplay-sprite-assignment-test
test: test-amiga-gameplay-sprite-assignment

GAMEPLAY_DRAW_LIST_TEST_SOURCES := tests/platform/amiga/AmigaGameplayDrawListTest.cpp src/platform/amiga/AmigaGameplayDrawList.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp src/core/ObjectPack.cpp src/core/RoomPack.cpp
CURRENCY_PROMOTION_TEST_SOURCES := tests/platform/amiga/AmigaCurrencyPromotionTest.cpp $(filter-out tests/platform/amiga/AmigaSceneRuntimeTest.cpp,$(AMIGA_SCENE_TEST_SOURCES)) src/platform/amiga/AmigaGameplayDrawList.cpp src/platform/amiga/AmigaRenderer.c
build/amiga-currency-promotion-test: $(CURRENCY_PROMOTION_TEST_SOURCES) tests/platform/amiga/AmigaSceneRuntimeTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(CURRENCY_PROMOTION_TEST_SOURCES)
build/amiga-currency-promotion-reference-test: $(CURRENCY_PROMOTION_TEST_SOURCES) tests/platform/amiga/AmigaSceneRuntimeTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_REFERENCE_SHADE_PARTITION -o $@ $(CURRENCY_PROMOTION_TEST_SOURCES)
build/amiga-currency-bobs-reference-test: $(CURRENCY_PROMOTION_TEST_SOURCES) tests/platform/amiga/AmigaSceneRuntimeTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_REFERENCE_CURRENCY_BOBS -o $@ $(CURRENCY_PROMOTION_TEST_SOURCES)
.PHONY: test-amiga-currency-promotion
build/packed-mask-intersection-test: tests/core/PackedMaskIntersectionTest.cpp src/core/PackedMaskIntersection.h src/core/BinaryEffectSurface.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-amiga-currency-promotion: build/amiga-currency-promotion-test build/amiga-currency-promotion-reference-test build/amiga-currency-bobs-reference-test build/packed-mask-intersection-test
	./build/packed-mask-intersection-test
	./build/amiga-currency-promotion-test
	./build/amiga-currency-promotion-test --overflow
	./build/amiga-currency-bobs-reference-test --overflow
	./build/amiga-currency-promotion-reference-test
	./build/amiga-currency-promotion-reference-test --overflow
test: test-amiga-currency-promotion
.PHONY: generate-overflow-witness
generate-overflow-witness: build/amiga-currency-promotion-test
	python3 tools/generate-overflow-witness.py
.PHONY: test-amiga-gameplay-draw-list
build/amiga-gameplay-draw-list-test: $(GAMEPLAY_DRAW_LIST_TEST_SOURCES) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(GAMEPLAY_DRAW_LIST_TEST_SOURCES)
build/amiga-gameplay-draw-list-replay-test: $(GAMEPLAY_DRAW_LIST_TEST_SOURCES) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -DDOWNWELL_REPLAY_VERIFY -o $@ $(GAMEPLAY_DRAW_LIST_TEST_SOURCES)
test-amiga-gameplay-draw-list: build/amiga-gameplay-draw-list-test build/amiga-gameplay-draw-list-replay-test
	./build/amiga-gameplay-draw-list-test
	./build/amiga-gameplay-draw-list-replay-test
.PHONY: test-red-ball-art
test-red-ball-art: generate-amiga-game
	python3 tests/red-ball-art-test.py
test: test-red-ball-art
test: test-amiga-gameplay-draw-list

.PHONY: test-amiga-landing-index
build/amiga-landing-index-test: tests/platform/amiga/AmigaLandingIndexTest.cpp src/platform/amiga/AmigaLandingIndex.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
test-amiga-landing-index: build/amiga-landing-index-test
	./build/amiga-landing-index-test
test: test-amiga-landing-index

.PHONY: test-amiga-terrain-raster
build/amiga-terrain-raster-test: tests/platform/amiga/AmigaTerrainRasterTest.cpp tests/platform/amiga/SelectionBitmapStub.cpp src/platform/amiga/AmigaTerrainRaster.cpp src/platform/amiga/AmigaGameplayRows.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp,$^)
test-amiga-terrain-raster: build/amiga-terrain-raster-test
	./build/amiga-terrain-raster-test
test: test-amiga-terrain-raster

.PHONY: test-amiga-style-retained
build/amiga-style-retained-test: tests/platform/amiga/AmigaStyleRetainedTest.cpp tests/platform/amiga/SelectionBitmapStub.cpp src/platform/amiga/AmigaSelectionLayer.cpp src/platform/amiga/AmigaSceneRuntime.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $(filter %.cpp %.c,$^)
test-amiga-style-retained: generate-amiga-game build/amiga-style-retained-test
	./build/amiga-style-retained-test generated/amiga-front.dwap
test: test-amiga-style-retained

.PHONY: test-amiga-clear-oracle-buffers
build/amiga-clear-oracle-buffers-test: tests/platform/amiga/AmigaClearOracleBuffersTest.cpp src/platform/amiga/AmigaClearOracleBuffers.h src/platform/amiga/AmigaRenderer.h tests/platform/amiga/allocation-compat/exec/memory.h tests/platform/amiga/allocation-compat/proto/exec.h
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/allocation-compat -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ $<
test-amiga-clear-oracle-buffers: build/amiga-clear-oracle-buffers-test
	./build/amiga-clear-oracle-buffers-test
test: test-amiga-clear-oracle-buffers

build/amiga-clear-overlay-test: tests/platform/amiga/AmigaClearOverlayTest.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaClearOverlayTest.cpp src/platform/amiga/AmigaRenderer.c
test-amiga-clear-overlay: generate-amiga-clear build/amiga-clear-overlay-test
	./build/amiga-clear-overlay-test generated/amiga-clear.dwap
.PHONY: test-amiga-clear-border-cache
build/amiga-clear-border-cache-test: tests/platform/amiga/AmigaClearBorderCacheTest.cpp src/platform/amiga/AmigaRenderer.c $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc -Itests/platform/amiga/compat $(CXXFLAGS) -o $@ tests/platform/amiga/AmigaClearBorderCacheTest.cpp src/platform/amiga/AmigaRenderer.c
test-amiga-clear-border-cache: generate-amiga-clear build/amiga-clear-border-cache-test
	./build/amiga-clear-border-cache-test generated/amiga-clear.dwap

generated/default-shader.json: tools/export-default-shader.sh tools/export-default-shader.csx data/data.win
	bash tools/export-default-shader.sh

.PHONY: verify-default-shader
verify-default-shader: generated/default-shader.json
	python3 tools/verify-default-shader.py
	python3 tools/verify-runner-vertex-shader.py

.PHONY: generate-effect-stamps test-effect-stamps
test: test-effect-stamps
generate-effect-stamps: generate-amiga-game
	python3 tools/generate-effect-stamps.py
test-effect-stamps: generate-effect-stamps build/amiga-effect-stamp-bank-test
	python3 tools/generate-effect-stamps.py --check
	python3 tests/effect-stamps-test.py
	./build/amiga-effect-stamp-bank-test
build/amiga-effect-stamp-bank-test: tests/platform/amiga/AmigaEffectStampBankTest.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<

.PHONY: test-amiga-immutable-upload
.PHONY: test-amiga-transient-bank
test: test-amiga-transient-bank
test-amiga-transient-bank: build/amiga-transient-bank-test
	./build/amiga-transient-bank-test
build/amiga-transient-bank-test: tests/platform/amiga/AmigaTransientBankTest.cpp src/platform/amiga/AmigaTransientBank.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

test-amiga-immutable-upload: build/amiga-immutable-upload-test
	./build/amiga-immutable-upload-test
build/amiga-immutable-upload-test: tests/platform/amiga/AmigaImmutableUploadTest.cpp src/platform/amiga/AmigaImmutableUpload.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

verify-amiga-game: generate-amiga-game verify-default-shader test-amiga-immutable-upload
	python3 tools/verify-runner-effect-blend.py
	python3 tools/verify-amiga-menu-sprites.py
	tools/generate-amiga-hud.py --profile game --check
	python3 tests/exit-mask-test.py
	python3 tests/entry-mask-test.py
	python3 tests/skeleton-art-test.py
	python3 tests/muzzle-art-test.py
	python3 tests/wall-impact-art-test.py
	python3 tests/blast-impact-art-test.py
	python3 tests/planar-packing-test.py

verify-native-data: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack generate-builder-pattern-pack $(STARTUP_TEST_TARGET)
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

verify-cross-replay: $(PORT_REPLAY_REFERENCE_TARGET)
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga REPLAY_VERIFY=1'
	bash -c '. amiga/env.sh && amiga/verify_replay.sh ../$(PORT_REPLAY_REFERENCE_TARGET)'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

.PHONY: verify-amiga-gameplay-cache
.PHONY: verify-amiga-gameplay-camera
.PHONY: verify-amiga-gameplay-shop
verify-amiga-gameplay-shop:
	bash amiga/verify_gameplay_shop.sh

verify-amiga-gameplay-camera:
	bash amiga/verify_gameplay_camera.sh

.PHONY: verify-amiga-memory
verify-amiga-memory:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_memory.gdb READY_MARKER=DOWNWELL_MEMORY_READY READY_DESCRIPTION="startup Chip RAM budget" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-30} amiga/verify_startup.sh'
.PHONY: verify-amiga-gameplay-bobs
verify-amiga-gameplay-bobs:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 SKIP_TO_GAME=1 SCENE_VERIFY=1 BOB_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_gameplay_bobs.gdb READY_MARKER=DOWNWELL_GAMEPLAY_BOBS_READY READY_DESCRIPTION="overflow actor oracle" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-45} amiga/verify_startup.sh'

verify-amiga-gameplay-cache:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 SKIP_TO_GAME=1 SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_gameplay_cache.gdb READY_MARKER=DOWNWELL_GAMEPLAY_CACHE_READY READY_DESCRIPTION="600-tick terrain cache oracle" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-45} amiga/verify_startup.sh'

.PHONY: verify-amiga-natural-route verify-amiga-clear-scene
verify-amiga-natural-route: verify-natural-exit
	bash -c '. amiga/env.sh; trap "$(MAKE) -C amiga clean && $(MAKE) -C amiga -j4" EXIT; $(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 SKIP_TO_GAME=1 NATURAL_VERIFY=1 && GDB_SOURCE=verify_natural.gdb READY_MARKER=DOWNWELL_NATURAL_READY READY_DESCRIPTION="natural cavern exit and reentry" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-360} amiga/verify_startup.sh'

verify-amiga-clear-scene:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 SKIP_TO_GAME=1 CLEAR_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_clear.gdb READY_MARKER=DOWNWELL_CLEAR_READY READY_DESCRIPTION="two native clears and third-level reentry" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-360} amiga/verify_startup.sh'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga -j4'
.PHONY: verify-amiga-exit-wipe
.PHONY: verify-amiga-entry-wipe
verify-amiga-entry-wipe:
	bash amiga/verify_entry_wipe.sh

verify-amiga-exit-wipe:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 SKIP_TO_GAME=1 EXIT_WIPE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_exit_wipe.gdb READY_MARKER=DOWNWELL_EXIT_WIPE_READY READY_DESCRIPTION="exit wipe DMA" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-45} amiga/verify_startup.sh'

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

verify-amiga-menu-wake-frame: $(AMIGA_MENU_TEST_TARGET)
	bash amiga/verify_menu_wake_frame.sh
	python3 tools/verify-amiga-garden-cache.py amiga/.run/menu-wake-frame.bin
	printf '%s  %s\n' '$(AMIGA_MENU_WAKE_FRAME_SHA256)' 'amiga/.run/menu-wake-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-menu-frame.py amiga/.run/menu-wake-frame.bin build/amiga-menu-wake-frame.png

.PHONY: verify-amiga-garden-frames
verify-amiga-menu-title-frame verify-amiga-menu-entry-frame: verify-amiga-garden-frames

verify-amiga-garden-frames: $(AMIGA_MENU_TEST_TARGET)
	bash amiga/verify_menu_wake_frame.sh all
	python3 tools/verify-amiga-garden-cache.py amiga/.run/menu-wake-frame.bin
	python3 tools/verify-amiga-garden-cache.py --scene title amiga/.run/menu-title-frame.bin
	python3 tools/verify-amiga-garden-cache.py --scene entry amiga/.run/menu-entry-frame.bin
	printf '%s  %s\n' '$(AMIGA_MENU_WAKE_FRAME_SHA256)' 'amiga/.run/menu-wake-frame.bin' | shasum -a 256 -c -
	printf '%s  %s\n' '$(AMIGA_MENU_TITLE_FRAME_SHA256)' 'amiga/.run/menu-title-frame.bin' | shasum -a 256 -c -
	printf '%s  %s\n' '$(AMIGA_MENU_ENTRY_FRAME_SHA256)' 'amiga/.run/menu-entry-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-menu-frame.py amiga/.run/menu-wake-frame.bin build/amiga-menu-wake-frame.png
	python3 tools/decode-amiga-menu-frame.py amiga/.run/menu-title-frame.bin build/amiga-menu-title-frame.png
	python3 tools/verify-amiga-entry-continuity.py

verify-amiga-gameplay-frame: build/capture-sdl-game
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_gameplay_frame.gdb READY_MARKER=DOWNWELL_GAMEPLAY_FRAME_READY READY_DESCRIPTION="first grounded gameplay raster" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_GAMEPLAY_FRAME_SHA256)' 'amiga/.run/gameplay-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-gameplay-frame.py amiga/.run/gameplay-frame.bin build/amiga-gameplay-frame.png
	@capture_dir=$$(mktemp -d build/landing-XXXXXX); \
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/capture-sdl-game "$$capture_dir" && \
	python3 tools/compare-gameplay-captures.py "$$capture_dir" build/amiga-gameplay-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

.PHONY: verify-amiga-gameplay-ammo-frame
.PHONY: verify-amiga-gameplay-laser-frame
verify-amiga-gameplay-laser-frame: build/capture-sdl-game
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga LASER_VERIFY=1 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_gameplay_ammo_frame.gdb READY_MARKER=DOWNWELL_GAMEPLAY_AMMO_FRAME_READY READY_DESCRIPTION="Laser Sight gameplay frame" VERIFY_TIMEOUT_SECONDS=45 amiga/verify_startup.sh'
	python3 tools/decode-amiga-gameplay-frame.py amiga/.run/gameplay-ammo-frame.bin build/amiga-laser-frame.png
	@capture_tick=$$(sed -n 's/^CAPTURE_STATE tick=\([0-9]*\).*/\1/p' amiga/.run/gdb-startup.log); \
	capture_dir=$$(mktemp -d build/laser-XXXXXX); \
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software DOWNWELL_CAPTURE_LASER=1 ./build/capture-sdl-game "$$capture_dir" "$$capture_tick" && \
	python3 tools/compare-gameplay-captures.py "$$capture_dir" build/amiga-laser-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

.PHONY: verify-amiga-gameplay-recharge-frame
verify-amiga-gameplay-recharge-frame: build/capture-sdl-game
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_gameplay_recharge_frame.gdb READY_MARKER=DOWNWELL_GAMEPLAY_RECHARGE_FRAME_READY READY_DESCRIPTION="gameplay gauge during recharge" amiga/verify_startup.sh'
	python3 tools/decode-amiga-gameplay-frame.py amiga/.run/gameplay-recharge-frame.bin build/amiga-gameplay-recharge-frame.png
	@capture_tick=$$(sed -n 's/^CAPTURE_STATE tick=\([0-9]*\).*/\1/p' amiga/.run/gdb-startup.log); \
	capture_dir=$$(mktemp -d build/recharge-XXXXXX); \
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/capture-sdl-game "$$capture_dir" "$$capture_tick" && \
	python3 tools/compare-gameplay-captures.py "$$capture_dir" build/amiga-gameplay-recharge-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-gameplay-ammo-frame: build/capture-sdl-game
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_gameplay_ammo_frame.gdb READY_MARKER=DOWNWELL_GAMEPLAY_AMMO_FRAME_READY READY_DESCRIPTION="gameplay gauge during a shot" amiga/verify_startup.sh'
	python3 tools/decode-amiga-gameplay-frame.py amiga/.run/gameplay-ammo-frame.bin build/amiga-gameplay-ammo-frame.png
	@capture_tick=$$(sed -n 's/^CAPTURE_STATE tick=\([0-9]*\).*/\1/p' amiga/.run/gdb-startup.log); \
	capture_dir=$$(mktemp -d build/ammo-XXXXXX); \
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/capture-sdl-game "$$capture_dir" "$$capture_tick" && \
	python3 tools/compare-gameplay-captures.py "$$capture_dir" build/amiga-gameplay-ammo-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-gameplay-active-frame: build/capture-sdl-game
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga clean && $(MAKE) -C amiga -j4" EXIT; $(MAKE) -C amiga clean; $(MAKE) -C amiga -j4 SCENE_DESCENT_VERIFY=1 SKIP_TO_GAME=1; GDB_SOURCE=verify_gameplay_active_frame.gdb READY_MARKER=DOWNWELL_GAMEPLAY_ACTIVE_FRAME_READY READY_DESCRIPTION="scrolled gameplay raster with an awake enemy" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-120} amiga/verify_startup.sh'
	python3 tools/decode-amiga-gameplay-frame.py amiga/.run/gameplay-active-frame.bin build/amiga-gameplay-active-frame.png
	@capture_tick=$$(sed -n 's/^CAPTURE_STATE tick=\([0-9]*\).*/\1/p' amiga/.run/gdb-startup.log); \
	capture_dir=$$(mktemp -d build/active-XXXXXX); \
	DOWNWELL_CAPTURE_DESCENT=1 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/capture-sdl-game "$$capture_dir" "$$capture_tick" && \
	python3 tools/compare-gameplay-captures.py "$$capture_dir" build/amiga-gameplay-active-frame.png

.PHONY: verify-amiga-gameplay-gemhigh-frame
verify-amiga-gameplay-gemhigh-frame: build/capture-sdl-game
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1 GEMHIGH_VERIFY=1 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-45} GDB_SOURCE=verify_gameplay_gemhigh_frame.gdb READY_MARKER=DOWNWELL_GAMEPLAY_GEMHIGH_FRAME_READY READY_DESCRIPTION="visible gem-high projectile" amiga/verify_startup.sh'
	python3 tools/decode-amiga-gameplay-frame.py amiga/.run/gameplay-gemhigh-frame.bin build/amiga-gameplay-gemhigh-frame.png
	@capture_tick=$$(sed -n 's/^CAPTURE_STATE tick=\([0-9]*\).*/\1/p' amiga/.run/gdb-startup.log); \
	capture_dir=$$(mktemp -d build/gemhigh-XXXXXX); \
	DOWNWELL_CAPTURE_GEM_HIGH=1 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/capture-sdl-game "$$capture_dir" "$$capture_tick" && \
	python3 tools/compare-gameplay-captures.py "$$capture_dir" build/amiga-gameplay-gemhigh-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-style-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_style_frame.gdb READY_MARKER=DOWNWELL_STYLE_FRAME_READY READY_DESCRIPTION="stable style-select planar frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_STYLE_FRAME_SHA256)' 'amiga/.run/style-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-style-frame.py amiga/.run/style-frame.bin build/amiga-style-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-locked-style-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_locked_style_frame.gdb READY_MARKER=DOWNWELL_LOCKED_STYLE_FRAME_READY READY_DESCRIPTION="stable locked-style planar frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_LOCKED_STYLE_FRAME_SHA256)' 'amiga/.run/locked-style-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-style-frame.py amiga/.run/locked-style-frame.bin build/amiga-locked-style-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-selected-style-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_selected_style_frame.gdb READY_MARKER=DOWNWELL_SELECTED_STYLE_FRAME_READY READY_DESCRIPTION="mid-transition selected-style planar frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_SELECTED_STYLE_FRAME_SHA256)' 'amiga/.run/selected-style-frame.bin' | shasum -a 256 -c -
	python3 tools/verify-amiga-style-mask.py --selected
	python3 tools/decode-amiga-style-frame.py amiga/.run/selected-style-frame.bin build/amiga-selected-style-frame.png --mask amiga/.run/selected-style-mask.bin
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-splash-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_splash_frame.gdb READY_MARKER=DOWNWELL_SPLASH_FRAME_READY READY_DESCRIPTION="complete Devolver publisher planar frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_SPLASH_FRAME_SHA256)' 'amiga/.run/splash-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-style-frame.py amiga/.run/splash-frame.bin build/amiga-splash-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-controls-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_controls_frame.gdb READY_MARKER=DOWNWELL_CONTROLS_FRAME_READY READY_DESCRIPTION="complete controls splash planar frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_CONTROLS_FRAME_SHA256)' 'amiga/.run/controls-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-style-frame.py amiga/.run/controls-frame.bin build/amiga-controls-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-credits-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_credits_frame.gdb READY_MARKER=DOWNWELL_CREDITS_FRAME_READY READY_DESCRIPTION="complete wide credits splash frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_CREDITS_FRAME_SHA256)' 'amiga/.run/credits-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-wide-frame.py amiga/.run/credits-frame.bin build/amiga-credits-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

.PHONY: verify-amiga-style-admission
verify-amiga-style-admission:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0" EXIT; $(MAKE) -C amiga -j4 SKIP_INTRO=1 SKIP_TO_GAME=0 SKIP_LEVEL=0; GDB_SOURCE=verify_style_admission.gdb READY_MARKER=DOWNWELL_STYLE_ADMISSION_READY READY_DESCRIPTION="direct style scene admission" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-45} amiga/verify_startup.sh'

verify-amiga-scenes:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 SCENE_DESCENT_VERIFY=0 SCENE_VERIFY=0" EXIT; $(MAKE) -C amiga clean; $(MAKE) -C amiga -j4 SCENE_DESCENT_VERIFY=1 SKIP_TO_GAME=0 SKIP_INTRO=0 SKIP_LEVEL=0; GDB_SOURCE=verify_scenes.gdb READY_MARKER=DOWNWELL_GAMEPLAY_ACTIVE READY_DESCRIPTION="complete native scene sequence through scrolling gameplay and enemy activation" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-180} amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_SPLASH_FRAME_SHA256)' 'amiga/.run/splash-frame.bin' | shasum -a 256 -c -
	printf '%s  %s\n' '$(AMIGA_CREDITS_FRAME_SHA256)' 'amiga/.run/credits-frame.bin' | shasum -a 256 -c -
	printf '%s  %s\n' '$(AMIGA_CONTROLS_FRAME_SHA256)' 'amiga/.run/controls-frame.bin' | shasum -a 256 -c -

.PHONY: verify-amiga-results-restart
.PHONY: verify-amiga-pattern-failure
verify-amiga-pattern-failure:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga -j4 STRIP=1" EXIT; $(MAKE) -C amiga -j4 SKIP_TO_GAME=1 STRIP=1; GDB_SOURCE=verify_builder_patterns_missing.gdb READY_MARKER=DOWNWELL_PATTERN_FAILURE_READY READY_DESCRIPTION="missing builder bank fails with return code 20" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-60} amiga/verify_startup.sh'

verify-amiga-results-restart:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga clean && $(MAKE) -C amiga -j4 STRIP=1" EXIT; $(MAKE) -C amiga clean; $(MAKE) -C amiga -j4 RESULTS_RESTART_VERIFY=1 SKIP_TO_GAME=1 STRIP=1; GDB_SOURCE=verify_results_restart.gdb READY_MARKER=DOWNWELL_RESULTS_RESTART_READY READY_DESCRIPTION="native scene admission across all results destinations and gameplay reentry" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-180} amiga/verify_startup.sh'

.PHONY: verify-amiga-encounter-failure
.PHONY: verify-amiga-encounter-reentry
.PHONY: verify-amiga-encounter-clear
verify-amiga-encounter-clear:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga clean && $(MAKE) -C amiga -j4" EXIT; $(MAKE) -C amiga clean; $(MAKE) -C amiga -f Makefile -f scene_collision_candidate.mk -j4 CLEAR_VERIFY=1 SKIP_TO_GAME=1 STRIP=1; mkdir -p tmp/encounter-clear-candidate; cp amiga/out/Downwell.exe amiga/out/Downwell.elf tmp/encounter-clear-candidate/; GDB_SOURCE=verify_encounter_clear.gdb READY_MARKER=DOWNWELL_CLEAR_READY READY_DESCRIPTION="native encounter session across two level completions" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-600} amiga/verify_startup.sh'

verify-amiga-encounter-reentry:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga clean && $(MAKE) -C amiga -j4" EXIT; $(MAKE) -C amiga clean; $(MAKE) -C amiga -f Makefile -f scene_collision_candidate.mk -j4 RESULTS_RESTART_VERIFY=1 SKIP_TO_GAME=1 STRIP=1; GDB_SOURCE=verify_encounter_reentry.gdb READY_MARKER=DOWNWELL_RESULTS_RESTART_READY READY_DESCRIPTION="production encounter session across Retry and room reentry" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-180} amiga/verify_startup.sh'

.PHONY: verify-amiga-encounter-update-failure
verify-amiga-encounter-update-failure:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga clean && $(MAKE) -C amiga -j4" EXIT; $(MAKE) -C amiga clean; $(MAKE) -C amiga -f Makefile -f encounter_collision_verify.mk -j4 FPSCOUNT=1 PROFILE_AUTORUN=1 DETERMINISTIC=1 SKIP_LEVEL=1 STRIP=1; GDB_SOURCE=verify_encounter_update_failure.gdb READY_MARKER=DOWNWELL_ENCOUNTER_UPDATE_FAILURE_READY READY_DESCRIPTION="terminal native collision callback failure" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-90} amiga/verify_startup.sh'

verify-amiga-encounter-failure:
	bash -c 'set -e; . amiga/env.sh; trap "$(MAKE) -C amiga -j4 STRIP=1" EXIT; $(MAKE) -C amiga -j4 RESULTS_RESTART_VERIFY=1 SKIP_TO_GAME=1 STRIP=1; GDB_SOURCE=verify_encounter_failure.gdb READY_MARKER=DOWNWELL_ENCOUNTER_FAILURE_READY READY_DESCRIPTION="terminal encounter failure during native Draw" VERIFY_TIMEOUT_SECONDS=$${VERIFY_TIMEOUT_SECONDS:-120} amiga/verify_startup.sh'

amiga:
	@bash -c '. amiga/env.sh && $(MAKE) -C amiga'

clean:
	$(RM) -r build
	$(MAKE) -C amiga clean

build/capture-sdl-landing: tools/capture-sdl-landing.cpp $(filter-out src/main.cpp,$(SOURCES)) $(HOST_CHECK_HEADERS) src/platform/sdl/PlatformSDL.h
	@mkdir -p build
	$(CXX) -Isrc $(SDL_CFLAGS) $(CXXFLAGS) -o $@ $(filter %.cpp,$^) $(SDL_LDFLAGS)

build/capture-sdl-game: tools/capture-sdl-landing.cpp $(filter-out src/main.cpp,$(SOURCES)) $(HOST_CHECK_HEADERS) src/platform/sdl/PlatformSDL.h
	@mkdir -p build
	$(CXX) -Isrc $(SDL_CFLAGS) $(CXXFLAGS) -DDOWNWELL_SKIP_TO_GAME -o $@ $(filter %.cpp,$^) $(SDL_LDFLAGS)

build/profile-sdl-game: tools/profile-sdl-game.cpp $(filter-out src/main.cpp,$(SOURCES)) $(HOST_CHECK_HEADERS) src/platform/sdl/PlatformSDL.h src/platform/sdl/SpriteVisibility.h
	@mkdir -p build
	$(CXX) -Isrc $(SDL_CFLAGS) $(CXXFLAGS) -DDOWNWELL_SKIP_TO_GAME -o $@ $(filter %.cpp,$^) $(SDL_LDFLAGS)

.PHONY: verify-runner-camera-view
verify-runner-camera-view:
	python3 tools/verify-runner-camera-view.py

.PHONY: verify-sdl-sprite-visibility
test: verify-sdl-sprite-visibility
verify-sdl-sprite-visibility:
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o build/sdl-sprite-visibility-test tests/platform/SDLSpriteVisibilityTest.cpp
	./build/sdl-sprite-visibility-test

build/live-collision-lifecycle-test: tests/core/LiveCollisionLifecycleTest.cpp $(filter-out tests/core/StartupRuntimeTest.cpp,$(STARTUP_TEST_SOURCES)) $(HOST_CHECK_HEADERS) generated/smoke-trig.inc
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(filter %.cpp,$^)

.PHONY: verify-live-collision-lifecycle
test: verify-live-collision-lifecycle
verify-live-collision-lifecycle: generate-room-pack generate-object-pack generate-sprite-pack generate-collision-masks generate-builder-pattern-pack build/live-collision-lifecycle-test
	./build/live-collision-lifecycle-test

.PHONY: verify-source-shot-owners
test: verify-source-shot-owners
verify-source-shot-owners: generate-room-pack generate-object-pack generate-sprite-pack generate-collision-masks generate-builder-pattern-pack build/live-collision-lifecycle-test build/gameplay-collision-owner-test
	python3 tools/verify-source-shot-owners.py

build/capture-projectile-art: tools/capture-projectile-art.cpp src/platform/sdl/PlatformSDL.cpp src/core/SpritePack.cpp src/core/FontPack.cpp $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(SDL_CFLAGS) $(CXXFLAGS) -o $@ $(filter %.cpp,$^) $(SDL_LDFLAGS)

.PHONY: verify-projectile-art
test: verify-projectile-art
verify-projectile-art: generate-sprite-pack build/capture-projectile-art
	python3 tests/projectile-art-test.py
	@mkdir -p captures
	@capture_dir=$$(mktemp -d captures/projectile-art-XXXXXX); \
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/capture-projectile-art "$$capture_dir" && \
	python3 tests/projectile-raster-test.py "$$capture_dir"

.PHONY: verify-runner-mask-point
test: verify-runner-mask-point
verify-runner-mask-point:
	@mkdir -p build
	python3 tools/verify-runner-mask-point.py
	$(CXX) -Isrc $(CXXFLAGS) -o build/runner-mask-point-test tests/core/RunnerMaskPointTest.cpp
	./build/runner-mask-point-test
	$(CXX) -Isrc $(CXXFLAGS) -o build/runner-point-candidate-test tests/core/RunnerPointCandidateTest.cpp
	./build/runner-point-candidate-test
	$(CXX) -Isrc $(CXXFLAGS) -o build/runner-point-query-test tests/core/RunnerPointQueryTest.cpp
	./build/runner-point-query-test

.PHONY: verify-wall-background
test: verify-wall-background
verify-wall-background:
	@mkdir -p build
	python3 tools/verify-wall-background.py
	$(CXX) -Isrc $(CXXFLAGS) -o build/wall-background-test tests/core/WallBackgroundTilesTest.cpp
	./build/wall-background-test

.PHONY: verify-spatial-publication-counters
test: verify-spatial-publication-counters
verify-spatial-publication-counters: build/spatial-publication-counters-test
	./build/spatial-publication-counters-test
build/spatial-publication-counters-test: tests/core/SpatialPublicationCountersTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: verify-replay-frame-trace
test: verify-replay-frame-trace
verify-replay-frame-trace: build/replay-frame-trace-test
	./build/replay-frame-trace-test
build/replay-frame-trace-test: tests/core/ReplayFrameTraceTest.cpp src/core/ReplayFrameTrace.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: test-generation-timing
test: test-prepared-terrain-birth test-runner-alarm-projection
.PHONY: test-runner-alarm-projection
test-runner-alarm-projection: build/runner-alarm-projection-test
	./build/runner-alarm-projection-test
build/runner-alarm-projection-test: tests/core/RunnerAlarmProjectionTest.cpp src/core/RunnerAlarmProjection.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<
.PHONY: test-prepared-terrain-birth
test-prepared-terrain-birth: build/prepared-terrain-birth-test
	./build/prepared-terrain-birth-test
build/prepared-terrain-birth-test: tests/core/PreparedTerrainBirthBoundsTest.cpp src/core/PreparedTerrainBirthBounds.h src/core/ObjectPack.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/PreparedTerrainBirthBoundsTest.cpp src/core/ObjectPack.cpp
test: test-generation-timing
test-generation-timing: build/generation-timing-test build/generation-timing-disabled-test build/generation-timing-birth-test build/generation-timing-layer-test
	./build/generation-timing-test
	./build/generation-timing-disabled-test
	./build/generation-timing-birth-test
	./build/generation-timing-layer-test
build/generation-timing-birth-test: tests/core/GenerationTimingTest.cpp src/core/Profile.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_FPSCOUNT -DDOWNWELL_GENERATION_TIMING -DDOWNWELL_GENERATION_TIMING_PHASE=profileInstanceBirth -o $@ $<
build/generation-timing-layer-test: tests/core/GenerationTimingTest.cpp src/core/Profile.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_FPSCOUNT -DDOWNWELL_GENERATION_TIMING -DDOWNWELL_GENERATION_TIMING_PHASE=profileInstanceLayerAttach -o $@ $<
build/generation-timing-test: tests/core/GenerationTimingTest.cpp src/core/Profile.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_FPSCOUNT -DDOWNWELL_GENERATION_TIMING -o $@ $<
build/generation-timing-disabled-test: tests/core/GenerationTimingTest.cpp src/core/Profile.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_FPSCOUNT -o $@ $<

.PHONY: test-phase-sampling
test: test-phase-sampling
test-phase-sampling: build/phase-sampling-test
	./build/phase-sampling-test
build/phase-sampling-test: tests/core/PhaseSamplingTest.cpp src/core/Profile.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_FPSCOUNT -DDOWNWELL_PHASE_SAMPLING -o $@ $<

.PHONY: test-compact-membership
test: test-compact-membership
test-compact-membership: build/compact-membership-test
	./build/compact-membership-test
build/compact-membership-test: tests/core/CompactMembershipTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: test-direct-layer-identity
test: test-direct-layer-identity
test-direct-layer-identity: build/direct-layer-identity-test
	./build/direct-layer-identity-test
build/direct-layer-identity-test: tests/core/DirectLayerIdentityTest.cpp $(wildcard src/core/*.h)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $<

.PHONY: test-amiga-trace-report
test: test-amiga-trace-report
test-amiga-trace-report:
	python3 -B tests/amiga-trace-report-test.py

-include $(DEPS)

TERRAIN_OWNERSHIP_TEST_SOURCES := tests/platform/amiga/AmigaTerrainOwnershipTest.cpp src/platform/amiga/AmigaGameplayGeneration.cpp src/platform/amiga/AmigaTerrain.cpp src/core/GameplayWorld.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
build/amiga-terrain-source-ownership-test: $(TERRAIN_OWNERSHIP_TEST_SOURCES) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_TERRAIN_SOURCE_RECORDS -o $@ $(TERRAIN_OWNERSHIP_TEST_SOURCES)
.PHONY: verify-terrain-ownership
build/amiga-terrain-ownership-test: $(TERRAIN_OWNERSHIP_TEST_SOURCES) $(HOST_CHECK_HEADERS)
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(TERRAIN_OWNERSHIP_TEST_SOURCES)
verify-terrain-ownership: build/amiga-terrain-ownership-test generate-room-pack generate-object-pack generate-builder-pattern-pack
	./build/amiga-terrain-ownership-test generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp generated/downwell-sprites.dwsc
test: verify-terrain-ownership
