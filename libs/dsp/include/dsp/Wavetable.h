#pragma once

#include <cstdint>

namespace dsp::wavetable {
inline constexpr uint32_t TABLE_SIZE = 2048; // must be a power of 2
inline constexpr float TABLE_SIZE_F = static_cast<float>(TABLE_SIZE);

// covers ~11 octaves (MIDI 0–127 is ~10.5)
inline constexpr uint8_t MAX_MIP_LEVELS = 11;

inline constexpr uint32_t MAX_FRAMES = 256;
inline constexpr uint8_t MAX_BANK_NAME_LEN = 64;

// Index wrap mask — used in readTable to wrap 2047 → 0
inline constexpr uint32_t TABLE_MASK = TABLE_SIZE - 1; // 0x7FF

// One single-cycle waveform at all mip levels
struct WavetableFrame {
  float mips[MAX_MIP_LEVELS][TABLE_SIZE];
};

struct WavetableBank {
  WavetableFrame* frames;  // heap-allocated: frameCount frames
  uint32_t frameCount = 1; // must be non-zero
  char name[MAX_BANK_NAME_LEN];
};

// Lifecycle
WavetableBank* createWavetableBank(uint32_t frameCount, const char* name);
bool destroyWavetableBank(WavetableBank* bank);

// Linear table lookup — phase is float in [0, TABLE_SIZE_F)
float readTable(const float* table, float phase);

/* Returns a continuous float mip level for the given phase increment.
 * phaseIncrement must be in TABLE_SIZE units (table positions/sample):
 * pass osc.phaseIncrements[v] * TABLE_SIZE_F  (or the interpolated
 * equivalent) Fractional part is the blend weight between mip floor and
 * ceiling. Call per-sample from the interpolated pitch increment — not just at
 * note-on.
 */
float selectMipLevel(float phaseIncrement);

float readWavetable(const WavetableBank* bank,
                    float phase,
                    float mipF,
                    float effectiveScanPos,
                    float fmPhaseOffset);

// Process oscillator (read table and increment phase)
float processWavetableOsc(const WavetableBank* bank,
                          float& phase,
                          float mipF,
                          float effectiveScanPos,
                          float fmPhaseOffset,
                          float pitchIncrement);

} // namespace dsp::wavetable
