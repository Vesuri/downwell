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
	src/core/StartupRuntime.cpp \
	src/platform/sdl/PlatformSDL.cpp \
	src/main.cpp

OBJECTS := $(patsubst %.cpp,build/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)
TARGET  := build/downwell-sdl
TEST_TARGET := build/startup-runtime-test
TEST_SOURCES := tests/core/StartupRuntimeTest.cpp src/core/StartupRuntime.cpp

.PHONY: all run test extract verify-extraction amiga clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	@mkdir -p $(@D)
	$(CXX) -Isrc $(CXXFLAGS) -o $@ $(TEST_SOURCES)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

extract:
	tools/extract.sh
	tools/verify-extraction.sh

verify-extraction:
	tools/verify-extraction.sh

amiga:
	@bash -c '. amiga/env.sh && $(MAKE) -C amiga'

clean:
	$(RM) -r build
	$(MAKE) -C amiga clean

-include $(DEPS)
