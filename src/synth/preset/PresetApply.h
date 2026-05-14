#pragma once

#include "Preset.h"

#include <string>
#include <vector>

namespace synth {
struct Engine;
}

namespace synth::preset {

struct ApplyResult {
  bool ok = true;
  std::vector<std::string> errors{};
  std::vector<std::string> warnings{};
};

ApplyResult applyPreset(const Preset& preset, Engine& engine);

Preset capturePreset(const Engine& engine);

void printPreset(const Preset& p);
} // namespace synth::preset
