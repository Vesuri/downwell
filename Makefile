# Downwell host reference build. Shared logic is kept free of SDL dependencies;
# only src/platform/sdl and main.cpp include SDL headers.

CXX := clang++
SKIP_INTRO ?= 0
SKIP_TO_GAME ?= 0
export SKIP_INTRO SKIP_TO_GAME

SDL_CFLAGS  := $(shell pkg-config --cflags sdl2 SDL2_image)
SDL_LDFLAGS := $(shell pkg-config --libs sdl2 SDL2_image)

CPPFLAGS := -Isrc $(SDL_CFLAGS)
ifeq ($(SKIP_INTRO),1)
CPPFLAGS += -DDOWNWELL_SKIP_INTRO
endif
ifeq ($(SKIP_TO_GAME),1)
CPPFLAGS += -DDOWNWELL_SKIP_TO_GAME
endif
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
	src/core/PlayerMotion.cpp \
	src/platform/sdl/PlatformSDL.cpp \
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
ROOM_PACK_TEST_SOURCES := tests/core/RoomPackTest.cpp src/core/RoomPack.cpp
OBJECT_PACK_TEST_TARGET := build/object-pack-test
OBJECT_PACK_TEST_SOURCES := tests/core/ObjectPackTest.cpp src/core/ObjectPack.cpp
SPRITE_PACK_TEST_TARGET := build/sprite-pack-test
SPRITE_PACK_TEST_SOURCES := tests/core/SpritePackTest.cpp src/core/SpritePack.cpp
FONT_PACK_TEST_TARGET := build/font-pack-test
FONT_PACK_TEST_SOURCES := tests/core/FontPackTest.cpp src/core/FontPack.cpp
BUILDER_PATTERN_PACK_TEST_TARGET := build/builder-pattern-pack-test
BUILDER_PATTERN_PACK_TEST_SOURCES := tests/core/BuilderPatternPackTest.cpp src/core/BuilderPatternPack.cpp
AREA_ONE_BUILDER_TEST_TARGET := build/area-one-builder-test
AREA_ONE_BUILDER_TEST_SOURCES := tests/core/AreaOneBuilderTest.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp src/core/GameMakerRandom.cpp
AMIGA_LIVE_TERRAIN_TEST_TARGET := build/amiga-live-terrain-test
AMIGA_LIVE_TERRAIN_TEST_SOURCES := tests/platform/amiga/AmigaLiveTerrainTest.cpp src/platform/amiga/AmigaTerrain.cpp src/platform/amiga/AmigaPortRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/BuilderPatternPack.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/Replay.cpp
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
AMIGA_PORT_TEST_SOURCES := tests/platform/amiga/AmigaPortRuntimeTest.cpp src/platform/amiga/AmigaPortRuntime.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp
AMIGA_SCENE_TEST_TARGET := build/amiga-scene-runtime-test
AMIGA_SCENE_TEST_SOURCES := tests/platform/amiga/AmigaSceneRuntimeTest.cpp src/platform/amiga/AmigaSceneRuntime.cpp src/platform/amiga/AmigaTerrain.cpp src/platform/amiga/AmigaPortRuntime.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
AMIGA_MENU_TEST_TARGET := build/amiga-menu-test
AMIGA_MENU_TEST_SOURCES := tests/platform/amiga/AmigaMenuTest.cpp src/platform/amiga/AmigaMenu.cpp src/core/StartupRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
AMIGA_MENU_SCROLL_TEST_TARGET := build/amiga-menu-scroll-test
AMIGA_MENU_SCROLL_TEST_SOURCES := tests/platform/amiga/AmigaMenuScrollTest.cpp src/platform/amiga/AmigaMenuScroll.cpp
AMIGA_RENDERER_TEST_TARGET := build/amiga-renderer-test
AMIGA_RENDERER_TEST_SOURCES := tests/platform/amiga/AmigaRendererTest.cpp src/platform/amiga/AmigaRenderer.c
PORT_REPLAY_REFERENCE_TARGET := build/port-replay-reference
PORT_REPLAY_REFERENCE_SOURCES := tools/port-replay-reference.cpp src/platform/amiga/AmigaPortRuntime.cpp src/core/PlayerMotion.cpp src/core/Replay.cpp src/core/GameMakerRandom.cpp
AMIGA_STYLE_FRAME_SHA256 := b23334336ced93fbc77435a47aaec7586972bd384902e3c850988c11c25d4c8f
AMIGA_LOCKED_STYLE_FRAME_SHA256 := 0a767cebfc24ed23cb936f3ebdc6dd8359a6784a2ba475be7f10a0cb0adaf524
AMIGA_SELECTED_STYLE_FRAME_SHA256 := 2bae3a77e4f22ba81e42ba3c648c917a4227b427511d458b7345b2c564363fb9
AMIGA_SPLASH_FRAME_SHA256 := b82d3f72e70a9788a16bdeb5f681fc675d831562d407bfeac317f4508d67d9f2
AMIGA_CONTROLS_FRAME_SHA256 := ee3f455e6c9192d08f64ea26cf6b56338badf0dadeee56acf85ae2cd2838560a
AMIGA_CREDITS_FRAME_SHA256 := 9e7137cae00e1925f0a61c045f23ed1571358e9e02451b2acf9db036453d3af1
AMIGA_MENU_WAKE_FRAME_SHA256 := 5f84cd99630daab845d51ecda5709970caa5bb265364bee92b0be7cd229c45fa
AMIGA_MENU_TITLE_FRAME_SHA256 := d9fcc0c40f37e61e239593aa8520a47415c054439283d7992264c40b63b39f2a
AMIGA_MENU_ENTRY_FRAME_SHA256 := f734aa6e16f4014ddbba20d84a4ba871aa8eaa339f4bdf383bb0a25f33e20d2b
AMIGA_GAMEPLAY_FRAME_SHA256 := 8a94f47718ac0e5685d37da5c9413921a3d1f7e708f1c7ec506e36ee60a9d78b
AMIGA_GAMEPLAY_ACTIVE_FRAME_SHA256 := aa029994a648d40bb5283e90bcd2d98011e8df0c678727fce8c1dc4422f56c38
AREA_ONE_TRACE_TARGET := build/trace-area-one
AREA_ONE_TRACE_SOURCES := tools/trace-area-one.cpp src/core/StartupRuntime.cpp src/core/PlayerMotion.cpp src/core/AreaOneBuilder.cpp src/core/GameMakerRandom.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp src/core/BuilderPatternPack.cpp
TEST_TARGETS := $(STARTUP_TEST_TARGET) $(REPLAY_TEST_TARGET) $(ALARM_TEST_TARGET) $(INSTANCE_TEST_TARGET) $(EVENT_TEST_TARGET) $(ROOM_TEST_TARGET) $(ROOM_PACK_TEST_TARGET) $(OBJECT_PACK_TEST_TARGET) $(SPRITE_PACK_TEST_TARGET) $(FONT_PACK_TEST_TARGET) $(BUILDER_PATTERN_PACK_TEST_TARGET) $(DEFERRED_INSTANCE_TEST_TARGET) $(RANDOM_TEST_TARGET) $(PLAYER_COLLISION_TEST_TARGET) $(GAME_RENDER_TEST_TARGET) $(HUD_NUMBER_TEST_TARGET) $(AMIGA_PLAYER_TEST_TARGET) $(AMIGA_PORT_TEST_TARGET) $(AMIGA_SCENE_TEST_TARGET) $(AMIGA_MENU_TEST_TARGET) $(AMIGA_MENU_SCROLL_TEST_TARGET) $(AMIGA_RENDERER_TEST_TARGET)

