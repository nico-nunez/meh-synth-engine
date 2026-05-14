#pragma once

#include "synth/ModMatrix.h"
#include "synth/SignalChain.h"
#include "synth/WavetableOsc.h"
#include "synth/params/ParamDefs.h"
#include "synth/preset/Preset.h"

#include "dsp/fx/FXChain.h"

#include <cstdint>
#include <string>
#include <vector>

namespace synth {
struct Engine;
}

namespace synth::program {

namespace {
using dsp::fx::chain::FXProcessor;
using mod_matrix::ModRoute;
using signal_chain::SignalProcessor;
using wavetable::osc::FMRoute;
} // namespace

inline constexpr uint8_t SYNTH_PROGRAM_NUM_OSCS = preset::NUM_OSCS;

struct SynthProgram {
  float paramValues[param::PARAM_COUNT]{};

  FMRoute oscFmRoutes[SYNTH_PROGRAM_NUM_OSCS][SYNTH_PROGRAM_NUM_OSCS]{};
  uint8_t oscFmRouteCounts[SYNTH_PROGRAM_NUM_OSCS]{};

  ModRoute modRoutes[mod_matrix::MAX_MOD_ROUTES]{};
  uint8_t modRouteCount = 0;

  SignalProcessor signalChain[signal_chain::MAX_CHAIN_SLOTS]{};
  uint8_t signalChainLength = 0;

  FXProcessor fxChain[dsp::fx::chain::MAX_EFFECT_SLOTS]{};
  uint8_t fxChainLength = 0;
};

struct ProgramBuildResult {
  bool ok = true;
  std::vector<std::string> errors{};
  std::vector<std::string> warnings{};
};

void initSynthProgram(SynthProgram& program);

ProgramBuildResult compilePresetToProgram(const preset::Preset& preset, SynthProgram* out);

void applySynthProgram(const SynthProgram& program, Engine& engine);

} // namespace synth::program
