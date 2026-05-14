#pragma once

#include "synth/ModMatrix.h"
#include "synth/SignalChain.h"
#include "synth/WavetableOsc.h"
#include "synth/params/ParamDefs.h"
#include "synth/preset/Preset.h"

#include "dsp/fx/FXChain.h"

#include <atomic>
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

struct ProgramSwapState {
  SynthProgram buffers[2]{};
  std::atomic<bool> pendingReady{false};
  std::atomic<bool> writeInFlight{false};
};

struct ProgramSwapResult {
  bool ok = true;
  const char* err = nullptr;
};

struct ProgramBuildResult {
  bool ok = true;
  std::vector<std::string> errors{};
  std::vector<std::string> warnings{};
};

SynthProgram* getWriteSwapBuffer(ProgramSwapState& swap);
SynthProgram* getReadSwapBuffer(ProgramSwapState& swap);

ProgramSwapResult prepareProgramSwap(Engine& engine, const SynthProgram& program);
ProgramSwapResult commitProgramSwap(Engine& engine);
void abortProgramSwap(Engine& engine);
void publishPendingProgramIfReady(Engine& engine);

void initProgramSwap(Engine& engine);
void initSynthProgram(SynthProgram& program);

ProgramBuildResult compilePresetToProgram(const preset::Preset& preset, SynthProgram* out);

void applySynthProgram(const SynthProgram& program, Engine& engine);

} // namespace synth::program
