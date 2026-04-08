#pragma once

namespace dsp::noise {

struct PinkNoiseState {
  float b0 = 0.0f;
  float b1 = 0.0f;
  float b2 = 0.0f;
};

// Kellet 3-stage IIR pink noise filter.
// whiteNoise: pre-generated white noise sample in [-1, 1].
// Returns pink noise sample. Caller is responsible for supplying the white noise input —
// do not call randNoiseValue() internally (keeps the filter composable with any noise source).
float processPinkNoise(PinkNoiseState& state, float whiteNoise);

} // namespace dsp::noise
