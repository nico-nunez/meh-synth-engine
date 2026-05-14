#include "TestRunner.h"

#include "synth/Engine.h"
#include "synth/program/SynthProgram.h"

static void test_prepare_commit_publish_program_swap() {
  TEST("prepare_commit_publish_program_swap");

  synth::Engine engine{};
  synth::initEngine(engine, {});

  synth::program::SynthProgram program{};
  synth::program::initSynthProgram(program);

  program.paramValues[synth::param::OSC1_MIX_LEVEL] = 0.25f;

  auto prepare = synth::program::prepareProgramSwap(engine, program);
  CHECK("prepare ok", prepare.ok);
  CHECK("write in flight", engine.programSwap.writeInFlight.load());
  CHECK("pending not ready", !engine.programSwap.pendingReady.load());

  auto commit = synth::program::commitProgramSwap(engine);
  CHECK("commit ok", commit.ok);
  CHECK("write released", !engine.programSwap.writeInFlight.load());
  CHECK("pending ready", engine.programSwap.pendingReady.load());

  synth::program::publishPendingProgramIfReady(engine);
  CHECK("pending cleared", !engine.programSwap.pendingReady.load());
  CHECK("param applied", engine.params[synth::param::OSC1_MIX_LEVEL] == 0.25f);
  CHECK("current buffer updated",
        engine.programSwap.buffers[0].paramValues[synth::param::OSC1_MIX_LEVEL] == 0.25f);
}

static void test_prepare_rejects_second_writer() {
  TEST("prepare_rejects_second_writer");

  synth::Engine engine{};
  synth::initEngine(engine, {});

  synth::program::SynthProgram program{};
  synth::program::initSynthProgram(program);

  auto first = synth::program::prepareProgramSwap(engine, program);
  auto second = synth::program::prepareProgramSwap(engine, program);

  CHECK("first ok", first.ok);
  CHECK("second rejected", !second.ok);

  synth::program::abortProgramSwap(engine);
  CHECK("write released", !engine.programSwap.writeInFlight.load());
}

static void test_prepare_rejects_pending_ready() {
  TEST("prepare_rejects_pending_ready");

  synth::Engine engine{};
  synth::initEngine(engine, {});

  synth::program::SynthProgram program{};
  synth::program::initSynthProgram(program);

  auto prepare = synth::program::prepareProgramSwap(engine, program);
  auto commit = synth::program::commitProgramSwap(engine);
  auto second = synth::program::prepareProgramSwap(engine, program);

  CHECK("prepare ok", prepare.ok);
  CHECK("commit ok", commit.ok);
  CHECK("second rejected", !second.ok);
}

static void test_process_audio_block_publishes_pending_program() {
  TEST("process_audio_block_publishes_pending_program");

  synth::Engine engine{};
  synth::initEngine(engine, {});

  synth::program::SynthProgram program{};
  synth::program::initSynthProgram(program);

  program.paramValues[synth::param::OSC1_MIX_LEVEL] = 0.5f;

  auto prepare = synth::program::prepareProgramSwap(engine, program);
  auto commit = synth::program::commitProgramSwap(engine);
  CHECK("prepare ok", prepare.ok);
  CHECK("commit ok", commit.ok);

  float left[16]{};
  float right[16]{};
  float* channels[2] = {left, right};
  synth::RenderContext ctx{};
  ctx.bpm = engine.bpm;

  engine.processAudioBlock(channels, 2, 16, ctx);

  CHECK("pending cleared", !engine.programSwap.pendingReady.load());
  CHECK("param applied", engine.params[synth::param::OSC1_MIX_LEVEL] == 0.5f);
}

void runSynthProgramPublicationTests() {
  SUITE("SynthProgramPublication");
  test_prepare_commit_publish_program_swap();
  test_prepare_rejects_second_writer();
  test_prepare_rejects_pending_ready();
  test_process_audio_block_publishes_pending_program();
}
