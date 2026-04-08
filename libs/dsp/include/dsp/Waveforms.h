#pragma once

namespace dsp::waveforms {

enum class WaveformType { Sine, Saw, Square, Triangle, WAVEFORM_COUNT };

// Sine wave (band-limited as-is)
float sine(float phase);

// NOTE:  using normalized phase results in a cool distorted sound
float sineNormalized(float phase);

// Naive Saw for LFO and initial PolyBLep value
float sawNaive(float phase);

// Saw wave with PolyBLep anti-aliasing
float saw(float phase, float phaseIncrement);

// Naive square for LFO and initial PolyBLep value
float squareNaive(float phase, float pulseWidth = 0.5f);

// Saw wave with PolyBLep anti-aliasing
float square(float phase, float phaseIncrement, float pulseWidth = 0.5f);

// Triangle wave (no slope correction)
float triangle(float phase);

// Process block
float processWaveform(WaveformType type,
                      float phase,
                      float phaseIncrement = 0.0,
                      float pulseWidth = 0.5f);
} // namespace dsp::waveforms
