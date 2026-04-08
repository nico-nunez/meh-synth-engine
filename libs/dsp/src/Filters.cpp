#include "dsp/Filters.h"
#include "dsp/Math.h"

#include <cassert>

namespace dsp::filters {

// ==== Cytomic/TPT Form (fixes issue with Chamberlin) ====
//
float modulateCutoff(float baseCutoff, float modulationOctaves) {
  return baseCutoff * dsp::math::fastExp2(modulationOctaves);
}

// Call when cutoff or resonance changes — NOT per sample
SVFCoeffs computeSVFCoeffs(float normalizedFreq, float Q) {
  assert(normalizedFreq < 0.5f);

  float g = math::fastTan(math::PI_F * normalizedFreq);
  float k = 1.0f / Q;
  float a1 = 1.0f / (1.0f + g * (g + k));
  float a2 = g * a1;
  float a3 = g * a2;
  return {a1, a2, a3, k};
}

SVFOutputs processSVF(float input, const SVFCoeffs& c, SVFState& s) {
  float v3 = input - s.ic2;
  float v1 = c.a1 * s.ic1 + c.a2 * v3;
  float v2 = s.ic2 + c.a2 * s.ic1 + c.a3 * v3;

  s.ic1 = 2.0f * v1 - s.ic1;
  s.ic2 = 2.0f * v2 - s.ic2;

  return {v2, v1, input - c.k * v1 - v2};
}

// ==== Ladder Filter (Moog Style) ====
float processLadder(float input, float freqCoeff, float resonance, LadderState& state) {
  float G = freqCoeff / (1.0f + freqCoeff);
  float G2 = G * G;
  float G3 = G2 * G;
  float G4 = G3 * G;

  // State contribution (what the filter would output with zero input)
  float S = (1.0f - G) * (G3 * state.s[0] + G2 * state.s[1] + G * state.s[2] + state.s[3]);

  // Solve implicit feedback
  float u = (input - resonance * S) / (1.0f + resonance * G4);

  // Process four cascaded one-pole TPT integrators
  float x = u;
  for (int i = 0; i < 4; i++) {
    float v = G * (x - state.s[i]);
    float y = v + state.s[i];
    state.s[i] = y + v;
    x = y;
  }

  return x; // 4-pole LP output
}

float processLadderNonlinear(float input,
                             float freqCoeff,
                             float resonance,
                             float drive,
                             float& y4Estimate,
                             LadderState& state) {
  float G = freqCoeff / (1.0f + freqCoeff);
  float G2 = G * G;
  float G3 = G2 * G;
  float G4 = G3 * G;

  // State contribution from previous sample
  float S = (1.0f - G) * (G3 * state.s[0] + G2 * state.s[1] + G * state.s[2] + state.s[3]);

  float a = drive * input;

  // Newton-Raphson: solve for y4
  // Initial guess: previous sample's y4 (excellent predictor at audio rate)
  float y4 = y4Estimate;

  for (int iter = 0; iter < 2; iter++) {
    float t4 = math::fastTanh(y4);
    float uv = a - resonance * t4;
    float tu = math::fastTanh(uv);

    float f = G4 * tu + S - y4;
    float fp = -G4 * resonance * (1.0f - tu * tu) * (1.0f - t4 * t4) - 1.0f;

    y4 -= f / fp;
  }

  y4Estimate = y4; // store for next sample's initial guess

  // Compute u from solved y4
  float u = math::fastTanh(a - resonance * math::fastTanh(y4));

  // Process four TPT one-pole stages (linear internals)
  float x = u;
  for (int i = 0; i < 4; i++) {
    float v = G * (x - state.s[i]);
    float y = v + state.s[i];
    state.s[i] = y + v;
    x = y;
  }

  return x;
}

float dcBlock(float sample, float& state, float coefficient) {
  float output = sample - state;
  state = sample * (1.0f - coefficient) + state * coefficient;
  return output;
}

} // namespace dsp::filters
