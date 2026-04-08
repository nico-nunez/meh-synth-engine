#pragma once

#include "dsp/Wavetable.h"

namespace dsp::wavetable {

WavetableBank* generateSine();
WavetableBank* generateSaw();
WavetableBank* generateSquare();
WavetableBank* generateTriangle();
WavetableBank* generateSineToSaw();

} // namespace dsp::wavetable
