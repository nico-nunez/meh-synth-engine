#include "synth/Engine.h"

#include <cstdio>

int main() {
  synth::EngineConfig config{};
  synth::Engine engine = synth::createEngine(config);

  printf("smoke_test: createEngine OK (sampleRate=%.0f, numFrames=%u)\n",
         engine.sampleRate,
         engine.numFrames);

  return 0;
}
