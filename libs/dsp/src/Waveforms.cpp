#include "dsp/Waveforms.h"
#include "dsp/Math.h"

#include <cmath>
#include <cstdlib>

namespace dsp::waveforms {

float polyBlep(float deltaTime) {
  if (deltaTime > 0.0f) {
    // We just passed the discontinuity (0 < t < 1)
    // Polynomial: t² - 2t + 1
    return deltaTime * deltaTime - 2.0f * deltaTime + 1.0f;
  } else {
    // We're approaching the discontinuity (-1 < t < 0)
    // Polynomial: t² + 2t + 1
    return deltaTime * deltaTime + 2.0f * deltaTime + 1.0f;
  }
}

// Sine wave (band-limited as-is)
float sine(float phase) {
  return std::sin(phase * math::TWO_PI_F);
}

// NOTE:  using normalized phase results in a cool distorted sound
float sineNormalized(float phase) {
  return std::sin(phase);
}

//==== SQUARE WAVEFORMS ====
// Naive Square for LFO and initial PolyBLep value
float squareNaive(float phase, float pulseWidth) {
  return phase < pulseWidth ? 1.0f : -1.0f;
}

// Square wave with PolyBLep anti-aliasing
float square(float phase, float phaseIncrement, float pulseWidth) {
  float value = squareNaive(phase, pulseWidth);

  // Discontinuity at phase = 0.0 (rising edge)
  if (phase < phaseIncrement) {
    float t = phase / phaseIncrement;
    value += polyBlep(t);
  } else if (phase > 1.0f - phaseIncrement) {
    float t = (phase - 1.0f) / phaseIncrement;
    value += polyBlep(t);
  }

  // Discontinuity at phase = pulseWidth (falling edge)
  // Compute phase relative to falling edge, handle wraparound
  float pwmPhase = phase - pulseWidth;
  if (pwmPhase < 0.0f)
    pwmPhase += 1.0f; // wrap negative value

  if (pwmPhase < phaseIncrement) {
    float t = pwmPhase / phaseIncrement;
    value -= polyBlep(t);
  } else if (pwmPhase > 1.0f - phaseIncrement) {
    float t = (pwmPhase - 1.0f) / phaseIncrement;
    value -= polyBlep(t);
  }

  return value;
}

// ===== SAW WAVEFORM ======
// Naive Saw for LFO and initial PolyBLep value
float sawNaive(float phase) {
  return 2.0f * phase - 1.0f;
}

// Saw wave with PolyBLep anti-aliasing
float saw(float phase, float phaseIncrement) {
  float value = sawNaive(phase);

  // Correct the discontinuity at phase wraparound
  if (phase < phaseIncrement) {
    // Just wrapped, t = how far past the discontinuity
    float deltaTime = phase / phaseIncrement;
    value -= polyBlep(deltaTime);
  } else if (phase > 1.0f - phaseIncrement) {
    // About to wrap, t = how far before the discontinuity (negative)
    float t = (phase - 1.0f) / phaseIncrement;
    value -= polyBlep(t);
  }

  return value;
}

// Triangle wave (no slope correction)
float triangle(float phase) {
  return 1.0f - 4.0f * std::abs(phase - 0.5f);
}

// Process oscillator sample value
// NOTE(nico): this probably is going to change
float processWaveform(WaveformType type, float phase, float phaseIncrement, float pulseWidth) {
  switch (type) {
  case WaveformType::WAVEFORM_COUNT:
  case WaveformType::Sine:
    return sine(phase);
  case WaveformType::Saw:
    return saw(phase, phaseIncrement);
  case WaveformType::Square:
    return square(phase, phaseIncrement, pulseWidth);
  case WaveformType::Triangle:
    return triangle(phase);
  }
}

} // namespace dsp::waveforms
