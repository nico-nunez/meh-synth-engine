#pragma once

#include "Types.h"

#include "dsp/Envelope.h"

#include <cstdint>

namespace synth::envelope {
using EnvelopeStatus = dsp::envelopes::Status;
using dsp::envelopes::CurveTable;

inline constexpr float CURVE_DEFAULT_ATTACK = -5.0f;
inline constexpr float CURVE_DEFAULT_DECAY = -5.0f;
inline constexpr float CURVE_DEFAULT_RELEASE = -5.0f;
/* ==== Concave (natural) defaults ====
 * |k| = 1-3: subtle shaping, barely noticeable
 * |k| = 4-6: moderate, musical range (where most presets will live)
 * |k| = 7-10: extreme, aggressive curves
 */

struct Envelope {
  // ==== Per-voice state (hot data) ====
  EnvelopeStatus states[MAX_VOICES];    // Current stage
  float levels[MAX_VOICES];             // Current output (0.0-1.0)
  float progress[MAX_VOICES];           // Progress in stage (0.0-1.0)
  float attackStartLevels[MAX_VOICES];  // Captured on retrigger
  float releaseStartLevels[MAX_VOICES]; // Captured on release

  // ==== Curve Tables (shared and regen on curve change) ====
  CurveTable attackCurve;
  CurveTable decayCurve;
  CurveTable releaseCurve;

  // ==== Global ADSR settings (cold data) ====
  float attackMs = 10.0f;
  float decayMs = 100.0f;
  float sustainLevel = 0.7f;
  float releaseMs = 200.0f;

  // ==== Curve parameters (cold data) ====
  float attackCurveParam = CURVE_DEFAULT_ATTACK;
  float decayCurveParam = CURVE_DEFAULT_DECAY;
  float releaseCurveParam = CURVE_DEFAULT_RELEASE;

  // ==== Pre-calculated increments (updated when UI changes) ====
  float attackIncrement = 0.0f; // 1.0 / (attackMs * 0.001 * sampleRate)
  float decayIncrement = 0.0f;
  float releaseIncrement = 0.0f;
};

void initEnvelope(Envelope& env, uint32_t voiceIndex, float sampleRate);

void triggerRelease(Envelope& env, uint32_t voiceIndex);

void updateCurveTables(Envelope& env);
void updateIncrements(Envelope& env, float sampleRate);

float processEnvelope(Envelope& env, uint32_t voiceIndex);
void retriggerEnvelope(Envelope& env, uint32_t voiceIndex, float sampleRate);

} // namespace synth::envelope
