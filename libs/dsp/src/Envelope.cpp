#include "dsp/Envelope.h"
#include "dsp/Math.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace dsp::envelopes {

void generateCurveTable(CurveTable& table, float curveParam) {
  constexpr uint32_t SIZE = CURVE_TABLE_SIZE;
  constexpr float INV_SIZE = 1.0f / static_cast<float>(SIZE - 1);

  // Linear
  if (std::abs(curveParam) < 0.001f) {
    for (uint32_t i = 0; i < SIZE; i++) {
      table.values[i] = static_cast<float>(i) * INV_SIZE;
    }
    return;
  }

  // Exponential
  float expK = std::exp(curveParam);
  float invDenominator = 1.0f / (expK - 1.0f);

  for (uint32_t i = 0; i < SIZE; i++) {
    float progress = static_cast<float>(i) * INV_SIZE;
    table.values[i] = math::expCurve(progress, curveParam, invDenominator);
  }
}

float readCurveTable(const CurveTable& table, float progress) {
  assert(progress >= 0.0f && progress <= 1.0f);

  constexpr uint32_t LAST = CURVE_TABLE_SIZE - 1;

  float indexF = progress * static_cast<float>(LAST);
  uint32_t i0 = static_cast<uint32_t>(indexF);
  float frac = indexF - static_cast<float>(i0);

  // When i0 == LAST, both entries read the same final value
  uint32_t i1 = i0 + (i0 < LAST);

  return table.values[i0] + frac * (table.values[i1] - table.values[i0]);
}

float processADSRCurved(Status& state,
                        float& amplitude,
                        float& progress,
                        float attackStartLevel,
                        float releaseStartLevel,
                        float attackInc,
                        float decayInc,
                        float releaseInc,
                        float sustainLevel,
                        const CurveTable& attackCurve,
                        const CurveTable& decayCurve,
                        const CurveTable& releaseCurve) {
  switch (state) {
  case Status::Attack:
    progress += attackInc;
    if (progress >= 1.0f) {
      state = Status::Decay;
      progress = 0.0f;
      amplitude = 1.0f;
    } else {
      float shaped = readCurveTable(attackCurve, progress);
      amplitude = attackStartLevel + (1.0f - attackStartLevel) * shaped;
    }
    break;

  case Status::Decay:
    progress += decayInc;
    if (progress >= 1.0f) {
      state = Status::Sustain;
      amplitude = sustainLevel;
    } else {
      float shaped = readCurveTable(decayCurve, progress);
      amplitude = 1.0f + (sustainLevel - 1.0f) * shaped;
    }
    break;

  case Status::Sustain:
    amplitude = sustainLevel;
    break;

  case Status::Release:
    progress += releaseInc;
    if (progress >= 1.0f) {
      state = Status::Idle;
      amplitude = 0.0f;
    } else {
      float shaped = readCurveTable(releaseCurve, progress);
      amplitude = releaseStartLevel * (1.0f - shaped);
    }
    break;

  case Status::Idle:
    amplitude = 0.0f;
    break;
  }

  return amplitude;
}

// Linear (original version)
float processADSR(Status& state,
                  float& amplitude,
                  float& progress,
                  float& releaseStartLevel,
                  float attackInc,
                  float decayInc,
                  float releaseInc,
                  float sustainLevel) {
  switch (state) {
  case Status::Attack:
    progress += attackInc;
    if (progress >= 1.0f) {
      state = Status::Decay;
      progress = 0.0f;
      amplitude = 1.0f;
    } else {
      amplitude = progress;
    }
    break;

  case Status::Decay:
    progress += decayInc;
    if (progress >= 1.0f) {
      state = Status::Sustain;
      amplitude = sustainLevel;
    } else {
      amplitude = 1.0f - progress * (1.0f - sustainLevel);
    }
    break;

  case Status::Sustain:
    amplitude = sustainLevel;
    break;

  case Status::Release:
    progress += releaseInc;
    if (progress >= 1.0f) {
      state = Status::Idle;
      amplitude = 0.0f;
    } else {
      amplitude = releaseStartLevel * (1.0f - progress);
    }
    break;

  case Status::Idle:
    amplitude = 0.0f;
    break;
  }

  return amplitude;
};
} // namespace dsp::envelopes
