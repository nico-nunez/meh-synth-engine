# engine.mk — include this in downstream app Makefiles
# Usage: include engine/engine.mk
#
# After include, ENGINE_SOURCES and ENGINE_INCLUDES are available.

ENGINE_ROOT := $(dir $(lastword $(MAKEFILE_LIST)))

ENGINE_SOURCES = \
  $(shell find $(ENGINE_ROOT)src $(ENGINE_ROOT)libs/dsp/src $(ENGINE_ROOT)libs/json/src -name '*.cpp')

ENGINE_INCLUDES = \
  -I$(ENGINE_ROOT)src \
  -I$(ENGINE_ROOT)libs/dsp/include \
  -I$(ENGINE_ROOT)libs/json/include
