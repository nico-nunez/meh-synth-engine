#include "Envelope.h"

#include <algorithm>
#include <cstdint>

namespace synth::envelope {

void initEnvelope(Envelope& env, uint32_t voiceIndex, float sampleRate) {
  env.states[voiceIndex] = EnvelopeStatus::Attack;
  env.levels[voiceIndex] = 0.0f;
  env.progress[voiceIndex] = 0.0f;
  env.attackStartLevels[voiceIndex] = 0.0f;

  updateIncrements(env, sampleRate);
}

void retriggerEnvelope(Envelope& env, uint32_t voiceIndex, float sampleRate) {
  env.attackStartLevels[voiceIndex] = env.levels[voiceIndex];
  env.states[voiceIndex] = EnvelopeStatus::Attack;
  env.progress[voiceIndex] = 0.0f;

  updateIncrements(env, sampleRate);
}

void updateCurveTables(Envelope& env) {
  using dsp::envelopes::generateCurveTable;

  generateCurveTable(env.attackCurve, env.attackCurveParam);
  generateCurveTable(env.decayCurve, env.decayCurveParam);
  generateCurveTable(env.releaseCurve, env.releaseCurveParam);
}

void updateIncrements(Envelope& env, float sampleRate) {
  env.attackIncrement = 1.0f / (std::max(env.attackMs, 0.001f) * 0.001f * sampleRate);
  env.decayIncrement = 1.0f / (std::max(env.decayMs, 0.001f) * 0.001f * sampleRate);
  env.releaseIncrement = 1.0f / (std::max(env.releaseMs, 0.001f) * 0.001f * sampleRate);
}

float processEnvelope(Envelope& env, uint32_t voiceIndex) {
  float level = dsp::envelopes::processADSRCurved(env.states[voiceIndex],
                                                  env.levels[voiceIndex],
                                                  env.progress[voiceIndex],
                                                  env.attackStartLevels[voiceIndex],
                                                  env.releaseStartLevels[voiceIndex],
                                                  env.attackIncrement,
                                                  env.decayIncrement,
                                                  env.releaseIncrement,
                                                  env.sustainLevel,
                                                  env.attackCurve,   // NEW
                                                  env.decayCurve,    // NEW
                                                  env.releaseCurve); // NEW

  return level;
}

void triggerRelease(Envelope& env, uint32_t voiceIndex) {
  env.states[voiceIndex] = EnvelopeStatus::Release;
  env.releaseStartLevels[voiceIndex] = env.levels[voiceIndex];
  env.progress[voiceIndex] = 0.0f;
}

} // namespace synth::envelope
