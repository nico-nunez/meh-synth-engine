CXX      = clang++
WARNS    = -Wall -Wextra -Werror -pedantic-errors
INCLUDES = -Isrc -Ilibs/dsp/include -Ilibs/json/include

# Build type: make BUILD=release
BUILD ?= debug
ifeq ($(BUILD),debug)
  OPTFLAGS = -O0 -ggdb
else ifeq ($(BUILD),release)
  OPTFLAGS = -O2 -DNDEBUG
else
  $(error Unknown BUILD type '$(BUILD)'. Use debug or release)
endif

CXXFLAGS = -std=c++17 $(WARNS) $(OPTFLAGS)

TARGET  = libmeh-synth-engine.a
SOURCES = $(shell find src libs/dsp/src libs/json/src -name '*.cpp')
OBJECTS = $(patsubst %.cpp,build/$(BUILD)/%.o,$(SOURCES))

$(TARGET): $(OBJECTS)
	ar rcs $@ $^

build/$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Smoke test — links the library, catches undefined references at link time
SMOKE_BIN = build/$(BUILD)/smoke_test
SMOKE_SRC = tools/smoke_test.cpp

smoke: $(TARGET) $(SMOKE_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SMOKE_SRC) $(TARGET) -o $(SMOKE_BIN)
	$(SMOKE_BIN)

clean:
	rm -rf $(TARGET) build/

.PHONY: clean smoke
