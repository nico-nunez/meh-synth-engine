#include "synth/program/SynthProgram.h"

#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/SignalChain.h"
#include "synth/WavetableOsc.h"
#include "synth/params/ParamRanges.h"
#include "synth/params/ParamSync.h"

#include "dsp/fx/FXChain.h"

#include <cmath>
#include <cstdint>
#include <string>

namespace synth::program {

namespace {

namespace osc = wavetable::osc;
namespace mm = mod_matrix;
namespace fx = dsp::fx::chain;
namespace sc = signal_chain;

inline constexpr float kFMRouteDepthMax = 10.0f;

void addError(ProgramBuildResult& result, const std::string& message) {
  result.ok = false;
  result.errors.emplace_back(message);
}

void addWarning(ProgramBuildResult& result, const std::string& message) {
  result.warnings.emplace_back(message);
}

float clampWarn(float value, float min, float max, const char* name, ProgramBuildResult& result) {
  if (value < min) {
    addWarning(result,
               std::string(name) + ": clamped " + std::to_string(value) + " to min " +
                   std::to_string(min));
    return min;
  }
  if (value > max) {
    addWarning(result,
               std::string(name) + ": clamped " + std::to_string(value) + " to max " +
                   std::to_string(max));
    return max;
  }
  return value;
}

bool validFMSource(osc::FMSource src) {
  return static_cast<uint8_t>(src) < osc::FM_SOURCE_COUNT;
}

bool validSignalProcessor(sc::SignalProcessor proc) {
  switch (proc) {
  case sc::SignalProcessor::None:
  case sc::SignalProcessor::SVF:
  case sc::SignalProcessor::Ladder:
  case sc::SignalProcessor::Saturator:
    return true;
  }
  return false;
}

bool validFXProcessor(fx::FXProcessor proc) {
  switch (proc) {
  case fx::FXProcessor::None:
  case fx::FXProcessor::Distortion:
  case fx::FXProcessor::Chorus:
  case fx::FXProcessor::Phaser:
  case fx::FXProcessor::Delay:
  case fx::FXProcessor::ReverbPlate:
    return true;
  }
  return false;
}

float clampModAmount(mm::ModDest dest, float amount) {
  namespace prm = param::ranges::mod;

  switch (dest) {
  case mm::SVFCutoff:
  case mm::LadderCutoff:
    return prm::clampCutoffMod(amount);
  case mm::SVFResonance:
  case mm::LadderResonance:
    return prm::clampResonanceMod(amount);
  case mm::Osc1Pitch:
  case mm::Osc2Pitch:
  case mm::Osc3Pitch:
  case mm::Osc4Pitch:
    return prm::clampPitchMod(amount);
  case mm::Osc1Mix:
  case mm::Osc2Mix:
  case mm::Osc3Mix:
  case mm::Osc4Mix:
    return prm::clampMixLevelMod(amount);
  case mm::Osc1ScanPos:
  case mm::Osc2ScanPos:
  case mm::Osc3ScanPos:
  case mm::Osc4ScanPos:
    return prm::clampScanPosMod(amount);
  case mm::Osc1FMDepth:
  case mm::Osc2FMDepth:
  case mm::Osc3FMDepth:
  case mm::Osc4FMDepth:
    return prm::clampFMDepthMod(amount);
  case mm::LFO1Rate:
  case mm::LFO2Rate:
  case mm::LFO3Rate:
    return prm::clampLFORateMod(amount);
  case mm::LFO1Amplitude:
  case mm::LFO2Amplitude:
  case mm::LFO3Amplitude:
    return prm::clampLFOAmplitudeMod(amount);
  case mm::NoDest:
  case mm::DEST_COUNT:
    return 0.0f;
  }
  return 0.0f;
}

void clearSynthProgram(SynthProgram& program) {
  program = SynthProgram{};
}

} // namespace

void initSynthProgram(SynthProgram& program) {
  for (int i = 0; i < param::PARAM_COUNT; ++i)
    program.paramValues[i] = param::PARAM_DEFS[i].defaultVal;

  program.signalChain[0] = sc::SignalProcessor::SVF;
  program.signalChain[1] = sc::SignalProcessor::Ladder;
  program.signalChain[2] = sc::SignalProcessor::Saturator;
  program.signalChainLength = 3;

  program.fxChain[0] = fx::FXProcessor::Distortion;
  program.fxChain[1] = fx::FXProcessor::Chorus;
  program.fxChain[2] = fx::FXProcessor::Phaser;
  program.fxChain[3] = fx::FXProcessor::Delay;
  program.fxChain[4] = fx::FXProcessor::ReverbPlate;
  program.fxChainLength = 5;
}

ProgramBuildResult compilePresetToProgram(const preset::Preset& preset, SynthProgram* out) {
  ProgramBuildResult result{};
  if (!out) {
    addError(result, "compilePresetToProgram: null output");
    return result;
  }

  SynthProgram program{};

  // Scalar params: complete copy with range normalization.
  for (int i = 0; i < param::PARAM_COUNT; ++i) {
    const auto& def = param::PARAM_DEFS[i];
    const float value = preset.paramValues[i];
    if (!std::isfinite(value)) {
      addError(result, std::string(def.name) + ": non-finite value");
      program.paramValues[i] = def.defaultVal;
      continue;
    }
    program.paramValues[i] = clampWarn(value, def.min, def.max, def.name, result);
  }

  // FM routes: full per-oscillator route state.
  for (uint8_t oscIndex = 0; oscIndex < SYNTH_PROGRAM_NUM_OSCS; ++oscIndex) {
    const uint8_t count = preset.oscFmRouteCounts[oscIndex];
    if (count > SYNTH_PROGRAM_NUM_OSCS) {
      addError(result,
               "osc" + std::to_string(static_cast<int>(oscIndex) + 1) +
                   ".fmRoutes: count exceeds max " +
                   std::to_string(static_cast<int>(SYNTH_PROGRAM_NUM_OSCS)));
      continue;
    }

    program.oscFmRouteCounts[oscIndex] = count;
    for (uint8_t routeIndex = 0; routeIndex < count; ++routeIndex) {
      const auto& route = preset.oscFmRoutes[oscIndex][routeIndex];
      if (!validFMSource(route.source)) {
        addError(result,
                 "osc" + std::to_string(static_cast<int>(oscIndex) + 1) + ".fmRoutes[" +
                     std::to_string(static_cast<int>(routeIndex)) + "]: invalid source");
        continue;
      }

      if (!std::isfinite(route.depth)) {
        addError(result,
                 "osc" + std::to_string(static_cast<int>(oscIndex) + 1) + ".fmRoutes[" +
                     std::to_string(static_cast<int>(routeIndex)) + "]: non-finite depth");
        continue;
      }

      program.oscFmRoutes[oscIndex][routeIndex].source = route.source;
      program.oscFmRoutes[oscIndex][routeIndex].depth =
          clampWarn(route.depth, 0.0f, kFMRouteDepthMax, "fmRoute.depth", result);
    }
  }

  // Mod matrix: resolve string names once, before runtime publication.
  if (preset.modMatrixCount > mm::MAX_MOD_ROUTES) {
    addError(result, "modMatrix: count exceeds max");
  } else {
    program.modRouteCount = preset.modMatrixCount;
    for (uint8_t i = 0; i < preset.modMatrixCount; ++i) {
      const auto& route = preset.modMatrix[i];
      const auto src = mm::parseModSrc(route.source.c_str());
      const auto dest = mm::parseModDest(route.destination.c_str());

      if (src == mm::ModSrc::NoSrc) {
        addError(result,
                 "modMatrix[" + std::to_string(static_cast<int>(i)) + "]: invalid source '" +
                     route.source + "'");
        continue;
      }
      if (dest == mm::ModDest::NoDest) {
        addError(result,
                 "modMatrix[" + std::to_string(static_cast<int>(i)) + "]: invalid destination '" +
                     route.destination + "'");
        continue;
      }
      if (!std::isfinite(route.amount)) {
        addError(result,
                 "modMatrix[" + std::to_string(static_cast<int>(i)) + "]: non-finite amount");
        continue;
      }

      program.modRoutes[i] = {src, dest, clampModAmount(dest, route.amount)};
    }
  }

  // Signal chain: preserve the contiguous prefix. None terminates the chain.
  bool signalTerminated = false;
  for (uint8_t i = 0; i < sc::MAX_CHAIN_SLOTS; ++i) {
    const auto proc = preset.signalChain[i];
    if (!validSignalProcessor(proc)) {
      addError(result,
               "signalChain[" + std::to_string(static_cast<int>(i)) + "]: invalid processor");
      continue;
    }

    if (proc == sc::SignalProcessor::None) {
      signalTerminated = true;
      continue;
    }

    if (signalTerminated) {
      addWarning(result,
                 "signalChain[" + std::to_string(static_cast<int>(i)) +
                     "]: non-empty processor after None ignored");
      continue;
    }

    program.signalChain[program.signalChainLength++] = proc;
  }

  // FX chain: use explicit preset length, then validate each active slot.
  if (preset.fxChainLength > fx::MAX_EFFECT_SLOTS) {
    addError(result, "fxChainLength exceeds max");
  } else {
    program.fxChainLength = preset.fxChainLength;
    for (uint8_t i = 0; i < preset.fxChainLength; ++i) {
      const auto proc = preset.fxChain[i];
      if (!validFXProcessor(proc) || proc == fx::FXProcessor::None) {
        addError(result,
                 "fxChain[" + std::to_string(static_cast<int>(i)) +
                     "]: invalid active effect processor");
        continue;
      }
      program.fxChain[i] = proc;
    }
  }

  if (!result.ok) {
    clearSynthProgram(*out);
    return result;
  }

  *out = program;
  return result;
}

void applySynthProgram(const SynthProgram& program, Engine& engine) {
  auto& pool = engine.voicePool;

  for (int i = 0; i < param::PARAM_COUNT; ++i) {
    const auto id = static_cast<param::ParamID>(i);
    param::sync::setParamDeferred(engine, id, program.paramValues[i]);
  }

  osc::WavetableOsc* oscs[] = {&pool.osc1, &pool.osc2, &pool.osc3, &pool.osc4};
  for (uint8_t i = 0; i < SYNTH_PROGRAM_NUM_OSCS; ++i) {
    oscs[i]->fmRouteCount = program.oscFmRouteCounts[i];
    for (uint8_t r = 0; r < program.oscFmRouteCounts[i]; ++r)
      oscs[i]->fmRoutes[r] = program.oscFmRoutes[i][r];
  }

  mm::clearRoutes(pool.modMatrix);
  for (uint8_t i = 0; i < program.modRouteCount; ++i)
    mm::addRoute(pool.modMatrix, program.modRoutes[i]);
  mm::clearPrevModDests(pool.modMatrix);

  sc::clearSigChain(pool.signalChain);
  sc::setSigChain(pool.signalChain, program.signalChain, program.signalChainLength);

  fx::clearFXChain(engine.fxChain);
  fx::setFXChain(engine.fxChain, program.fxChain, program.fxChainLength);

  engine.dirtyFlags.markAll();
  param::sync::syncDirtyParams(engine);

  engine.fxChain.delay.state.currentDelaySamples = engine.fxChain.delay.targetDelaySamples;
}

} // namespace synth::program
