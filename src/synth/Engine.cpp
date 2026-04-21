#include "Engine.h"

#include "synth/SignalChain.h"
#include "synth/VoicePool.h"
#include "synth/WavetableOsc.h"
#include "synth/events/Events.h"
#include "synth/params/ParamDefs.h"
#include "synth/params/ParamSync.h"
#include "synth/preset/Preset.h"
#include "synth/preset/PresetApply.h"

#include "dsp/Buffers.h"
#include "dsp/fx/FXChain.h"

#include <algorithm>
#include <cstdio>

namespace synth {

namespace {
void applyMIDICC(Engine& engine, uint8_t cc, uint8_t value) {
  auto& pool = engine.voicePool;

  if (cc == 1) {
    pool.modWheelValue = value / 127.0f;
    return;
  }
  if (cc == 64) {
    bool wasHeld = pool.sustain.held;
    pool.sustain.held = (value >= 64);

    if (wasHeld && !pool.sustain.held) {
      // Mono: handle the one voice directly
      if (pool.mono.enabled) {
        uint32_t v = pool.mono.voiceIndex;

        if (v < MAX_VOICES && pool.sustain.notes[v]) {
          pool.sustain.notes[v] = false;

          if (pool.mono.stackDepth > 0) {
            uint8_t prevNote = pool.mono.noteStack[pool.mono.stackDepth - 1];

            if (pool.porta.enabled) {
              float from = static_cast<float>(pool.midiNotes[v]);
              float to = static_cast<float>(prevNote);
              pool.porta.offsets[v] = (from - to) + pool.porta.offsets[v];
              pool.porta.lastNote = prevNote;
            }

            voices::redirectVoicePitch(pool, v, prevNote, engine.sampleRate);
          } else {
            envelope::triggerRelease(pool.ampEnv, v);
            envelope::triggerRelease(pool.filterEnv, v);
            envelope::triggerRelease(pool.modEnv, v);
          }
        }
        return;
      }

      // Poly: release all deferred voices
      for (uint32_t i = 0; i < MAX_VOICES; ++i) {
        if (pool.sustain.notes[i]) {
          pool.sustain.notes[i] = false;
          envelope::triggerRelease(pool.ampEnv, i);
          envelope::triggerRelease(pool.filterEnv, i);
          envelope::triggerRelease(pool.modEnv, i);
        }
      }
    }
    return;
  }

  ParamID id = ParamID::PARAM_UNKNOWN;

  switch (cc) {
  case 7:
    id = ParamID::MASTER_GAIN;
    break;
  case 74:
    id = ParamID::SVF_CUTOFF;
    break;
  case 71:
    id = ParamID::SVF_RESONANCE;
    break;
  default:
    return;
  }

  const auto& def = getParamDef(id);
  float denorm = def.min + (value / 127.0f) * (def.max - def.min);
  param::sync::setParam(engine, id, denorm);
}

} // namespace

// ================
//  Engine Methods
// ================

// ==== Create and Initialize Engine ====
Engine createEngine(const EngineConfig& config) {

  Engine engine{};

  engine.numFrames = config.numFrames;

  engine.sampleRate = config.sampleRate;
  engine.invSampleRate = 1.0f / config.sampleRate;

  dsp::buffers::initStereoBuffer(engine.poolBuffer, config.numFrames);
  voices::initVoicePool(engine.voicePool);
  dsp::fx::chain::initFXChain(engine.fxChain, engine.bpm, engine.sampleRate);

  param::sync::initParamDefaults(engine);

  auto initPreset = preset::createInitPreset();
  preset::applyPreset(initPreset, engine);

  return engine;
}

void Engine::processParamEvent(const ParamEvent& event) {
  auto id = static_cast<ParamID>(event.id);
  param::sync::setParam(*this, id, event.value);
}

void Engine::processMIDIEvent(const MIDIEvent& event) {
  using Type = MIDIEvent::Type;

  switch (event.type) {
  case Type::NoteOn:
    if (event.data.noteOn.velocity > 0)
      voices::handleNoteOn(voicePool,
                           event.data.noteOn.note,
                           event.data.noteOn.velocity,
                           ++noteCount,
                           sampleRate);
    else
      voices::releaseVoice(voicePool, event.data.noteOn.note, sampleRate);
    break;

  case Type::NoteOff:
    voices::handleNoteOff(voicePool, event.data.noteOff.note, sampleRate);
    break;

  case Type::ControlChange: {
    applyMIDICC(*this, event.data.cc.number, event.data.cc.value);
    break;
  }

  case Type::PitchBend:
    // Normalize value [-8192, 8191] -> [-1.0, 1.0]
    voicePool.pitchBend.value = event.data.pitchBend.value / 8192.0f;
    break;

  // TODO(nico)...at some point
  case Type::Aftertouch:
    break;
  case Type::ChannelPressure:
    break;
  case Type::ProgramChange:
    break;

  default:
    break;
  }
}

void Engine::processEngineEvent(const EngineEvent& event) {
  namespace osc = wavetable::osc;
  namespace mm = mod_matrix;
  namespace sc = signal_chain;
  namespace fx = dsp::fx::chain;

  switch (event.type) {

  case EngineEvent::Type::AddFMRoute: {
    auto* carrier =
        voices::getOscByEnum(voicePool, static_cast<osc::FMCarrier>(event.data.addFMRoute.carrier));
    if (!carrier)
      return;
    osc::addFMRoute(*carrier,
                    static_cast<osc::FMSource>(event.data.addFMRoute.source),
                    event.data.addFMRoute.depth);
    return;
  }

  case EngineEvent::Type::RemoveFMRoute: {
    auto* carrier =
        voices::getOscByEnum(voicePool,
                             static_cast<osc::FMCarrier>(event.data.removeFMRoute.carrier));
    if (!carrier)
      return;
    osc::removeFMRoute(*carrier, static_cast<osc::FMSource>(event.data.removeFMRoute.source));
    return;
  }

  case EngineEvent::Type::ClearFMRoutes: {
    auto* carrier =
        voices::getOscByEnum(voicePool,
                             static_cast<osc::FMCarrier>(event.data.clearFMRoutes.carrier));
    if (!carrier)
      return;
    osc::clearFMRoutes(*carrier);
    return;
  }

  case EngineEvent::Type::AddModRoute: {
    mm::addRoute(voicePool.modMatrix,
                 static_cast<mm::ModSrc>(event.data.addModRoute.source),
                 static_cast<mm::ModDest>(event.data.addModRoute.destination),
                 event.data.addModRoute.amount);
    return;
  }

  case EngineEvent::Type::RemoveModRoute: {
    mm::removeRoute(voicePool.modMatrix, event.data.removeModRoute.routeIndex);
    return;
  }

  case EngineEvent::Type::ClearModRoutes: {
    mm::clearRoutes(voicePool.modMatrix);
    return;
  }

  case EngineEvent::Type::SetSignalChain: {
    sc::SignalProcessor procs[sc::MAX_CHAIN_SLOTS];
    uint8_t count = event.data.setSignalChain.count;
    for (uint8_t i = 0; i < count; ++i)
      procs[i] = static_cast<sc::SignalProcessor>(event.data.setSignalChain.processors[i]);
    sc::setSigChain(voicePool.signalChain, procs, count);
    return;
  }

  case EngineEvent::Type::ClearSignalChain: {
    sc::clearSigChain(voicePool.signalChain);
    return;
  }

  case EngineEvent::Type::SetFXChain: {
    fx::FXProcessor procs[fx::MAX_EFFECT_SLOTS];
    uint8_t count = event.data.setFXChain.count;
    for (uint8_t i = 0; i < count; ++i)
      procs[i] = static_cast<fx::FXProcessor>(event.data.setFXChain.processors[i]);
    fx::setFXChain(fxChain, procs, count);
    return;
  }

  case EngineEvent::Type::ClearFXChain: {
    fx::clearFXChain(fxChain);
    return;
  }

  case EngineEvent::Type::ApplyPreset: {
    if (!event.data.applyPreset.preset)
      return;
    preset::applyPreset(*event.data.applyPreset.preset, *this);
    return;
  }

  case EngineEvent::Type::Panic:
    voices::panicVoicePool(voicePool);
    return;
  }
}

void Engine::renderVoicesRange(uint32_t startFrame, uint32_t endFrame) {
  while (startFrame < endFrame) {
    uint32_t chunkSize = std::min<uint32_t>(ENGINE_BLOCK_SIZE, endFrame - startFrame);
    auto bufferSlice = dsp::buffers::createStereoBufferSlice(poolBuffer, startFrame);
    voices::processVoices(voicePool, bufferSlice, chunkSize, invSampleRate);
    startFrame += chunkSize;
  }
}

void Engine::applyScheduledEvent(const ScheduledEvent& event) {
  switch (event.kind) {
  case ScheduledEvent::Kind::MIDI:
    processMIDIEvent(event.data.midi);
    return;
  case ScheduledEvent::Kind::Param:
    processParamEvent(event.data.param);
    return;
  case ScheduledEvent::Kind::Engine:
    processEngineEvent(event.data.engine);
    return;
  }
}

/* NOTE: Use internal Engine block size to allow processing of
 * expensive calculation that need to occur more often than once per audio
 * buffer block but NOT on every sample either.  E.g. Modulation
 */
void Engine::processAudioBlock(float** outputBuffer,
                               size_t numChannels,
                               size_t numFrames,
                               RenderContext ctx) {
  if (numFrames == 0)
    return;

  if (bpm != ctx.bpm) {
    bpm = ctx.bpm;
    dirtyFlags.mark(param::UpdateGroup::BPMSync);
    param::sync::syncDirtyParams(*this);
  }

  uint32_t currentFrame = 0;
  uint32_t evt = 0;

  while (ctx.events && evt < ctx.numEvents) {
    uint32_t offsetFrame = ctx.events[evt].sampleOffset;

    if (currentFrame < offsetFrame)
      renderVoicesRange(currentFrame, offsetFrame);

    currentFrame = offsetFrame;

    do {
      applyScheduledEvent(ctx.events[evt++]);
    } while (evt < ctx.numEvents && ctx.events[evt].sampleOffset == offsetFrame);
  }

  if (currentFrame < numFrames)
    renderVoicesRange(currentFrame, static_cast<uint32_t>(numFrames));

  dsp::fx::chain::processFXChain(fxChain, poolBuffer, numFrames);

  for (size_t frame = 0; frame < numFrames; frame++) {
    if (numChannels == 0)
      continue;

    // Mono
    if (numChannels == 1)
      outputBuffer[0][frame] = (poolBuffer.left[frame] + poolBuffer.right[frame]) * 0.5f;

    // TODO(nico): handle for more than just stereo channels
    if (numChannels >= 2) {
      outputBuffer[0][frame] = poolBuffer.left[frame];
      outputBuffer[1][frame] = poolBuffer.right[frame];
    }
  }
}

} // namespace synth
