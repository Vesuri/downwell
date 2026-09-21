# Downwell host reference build. Shared logic is kept free of SDL dependencies;
# only src/platform/sdl and main.cpp include SDL headers.

CXX := clang++

SDL_CFLAGS  := $(shell pkg-config --cflags sdl2 SDL2_image)
SDL_LDFLAGS := $(shell pkg-config --libs sdl2 SDL2_image)

CPPFLAGS := -Isrc $(SDL_CFLAGS)
CXXFLAGS := -std=c++17 -g -O0 -Wall -Wextra -Wpedantic -MMD -MP
LDFLAGS  := $(SDL_LDFLAGS)

SOURCES := \
	src/core/Game.cpp \
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
STARTUP_TEST_SOURCES := tests/core/StartupRuntimeTest.cpp src/core/StartupRuntime.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp
REPLAY_TEST_TARGET := build/replay-test
REPLAY_TEST_SOURCES := tests/core/ReplayTest.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp src/core/RoomPack.cpp src/core/ObjectPack.cpp
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
DEFERRED_INSTANCE_TEST_TARGET := build/deferred-instance-queue-test
DEFERRED_INSTANCE_TEST_SOURCES := tests/core/DeferredInstanceQueueTest.cpp src/core/ObjectPack.cpp
TEST_TARGETS := $(STARTUP_TEST_TARGET) $(REPLAY_TEST_TARGET) $(ALARM_TEST_TARGET) $(INSTANCE_TEST_TARGET) $(EVENT_TEST_TARGET) $(ROOM_TEST_TARGET) $(ROOM_PACK_TEST_TARGET) $(OBJECT_PACK_TEST_TARGET) $(SPRITE_PACK_TEST_TARGET) $(FONT_PACK_TEST_TARGET) $(DEFERRED_INSTANCE_TEST_TARGET)

.PHONY: all run test extract verify-extraction generate-room-pack verify-room-pack generate-object-pack verify-object-pack generate-sprite-pack verify-sprite-pack generate-font-pack verify-font-pack verify-native-data amiga clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

run: generate-room-pack generate-object-pack generate-sprite-pack generate-font-pack $(TARGET)
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

$(DEFERRED_INSTANCE_TEST_TARGET): $(DEFERRED_INSTANCE_TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(DEFERRED_INSTANCE_TEST_SOURCES)

test: generate-font-pack $(TEST_TARGETS)
	./$(STARTUP_TEST_TARGET)
	./$(REPLAY_TEST_TARGET)
	./$(ALARM_TEST_TARGET)
	./$(INSTANCE_TEST_TARGET)
	./$(EVENT_TEST_TARGET)
	./$(ROOM_TEST_TARGET)
	./$(ROOM_PACK_TEST_TARGET)
	./$(OBJECT_PACK_TEST_TARGET)
	./$(SPRITE_PACK_TEST_TARGET)
	./$(FONT_PACK_TEST_TARGET) generated/downwell-fonts.dwfn
	./$(DEFERRED_INSTANCE_TEST_TARGET)

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

verify-native-data: generate-room-pack generate-object-pack $(STARTUP_TEST_TARGET)
	./$(STARTUP_TEST_TARGET) generated/downwell-rooms.dwrm generated/downwell-objects.dwob

amiga:
	@bash -c '. amiga/env.sh && $(MAKE) -C amiga'

clean:
	$(RM) -r build
	$(MAKE) -C amiga clean

-include $(DEPS)
