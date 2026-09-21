# Downwell host reference build. Shared logic is kept free of SDL dependencies;
# only src/platform/sdl and main.cpp include SDL headers.

CXX := clang++

SDL_CFLAGS  := $(shell pkg-config --cflags sdl2)
SDL_LDFLAGS := $(shell pkg-config --libs sdl2)

CPPFLAGS := -Isrc $(SDL_CFLAGS)
CXXFLAGS := -std=c++17 -g -O0 -Wall -Wextra -Wpedantic -MMD -MP
LDFLAGS  := $(SDL_LDFLAGS)

SOURCES := \
	src/core/Game.cpp \
	src/core/Replay.cpp \
	src/core/StartupRuntime.cpp \
	src/platform/sdl/PlatformSDL.cpp \
	src/main.cpp

OBJECTS := $(patsubst %.cpp,build/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)
TARGET  := build/downwell-sdl
STARTUP_TEST_TARGET := build/startup-runtime-test
STARTUP_TEST_SOURCES := tests/core/StartupRuntimeTest.cpp src/core/StartupRuntime.cpp
REPLAY_TEST_TARGET := build/replay-test
REPLAY_TEST_SOURCES := tests/core/ReplayTest.cpp src/core/Replay.cpp src/core/StartupRuntime.cpp
ALARM_TEST_TARGET := build/alarm-bank-test
ALARM_TEST_SOURCES := tests/core/AlarmBankTest.cpp
INSTANCE_TEST_TARGET := build/instance-store-test
INSTANCE_TEST_SOURCES := tests/core/InstanceStoreTest.cpp
EVENT_TEST_TARGET := build/event-scheduler-test
EVENT_TEST_SOURCES := tests/core/EventSchedulerTest.cpp
ROOM_TEST_TARGET := build/room-loader-test
ROOM_TEST_SOURCES := tests/core/RoomLoaderTest.cpp
TEST_TARGETS := $(STARTUP_TEST_TARGET) $(REPLAY_TEST_TARGET) $(ALARM_TEST_TARGET) $(INSTANCE_TEST_TARGET) $(EVENT_TEST_TARGET) $(ROOM_TEST_TARGET)

.PHONY: all run test extract verify-extraction generate-room-pack verify-room-pack amiga clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
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

test: $(TEST_TARGETS)
	./$(STARTUP_TEST_TARGET)
	./$(REPLAY_TEST_TARGET)
	./$(ALARM_TEST_TARGET)
	./$(INSTANCE_TEST_TARGET)
	./$(EVENT_TEST_TARGET)
	./$(ROOM_TEST_TARGET)

extract:
	tools/extract.sh
	tools/verify-extraction.sh

verify-extraction:
	tools/verify-extraction.sh

generate-room-pack:
	tools/generate-room-pack.py

verify-room-pack:
	tools/generate-room-pack.py --check

amiga:
	@bash -c '. amiga/env.sh && $(MAKE) -C amiga'

clean:
	$(RM) -r build
	$(MAKE) -C amiga clean

-include $(DEPS)
