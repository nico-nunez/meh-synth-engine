#pragma once

namespace dsp::waveshaper {
float hardClip(float sample, float threshold);

// NOTE(nico): _drive_ and _invDrive_ should be denormalized
// but _mix_ still normalized
float softClip(float sample, float drive, float invDrive, float mix = 1.0);

// NOTE(nico): drive should be denormalized, but
// bias should be normalized.
// Callers responsibility to ensure drive is not too much
float tapeSimulation(float sample, float drive, float bias);

// ==== Alternatives ====
// Algebraic soft clip — cheaper than tanh, slightly brighter character
float saturate_soft(float x);

// Asymmetric — different compression on positive vs negative halves
// Adds even harmonics (2nd, 4th) = "warmth", transistor-like
float saturate_asymm(float x);

// NOTE(nico): Revisit this for creative effect
// Less for protection and requires control of input levels.  Can't be too hot
float softClipAlt(float x);

float softLimit(float x, float T = 0.9f);
float fastSoftLimit(float x, float T = 0.9f);
} // namespace dsp::waveshaper
