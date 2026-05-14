#include "synth/Engine.h"

#include <cstdio>

int main() {
  synth::Engine engine{};
  synth::EngineConfig config{};
  synth::initEngine(engine, config);

  printf("smoke_test: createEngine OK (sampleRate=%.0f, numFrames=%u)\n",
         engine.sampleRate,
         engine.numFrames);

  return 0;
}
