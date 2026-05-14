#include "PresetApply.h"

#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/SignalChain.h"
#include "synth/WavetableOsc.h"
#include "synth/params/ParamDefs.h"
#include "synth/params/ParamSync.h"
#include "synth/program/SynthProgram.h"

#include "dsp/fx/FXChain.h"

#include <cstdint>
#include <cstdio>
#include <utility>

namespace synth::preset {

namespace {
namespace osc = wavetable::osc;
namespace mm = mod_matrix;
namespace sc = signal_chain;
} // namespace

// ============================================================
// Apply Preset
// ============================================================
ApplyResult applyPreset(const Preset& preset, Engine& engine) {
  using namespace program;
  ApplyResult result{};

  SynthProgram compiled{};
  ProgramBuildResult build = compilePresetToProgram(preset, &compiled);

  result.ok = build.ok;
  result.errors = std::move(build.errors);
  result.warnings = std::move(build.warnings);

  if (!result.ok)
    return result;

  applySynthProgram(compiled, engine);
  return result;
}

// ============================================================
// capturePreset
// ============================================================

Preset capturePreset(const Engine& engine) {
  const auto& pool = engine.voicePool;

  Preset p;
  p.version = CURRENT_PRESET_VERSION;

  // ==== All bound params ====
  for (int i = 0; i < param::PARAM_COUNT; i++) {
    p.paramValues[i] = engine.params[i];
  }

  // ==== Enum fields — read directly from engine ====
  const osc::WavetableOsc* oscs[] = {&pool.osc1, &pool.osc2, &pool.osc3, &pool.osc4};
  for (int i = 0; i < NUM_OSCS; i++) {
    p.oscFmRouteCounts[i] = oscs[i]->fmRouteCount;
    for (uint8_t r = 0; r < oscs[i]->fmRouteCount; r++)
      p.oscFmRoutes[i][r] = oscs[i]->fmRoutes[r];
  }

  // ==== Mod matrix ====
  p.modMatrixCount = pool.modMatrix.count;
  for (uint8_t i = 0; i < pool.modMatrix.count; i++) {
    const auto& route = pool.modMatrix.routes[i];
    p.modMatrix[i].source = mm::modSrcToString(route.src);
    p.modMatrix[i].destination = mm::modDestToString(route.dest);
    p.modMatrix[i].amount = route.amount;
  }

  // ==== Signal chain ====
  for (uint8_t i = 0; i < pool.signalChain.length && i < sc::MAX_CHAIN_SLOTS; i++) {
    p.signalChain[i] =
        i < pool.signalChain.length ? pool.signalChain.slots[i] : sc::SignalProcessor::None;
  }

  // ==== FX chain ordering ====
  p.fxChainLength = engine.fxChain.length;
  for (uint8_t i = 0; i < dsp::fx::chain::MAX_EFFECT_SLOTS; i++)
    p.fxChain[i] = engine.fxChain.slots[i];

  return p;
}

void printPreset(const Preset& p) {
  // --- Metadata ---
  printf("[Preset: %s]\n", p.metadata.name.c_str());
  if (!p.metadata.category.empty())
    printf("  category:  %s\n", p.metadata.category.c_str());
  if (!p.metadata.author.empty())
    printf("  author:    %s\n", p.metadata.author.c_str());
  printf("  version:   %u\n", p.version);

  // --- Params ---
  printf("[Params]\n");
  for (int i = 0; i < param::PARAM_COUNT; i++) {
    const auto& def = param::PARAM_DEFS[i];
    float v = p.paramValues[i];
    switch (def.type) {

    case param::ParamType::Float:
      printf("  %-28s %.3f\n", def.name, v);
      break;

    case param::ParamType::Bool:
      printf("  %-28s %s\n", def.name, v >= 0.5f ? "true" : "false");
      break;

    // TODO: print enum strings
    case param::ParamType::Int8:
    case param::ParamType::OscBankID:
    case param::ParamType::PhaseMode:
    case param::ParamType::NoiseType:
    case param::ParamType::FilterMode:
    case param::ParamType::DistortionType:
    case param::ParamType::Subdivision:
      printf("  %-28s %d\n", def.name, static_cast<int>(v));
      break;
    }
  }

  // --- Enum Fields ---
  printf("[Enum Fields]\n");
  const char* oscKeys[] = {"osc1", "osc2", "osc3", "osc4"};
  for (int i = 0; i < NUM_OSCS; i++) {
    if (p.oscFmRouteCounts[i] == 0) {
      printf("  %s.fmRoutes  (none)\n", oscKeys[i]);
    } else {
      for (uint8_t r = 0; r < p.oscFmRouteCounts[i]; r++)
        printf("  %s.fmRoutes[%u]  source=%-6s depth=%.3f\n",
               oscKeys[i],
               r,
               osc::fmSourceToString(p.oscFmRoutes[i][r].source),
               p.oscFmRoutes[i][r].depth);
    }
  }

  // --- Mod Matrix ---
  printf("[Mod Matrix]  (%u routes)\n", p.modMatrixCount);
  for (uint8_t i = 0; i < p.modMatrixCount; i++) {
    const auto& r = p.modMatrix[i];
    printf("  %u: %-12s -> %-20s amount=%.3f\n",
           i,
           r.source.c_str(),
           r.destination.c_str(),
           r.amount);
  }

  // --- Signal Chain ---
  printf("[Signal Chain]\n");
  for (int i = 0; i < sc::MAX_CHAIN_SLOTS; i++) {
    if (p.signalChain[i] == sc::SignalProcessor::None)
      break;
    printf("  %d: %s\n", i, sc::signalProcessorToString(p.signalChain[i]));
  }
}
} // namespace synth::preset
