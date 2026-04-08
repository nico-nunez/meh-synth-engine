CXX       = clang++
CXXFLAGS  = -std=c++17 -Wall -Wextra -Werror -pedantic-errors -O0 -ggdb
TARGET    = libmeh-synth-engine.a

SOURCES = $(shell find src libs/dsp/src libs/json/src -name '*.cpp')
OBJECTS = $(patsubst %.cpp,build/%.o,$(SOURCES))

INCLUDES = -Isrc -Ilibs/dsp/include -Ilibs/json/include

$(TARGET): $(OBJECTS)
	ar rcs $@ $^

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(TARGET) build/

.PHONY: clean

