#pragma once

#include "dsp/Buffers.h"

#include <cstdint>

namespace dsp::dynamics {

// Peak limiter with independent attack and release.
// Policy-free: threshold is passed at process time, not stored here.
// Init once at session creation; process once per callback on the master bus.
struct PeakLimiter {
  float envelope = 1.0f;
  float attackCoeff = 0.0f;
  float releaseCoeff = 0.0f;
};

// Compute attack/release one-pole coefficients from time constants in milliseconds.
// Call once at session creation, not in the callback.
void initPeakLimiter(PeakLimiter& lim, float sampleRate, float attackMs, float releaseMs);

// Apply gain-reduction limiting to a stereo buffer in-place.
// thresholdLinear: e.g. dsp::math::dBToLinear(-1.0f) ≈ 0.891f
// Call after master gain, before writing to device.
void processPeakLimiter(PeakLimiter& lim,
                        dsp::buffers::StereoBufferView buf,
                        uint32_t numFrames,
                        float thresholdLinear);

} // namespace dsp::dynamics
