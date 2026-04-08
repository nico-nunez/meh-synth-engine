#pragma once

#include <cstdint>
namespace dsp::envelopes {

enum class Status { Idle, Attack, Decay, Sustain, Release };

inline constexpr uint32_t CURVE_TABLE_SIZE = 256;

struct CurveTable {
  float values[CURVE_TABLE_SIZE];
};

void generateCurveTable(CurveTable& table, float curveParam);

float readCurveTable(const CurveTable& table, float progress);

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
                        const CurveTable& releaseCurve);

// Linear (original version)
float processADSR(Status& state,
                  float& amplitude,
                  float& progress,
                  float& releaseStartLevel,
                  float attackInc,
                  float decayInc,
                  float releaseInc,
                  float sustainLevel);

} // namespace dsp::envelopes
