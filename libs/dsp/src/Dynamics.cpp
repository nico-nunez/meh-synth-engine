#include "dsp/Dynamics.h"

#include <algorithm>
#include <cmath>

namespace dsp::dynamics {

void initPeakLimiter(PeakLimiter& limiter, float sampleRate, float attackMs, float releaseMs) {
  limiter.envelope = 1.0f;
  limiter.attackCoeff = std::exp(-1.0f / (sampleRate * attackMs * 0.001f));
  limiter.releaseCoeff = std::exp(-1.0f / (sampleRate * releaseMs * 0.001f));
}

void processPeakLimiter(PeakLimiter& limiter,
                        buffers::StereoBufferView buf,
                        uint32_t numFrames,
                        float threshold) {
  for (uint32_t i = 0; i < numFrames; ++i) {
    float peak = std::max(std::abs(buf.left[i]), std::abs(buf.right[i]));

    float targetGain = (peak > threshold) ? threshold / peak : 1.0f;
    float coeff = (targetGain < limiter.envelope) ? limiter.attackCoeff : limiter.releaseCoeff;

    limiter.envelope = targetGain + coeff * (limiter.envelope - targetGain);

    buf.left[i] *= limiter.envelope;
    buf.right[i] *= limiter.envelope;
  }
}

} // namespace dsp::dynamics
