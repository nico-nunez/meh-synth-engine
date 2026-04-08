#pragma once

namespace dsp::filters {
struct SVFOutputs {
  float lp, bp, hp;
};

// ==== Cytomic / TPT Form ====
struct SVFState {
  float ic1 = 0.0f; // integrator 1 capacitor state
  float ic2 = 0.0f; // integrator 2 capacitor state
};

struct SVFCoeffs {
  float a1, a2, a3;
  float k; // damping = 1/Q
};

float modulateCutoff(float baseCutoff, float modulationOctaves);

SVFCoeffs computeSVFCoeffs(float normalizedFreq, float Q);
SVFOutputs processSVF(float input, const SVFCoeffs& c, SVFState& s);

// ==== Ladder Filter (Moog style) ====
struct LadderState {
  float s[4] = {0, 0, 0, 0};
};

float processLadder(float input, float feqCoeff, float resonance, LadderState& state);
float processLadderNonlinear(float input,
                             float freqCoeff,
                             float resonance,
                             float drive,
                             float& y4Estimate,
                             LadderState& state);

/* DC Block is a high-pass filter that removes DC offset (constant bias) from
 * the audio signal.
 *
 * Why you need it:
 * - Saturation creates DC - asymmetric clipping shifts the average value
 * - Oscillator drift - numerical errors can accumulate over time
 * - Speaker protection - DC can damage speakers (wasted power, cone movement)
 *
 * The coefficient (default 0.995):
 * - Higher (0.999) = removes ONLY DC, preserves sub-bass
 * - Lower (0.99) = more aggressive, removes more low frequencies
 * - 0.995 is a good default - cutoff around 3-5 Hz
 */
float dcBlock(float sample, float& state, float coefficient = 0.995f);

} // namespace dsp::filters
