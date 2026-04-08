#include "dsp/Waveshaper.h"
#include "dsp/Math.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace dsp::waveshaper {
// Linear below T, soft knee above — no harmonic distortion in normal operation
float softLimit(float x, float T) {
  float ax = fabsf(x);
  if (ax <= T)
    return x;
  // Soft knee from T to 1.0
  float excess = ax - T;
  float knee = T + (1.0f - T) * tanhf(excess / (1.0f - T));
  return std::copysignf(knee, x);
}

// Linear below T, soft knee above — no harmonic distortion in normal operation
float fastSoftLimit(float x, float T) {
  float ax = fabsf(x);
  if (ax <= T)
    return x;
  // Soft knee from T to 1.0
  float excess = ax - T;
  float range = 1.0f - T;
  float t = excess / range;                  // normalize to [0, inf)
  float knee = T + range * (t / (1.0f + t)); // Padé-style, hard limits at 1.0 asymptotically
  return std::copysignf(knee, x);
}

// Denormalized drive and invDrive
// mix expected to be normalized [0,1]
float softClip(float sample, float drive, float invDrive, float mix) {
  // Check for denormalized range
  assert(drive >= 0.0f);

  float saturated = math::fastTanh(sample * drive) * invDrive;
  return sample * (1.0f - mix) + (saturated * mix);
}

// NOTE(nico): Revisit this for creative effect
// Less for protection and requires control of input levels
// Can't be too hot
float softClipAlt(float x) {
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return x - (x * x * x) / 3.0f; // x - x³/3
}

float hardClip(float sample, float threshold) {
  return std::clamp(sample, -threshold, threshold);
}

// NOTE(nico): drive should be denormalized, but
// bias should be normalized
// Callers responsibility to ensure drive is not too much
float tapeSimulation(float sample, float drive, float bias) {
  float x = sample * drive + bias * 0.1f;

  // Asymmetric clipping (tape characteristic)
  if (x > 0.0f) {
    x = x / (1.0f + x);
  } else {
    x = x / (1.0f - 0.7f * x); // Different curve for negative
  }

  return x;
}

// Algebraic soft clip — cheaper than tanh, slightly brighter character
float saturate_soft(float x) {
  return x / (1.0f + std::abs(x));
}

// Asymmetric — different compression on positive vs negative halves
// Adds even harmonics (2nd, 4th) = "warmth", transistor-like
float saturate_asymm(float x) {
  return x > 0.0f ? math::fastTanh(x * 1.2f) : math::fastTanh(x * 0.8f);
}
} // namespace dsp::waveshaper
