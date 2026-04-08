#include "Utils.h"

#include "synth/Types.h"

#include "dsp/Math.h"

#include <cfloat>
#include <cmath>

namespace synth::utils {

float semitoneToFrequency(int semitones) {
  // Cast `semitones` to float since std::pow is optimized for floats(32-bit)
  return ROOT_NOTE_FREQ *
         static_cast<float>(std::pow(dsp::math::SEMITONE_RATIO, static_cast<float>(semitones)));
}

float midiToFrequency(int midiValue) {
  return semitoneToFrequency(midiValue - ROOT_NOTE_MIDI);
}

// ==== DB Conversion Helpers ====
float dBtoLinear(float dB) {
  return std::pow(10.0f, dB / 20.0f);
}

float linearToDb(float linear) {
  if (linear <= 0.0f)
    return -FLT_MAX;
  return 20.0f * std::log10f(linear);
}

} // namespace synth::utils