.PHONY: all run test trace-area-one extract verify-extraction generate-room-pack verify-room-pack generate-object-pack verify-object-pack generate-sprite-pack verify-sprite-pack generate-font-pack verify-font-pack generate-builder-pattern-pack verify-builder-pattern-pack generate-amiga-hud verify-amiga-hud generate-amiga-game verify-amiga-game verify-native-data verify-amiga-startup verify-amiga-menu-title verify-amiga-menu-entry verify-amiga-menu-wake-frame verify-amiga-menu-title-frame verify-amiga-menu-entry-frame verify-amiga-gameplay-frame verify-amiga-gameplay-active-frame verify-amiga-splash-frame verify-amiga-controls-frame verify-amiga-credits-frame verify-amiga-style-frame verify-amiga-locked-style-frame verify-amiga-selected-style-frame verify-amiga-scenes verify-cross-replay amiga clean

all: $(TARGET)

.PHONY: verify-gameplay-death-state
verify-gameplay-death-state: generate-room-pack generate-object-pack generate-builder-pattern-pack $(AMIGA_SCENE_TEST_TARGET)
	DOWNWELL_COMBAT_TICKS=6000 DOWNWELL_VERIFY_COMBAT=1 DOWNWELL_VERIFY_DEATH=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

# These executables compile multiple translation units in one command. The
# compiler's single .d file only describes the final input, so explicitly
# invalidate them on shared header changes rather than trusting stale tools.
HOST_CHECK_HEADERS := $(wildcard src/core/*.h src/platform/amiga/*.h)
$(TEST_TARGETS) $(PORT_REPLAY_REFERENCE_TARGET) $(AREA_ONE_BUILDER_TEST_TARGET) $(AMIGA_LIVE_TERRAIN_TEST_TARGET) build/gem-homing-test build/frog-behavior-test build/bat-motion-test build/enemy-contact-test: $(HOST_CHECK_HEADERS)

test: build/gem-homing-test
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

build/standard-bullet-test: tests/core/StandardBulletStepTest.cpp src/core/StandardBulletStep.h src/core/EnemyContact.h src/core/GameMakerRounding.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/StandardBulletStepTest.cpp

build/main-camera-test: tests/core/MainCameraMotionTest.cpp src/core/MainCameraMotion.h
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

build/sdl-screenshot-test: tests/platform/SDLScreenshotTest.cpp src/platform/sdl/PlatformSDL.cpp src/core/SpritePack.cpp src/core/FontPack.cpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/skip-intro-test: tests/core/SkipIntroTest.cpp $(filter-out tests/core/GameRenderTest.cpp,$(GAME_RENDER_TEST_SOURCES)) src/platform/amiga/AmigaSceneRuntime.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_SKIP_INTRO -o $@ $^

build/skip-to-game-test: tests/core/SkipToGameTest.cpp $(filter-out tests/core/GameRenderTest.cpp,$(GAME_RENDER_TEST_SOURCES)) src/platform/amiga/AmigaSceneRuntime.cpp
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -DDOWNWELL_SKIP_INTRO -DDOWNWELL_SKIP_TO_GAME -o $@ $^

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
build/shop-inventory-test: tests/core/ShopInventoryTest.cpp src/core/ShopInventory.h src/core/CurrencyAccount.h src/core/GameMakerRounding.h
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
	@printf '%s\n' '$(SKIP_INTRO)' '$(SKIP_TO_GAME)' > $@.tmp
	@cmp -s $@.tmp $@ || cp $@.tmp $@

$(OBJECTS): build/startup-options

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

generated/amiga-shop.text.h: tools/generate-amiga-hud.py data/language.ini
	tools/generate-amiga-hud.py --profile shop

build/src/core/Game.o $(GAME_RENDER_TEST_TARGET): generated/amiga-shop.text.h

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

$(AREA_ONE_BUILDER_TEST_TARGET): $(AREA_ONE_BUILDER_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AREA_ONE_BUILDER_TEST_SOURCES)

$(AMIGA_LIVE_TERRAIN_TEST_TARGET): $(AMIGA_LIVE_TERRAIN_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(AMIGA_LIVE_TERRAIN_TEST_SOURCES)

$(DEFERRED_INSTANCE_TEST_TARGET): $(DEFERRED_INSTANCE_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(DEFERRED_INSTANCE_TEST_SOURCES)

$(RANDOM_TEST_TARGET): $(RANDOM_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(RANDOM_TEST_SOURCES)

$(PLAYER_CONTROLS_TEST_TARGET): tests/core/PlayerControlsTest.cpp src/core/PlayerControls.h
	@mkdir -p build
	$(CXX) -Isrc $(CXXFLAGS) -o $@ tests/core/PlayerControlsTest.cpp

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

test: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack generate-builder-pattern-pack generate-amiga-game generate-amiga-hud $(TEST_TARGETS) $(PLAYER_CONTROLS_TEST_TARGET) $(AREA_ONE_BUILDER_TEST_TARGET) $(AMIGA_LIVE_TERRAIN_TEST_TARGET) build/bat-motion-test build/enemy-contact-test build/frog-behavior-test
	./build/frog-behavior-test
	python3 tools/verify-enemy-contact.py
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
	python3 tools/verify-amiga-style-mask.py
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
	./$(AREA_ONE_BUILDER_TEST_TARGET) generated/downwell-builder-patterns.dwbp
	./$(AMIGA_LIVE_TERRAIN_TEST_TARGET) generated/downwell-builder-patterns.dwbp
	./$(DEFERRED_INSTANCE_TEST_TARGET)
	./$(RANDOM_TEST_TARGET)
	./$(PLAYER_COLLISION_TEST_TARGET)
	./$(PLAYER_CONTROLS_TEST_TARGET)
	./$(GAME_RENDER_TEST_TARGET)
	./$(HUD_NUMBER_TEST_TARGET)
	./$(AMIGA_PLAYER_TEST_TARGET)
	./$(AMIGA_PORT_TEST_TARGET)
	DOWNWELL_VERIFY_COMBAT=1 DOWNWELL_VERIFY_FROG_MOTION=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	DOWNWELL_COMBAT_FIRE_PHASE=5 DOWNWELL_VERIFY_COMBAT=1 DOWNWELL_VERIFY_FROG_MOTION=1 ./$(AMIGA_SCENE_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp
	./$(AMIGA_MENU_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob
	./$(AMIGA_MENU_SCROLL_TEST_TARGET)
	./$(AMIGA_RENDERER_TEST_TARGET) generated/amiga-game.dwap generated/amiga-hud.dwap

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

generate-amiga-game:
	tools/generate-amiga-hud.py --profile game

verify-amiga-game: generate-amiga-game
	python3 tools/verify-amiga-menu-sprites.py
	tools/generate-amiga-hud.py --profile game --check
	python3 tests/exit-mask-test.py

verify-native-data: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack generate-builder-pattern-pack $(STARTUP_TEST_TARGET)
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp

verify-cross-replay: $(PORT_REPLAY_REFERENCE_TARGET)
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga REPLAY_VERIFY=1'
	bash -c '. amiga/env.sh && amiga/verify_replay.sh ../$(PORT_REPLAY_REFERENCE_TARGET)'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

.PHONY: verify-amiga-gameplay-cache
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

.PHONY: verify-amiga-exit-wipe
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

verify-amiga-menu-wake-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_menu_wake_frame.gdb READY_MARKER=DOWNWELL_MENU_WAKE_FRAME_READY READY_DESCRIPTION="hardware-scrolled waking garden frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_MENU_WAKE_FRAME_SHA256)' 'amiga/.run/menu-wake-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-menu-frame.py amiga/.run/menu-wake-frame.bin build/amiga-menu-wake-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-menu-title-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_menu_title_frame.gdb READY_MARKER=DOWNWELL_MENU_TITLE_FRAME_READY READY_DESCRIPTION="fully revealed Downwell title frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_MENU_TITLE_FRAME_SHA256)' 'amiga/.run/menu-title-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-menu-frame.py amiga/.run/menu-title-frame.bin build/amiga-menu-title-frame.png
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

verify-amiga-menu-entry-frame:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_menu_entry_frame.gdb READY_MARKER=DOWNWELL_MENU_ENTRY_FRAME_READY READY_DESCRIPTION="first hardware-scrolled well-entry frame" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_MENU_ENTRY_FRAME_SHA256)' 'amiga/.run/menu-entry-frame.bin' | shasum -a 256 -c -
	python3 tools/verify-amiga-entry-continuity.py
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

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
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_gameplay_active_frame.gdb READY_MARKER=DOWNWELL_GAMEPLAY_ACTIVE_FRAME_READY READY_DESCRIPTION="scrolled gameplay raster with an awake enemy" amiga/verify_startup.sh'
	printf '%s  %s\n' '$(AMIGA_GAMEPLAY_ACTIVE_FRAME_SHA256)' 'amiga/.run/gameplay-active-frame.bin' | shasum -a 256 -c -
	python3 tools/decode-amiga-gameplay-frame.py amiga/.run/gameplay-active-frame.bin build/amiga-gameplay-active-frame.png
	@capture_tick=$$(sed -n 's/^CAPTURE_STATE tick=\([0-9]*\).*/\1/p' amiga/.run/gdb-startup.log); \
	capture_dir=$$(mktemp -d build/active-XXXXXX); \
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software ./build/capture-sdl-game "$$capture_dir" "$$capture_tick" && \
	python3 tools/compare-gameplay-captures.py "$$capture_dir" build/amiga-gameplay-active-frame.png
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

verify-amiga-scenes:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_scenes.gdb READY_MARKER=DOWNWELL_GAMEPLAY_ACTIVE READY_DESCRIPTION="complete native scene sequence through scrolling gameplay and enemy activation" amiga/verify_startup.sh'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

.PHONY: verify-amiga-results-restart
verify-amiga-results-restart:
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga SCENE_VERIFY=1 RESULTS_RESTART_VERIFY=1 SKIP_TO_GAME=1'
	bash -c '. amiga/env.sh && GDB_SOURCE=verify_results_restart.gdb READY_MARKER=DOWNWELL_RESULTS_RESTART_READY READY_DESCRIPTION="all results destinations and gameplay reentry" amiga/verify_startup.sh'
	bash -c '. amiga/env.sh && $(MAKE) -C amiga clean && $(MAKE) -C amiga'

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

-include $(DEPS)
