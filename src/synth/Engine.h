#pragma once

#include "synth/VoicePool.h"
#include "synth/events/Events.h"
#include "synth/params/ParamDefs.h"

#include "dsp/Buffers.h"
#include "dsp/Waveforms.h"
#include "dsp/fx/FXChain.h"

#include <cstddef>
#include <cstdint>

namespace synth {
using events::EngineEvent;
using events::MIDIEvent;
using events::ParamEvent;
using events::ScheduledEvent;

using dsp::fx::chain::FXChain;
using voices::VoicePool;

using dsp::buffers::StereoBuffer;
using dsp::waveforms::WaveformType;

using param::ParamID;
using param::UpdateGroup;
using param::UpdateGroupFlags;

// --- Constants ---
inline constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
inline constexpr uint32_t DEFAULT_FRAMES = 512;
inline constexpr uint16_t DEFAULT_CHANNELS = 2;

struct EngineConfig {
  uint32_t numFrames = DEFAULT_FRAMES;
  float sampleRate = DEFAULT_SAMPLE_RATE;
};

struct RenderContext {
  float bpm = 120.0f;

  ScheduledEvent* events = nullptr; // optional
  size_t numEvents = 0;
};

struct Engine {
  uint32_t numFrames = DEFAULT_FRAMES;

  float sampleRate = DEFAULT_SAMPLE_RATE;
  float invSampleRate = 1.0f / sampleRate;

  float bpm = 120.0f;

  float params[param::PARAM_COUNT]{};

  VoicePool voicePool;
  FXChain fxChain;
  StereoBuffer poolBuffer{};
  uint32_t noteCount = 0;
  UpdateGroupFlags dirtyFlags;

  void processMIDIEvent(const MIDIEvent& event);
  void processParamEvent(const ParamEvent& event);
  void processEngineEvent(const EngineEvent& event);

  void renderVoicesRange(uint32_t startFrame, uint32_t frameCount);
  void applyScheduledEvent(const ScheduledEvent& event);

  void
  processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames, RenderContext ctx);
};

Engine createEngine(const EngineConfig& config);

} // namespace synth
