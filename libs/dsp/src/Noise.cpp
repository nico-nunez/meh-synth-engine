#include "dsp/Noise.h"

namespace dsp::noise {

float processPinkNoise(PinkNoiseState& state, float w) {
  state.b0 = 0.99886f * state.b0 + w * 0.0555179f;
  state.b1 = 0.99332f * state.b1 + w * 0.0750759f;
  state.b2 = 0.96900f * state.b2 + w * 0.1538520f;
  return (state.b0 + state.b1 + state.b2 + w * 0.5362f) * 0.11f;
}

} // namespace dsp::noise
