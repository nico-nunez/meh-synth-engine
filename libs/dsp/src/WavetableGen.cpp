#include "dsp/WavetableGen.h"
#include "dsp/Math.h"

#include <algorithm>
#include <cmath>

namespace dsp::wavetable {

namespace {
constexpr float invTableSize = 1.0f / TABLE_SIZE_F;

inline float sineAt(uint32_t harmonic, uint32_t sampleIndex) {
  return sinf(math::TWO_PI_F * static_cast<float>(harmonic) * static_cast<float>(sampleIndex) *
              invTableSize);
}
} // namespace

WavetableBank* generateSine() {
  WavetableBank* bank = createWavetableBank(1, "sine");
  WavetableFrame* frame = &bank->frames[0];

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < MAX_MIP_LEVELS; mipLevel++) {

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < TABLE_SIZE; sampleIndex++) {

      // Pure sine (1 harmonics)
      frame->mips[mipLevel][sampleIndex] = sineAt(1, sampleIndex);
    }
  }

  return bank;
}

WavetableBank* generateSaw() {
  WavetableBank* bank = createWavetableBank(1, "saw");
  WavetableFrame* frame = &bank->frames[0];

  uint32_t tableLimit = TABLE_SIZE / 2;

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < MAX_MIP_LEVELS; mipLevel++) {
    uint32_t mipLimit = std::max(1u, tableLimit >> mipLevel);

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < TABLE_SIZE; sampleIndex++) {
      float sample = 0.0f;

      // Harminic Level (odd & even)
      for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic++) {
        float sign = ((harmonic & 1) == 1) ? 1.0f : -1.0f; // is odd
        sample += sign * sineAt(harmonic, sampleIndex) / static_cast<float>(harmonic);
      }

      frame->mips[mipLevel][sampleIndex] = sample * (2.0f * math::INV_PI_F);
    }
  }

  return bank;
}

WavetableBank* generateSquare() {
  WavetableBank* bank = createWavetableBank(1, "square");
  WavetableFrame* frame = &bank->frames[0];

  uint32_t tableLimit = TABLE_SIZE / 2;

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < MAX_MIP_LEVELS; mipLevel++) {
    uint32_t mipLimit = std::max(1u, tableLimit >> mipLevel);

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < TABLE_SIZE; sampleIndex++) {
      float sample = 0.0f;

      // Harminic Level (only odd)
      for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic += 2) {
        sample += sineAt(harmonic, sampleIndex) / static_cast<float>(harmonic);
      }

      frame->mips[mipLevel][sampleIndex] = sample * (4.0f * math::INV_PI_F);
    }
  }

  return bank;
}

WavetableBank* generateTriangle() {
  WavetableBank* bank = createWavetableBank(1, "triangle");
  WavetableFrame* frame = &bank->frames[0];

  uint32_t tableLimit = TABLE_SIZE / 2;

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < MAX_MIP_LEVELS; mipLevel++) {
    uint32_t mipLimit = std::max(1u, tableLimit >> mipLevel);

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < TABLE_SIZE; sampleIndex++) {
      float sample = 0.0f;

      // Harminic Level (only odd)
      for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic += 2) {
        // Convert odd increments into counter (1,3,5,7...) -> (0,1,2,3...)
        float sign = ((((harmonic - 1) / 2) & 1) == 0) ? 1.0f : -1.0f;

        sample += sign * sineAt(harmonic, sampleIndex) /
                  (static_cast<float>(harmonic) * static_cast<float>(harmonic));
      }

      frame->mips[mipLevel][sampleIndex] = sample * (8.0f * (math::INV_PI_F * math::INV_PI_F));
    }
  }

  return bank;
}

WavetableBank* generateSineToSaw() {
  const uint32_t NUM_FRAMES = 8;
  const uint32_t tableLimit = TABLE_SIZE / 2;

  WavetableBank* bank = createWavetableBank(NUM_FRAMES, "sine_to_saw");

  // Frame Level
  for (uint32_t frameIndex = 0; frameIndex < NUM_FRAMES; frameIndex++) {
    WavetableFrame* frame = &bank->frames[frameIndex];

    // Increase morph (sine -> saw) per frame (0 -> 1)
    // Frame 0 = no morph (0) and pure sine
    // Frame 7 = full morph (1) and pure saw
    float morphAmount = static_cast<float>(frameIndex) / static_cast<float>(NUM_FRAMES - 1);

    uint32_t frameHarmonics =
        std::max(1u, static_cast<uint32_t>(1.0f + morphAmount * (tableLimit - 1)));

    // Mip Level
    for (uint32_t mipLevel = 0; mipLevel < MAX_MIP_LEVELS; mipLevel++) {
      uint32_t mipHarmonics = std::max(1u, tableLimit >> mipLevel);
      uint32_t mipLimit = std::min(frameHarmonics, mipHarmonics);

      // Sample Level
      for (uint32_t sampleIndex = 0; sampleIndex < TABLE_SIZE; sampleIndex++) {
        float sample = 0.0f;

        // Harmonic Level
        for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic++) {
          float sign = ((harmonic & 1) == 1) ? 1.0f : -1.0f;

          sample += sign * sineAt(harmonic, sampleIndex) / static_cast<float>(harmonic);
        }

        frame->mips[mipLevel][sampleIndex] = sample * (2.0f * dsp::math::INV_PI_F);
      }
    }
  }

  return bank;
}

} // namespace dsp::wavetable
