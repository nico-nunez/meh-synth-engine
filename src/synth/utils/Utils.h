#pragma once

namespace synth::utils {

float semitoneToFrequency(int semitones);

float midiToFrequency(int midiValue);

float dBtoLinear(float dB);
float linearToDb(float dB);

} // namespace synth::utils
