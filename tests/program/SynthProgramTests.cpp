#include "TestRunner.h"

#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/preset/Preset.h"
#include "synth/program/SynthProgram.h"

namespace {

bool hasModRoute(const synth::program::SynthProgram& program,
                 synth::mod_matrix::ModSrc src,
                 synth::mod_matrix::ModDest dest,
                 float amount) {
  for (uint8_t i = 0; i < program.modRouteCount; ++i) {
    const auto& route = program.modRoutes[i];
    if (route.src == src && route.dest == dest && route.amount == amount)
      return true;
  }
  return false;
}

} // namespace

static void test_compile_init_preset_covers_all_bulk_state() {
  TEST("compile_init_preset_covers_all_bulk_state");

  auto preset = synth::preset::createInitPreset();
  synth::program::SynthProgram program{};
  auto result = synth::program::compilePresetToProgram(preset, &program);

  CHECK("compile ok", result.ok);
  CHECK("no errors", result.errors.empty());
  CHECK("param copied",
        program.paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].defaultVal);
  CHECK("signal chain length", program.signalChainLength == 3);
  CHECK("fx chain length", program.fxChainLength == 5);
  CHECK("mod route count", program.modRouteCount == 0);
  CHECK("fm route count", program.oscFmRouteCounts[0] == 0);
}

static void test_compile_resolves_mod_route_strings() {
  TEST("compile_resolves_mod_route_strings");

  auto preset = synth::preset::createInitPreset();
  preset.modMatrixCount = 1;
  preset.modMatrix[0].source = "lfo1";
  preset.modMatrix[0].destination = "svf.cutoff";
  preset.modMatrix[0].amount = 0.5f;

  synth::program::SynthProgram program{};
  auto result = synth::program::compilePresetToProgram(preset, &program);

  CHECK("compile ok", result.ok);
  CHECK("one route", program.modRouteCount == 1);
  CHECK("route resolved",
        hasModRoute(program, synth::mod_matrix::LFO1, synth::mod_matrix::SVFCutoff, 0.5f));
}

static void test_compile_rejects_invalid_mod_route_strings() {
  TEST("compile_rejects_invalid_mod_route_strings");

  auto preset = synth::preset::createInitPreset();
  preset.modMatrixCount = 1;
  preset.modMatrix[0].source = "badSource";
  preset.modMatrix[0].destination = "svf.cutoff";
  preset.modMatrix[0].amount = 0.5f;

  synth::program::SynthProgram program{};
  auto result = synth::program::compilePresetToProgram(preset, &program);

  CHECK("compile failed", !result.ok);
  CHECK("has error", !result.errors.empty());
  CHECK("output cleared", program.modRouteCount == 0);
}

static void test_compile_clamps_param_values_before_apply() {
  TEST("compile_clamps_param_values_before_apply");

  auto preset = synth::preset::createInitPreset();
  preset.paramValues[synth::param::OSC1_MIX_LEVEL] = 999.0f;

  synth::program::SynthProgram program{};
  auto result = synth::program::compilePresetToProgram(preset, &program);

  CHECK("compile ok", result.ok);
  CHECK("has warning", !result.warnings.empty());
  CHECK("clamped value",
        program.paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].max);
}

static void test_apply_program_updates_engine_without_preset_strings() {
  TEST("apply_program_updates_engine_without_preset_strings");

  synth::Engine engine{};
  synth::EngineConfig config{};
  synth::initEngine(engine, config);

  auto preset = synth::preset::createInitPreset();
  preset.paramValues[synth::param::OSC1_MIX_LEVEL] = 0.25f;
  preset.modMatrixCount = 1;
  preset.modMatrix[0].source = "lfo1";
  preset.modMatrix[0].destination = "osc1.pitch";
  preset.modMatrix[0].amount = 3.0f;

  synth::program::SynthProgram program{};
  auto result = synth::program::compilePresetToProgram(preset, &program);
  CHECK("compile ok", result.ok);

  synth::program::applySynthProgram(program, engine);

  CHECK("param applied", engine.params[synth::param::OSC1_MIX_LEVEL] == 0.25f);
  CHECK("mod route applied", engine.voicePool.modMatrix.count == 1);
  CHECK("mod route source", engine.voicePool.modMatrix.routes[0].src == synth::mod_matrix::LFO1);
  CHECK("mod route dest",
        engine.voicePool.modMatrix.routes[0].dest == synth::mod_matrix::Osc1Pitch);
}

void runSynthProgramTests() {
  SUITE("SynthProgram");
  test_compile_init_preset_covers_all_bulk_state();
  test_compile_resolves_mod_route_strings();
  test_compile_rejects_invalid_mod_route_strings();
  test_compile_clamps_param_values_before_apply();
  test_apply_program_updates_engine_without_preset_strings();
}
