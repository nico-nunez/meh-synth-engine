# meh-synth-engine

`meh-synth-engine` is a standalone C++ synth engine library extracted from a larger synth application. It owns voice allocation, synthesis, modulation, filtering, FX, parameter routing, and preset application, while staying independent from any audio, MIDI, UI, or platform layer.

The project builds as a static library and can also be embedded directly into another Makefile via [`engine.mk`](engine.mk).

For subsystem-level documentation, see [`docs/`](docs/README.md).

## What It Includes

- 4 wavetable oscillators per voice
- 64-voice polyphony
- 3 envelopes: amp, filter, and modulation
- 3 LFOs with retrigger, fade-in, and tempo-sync support
- 16-slot modulation matrix
- Configurable per-voice signal chain: `svf`, `ladder`, `saturator`
- Post-voice FX chain: distortion, chorus, phaser, delay, reverb
- Mono, legato, portamento, pitch bend, and unison support
- Preset capture/apply and JSON serialization support
- Event-driven API for MIDI, scalar params, and higher-level engine commands

## Project Layout

- [`src/synth`](src/synth) contains the engine and synth-specific subsystems.
- [`src/synth/Engine.h`](src/synth/Engine.h) is the main embedding surface.
- [`src/synth/VoicePool.h`](src/synth/VoicePool.h) holds voice state and the per-voice signal path.
- [`src/synth/params`](src/synth/params) defines parameters, ranges, routing, and dirty-sync behavior.
- [`src/synth/events/Events.h`](src/synth/events/Events.h) defines the engine-facing event types.
- [`src/synth/preset`](src/synth/preset) handles preset representation, apply/capture, and serialization.
- [`libs/dsp`](libs/dsp) provides the reusable DSP primitives and FX implementations the engine builds on.
- [`libs/json`](libs/json) is a small JSON dependency used by the preset layer.

## Build

Build the static library:

```bash
make
```

This produces:

```text
libmeh-synth-engine.a
```

Clean artifacts:

```bash
make clean
```

The default build uses `clang++` with C++17 and debug-friendly flags from [`Makefile`](Makefile).

## Embedding In Another Project

You can either link the built static library, or include the engine sources directly from another Makefile using [`engine.mk`](engine.mk).

Example:

```make
include path/to/meh-synth-engine/engine.mk

SOURCES += $(ENGINE_SOURCES)
CPPFLAGS += $(ENGINE_INCLUDES)
```

That exposes:

- `ENGINE_SOURCES`
- `ENGINE_INCLUDES`

## Core API

Create an engine with a host-provided sample rate and buffer size:

```cpp
#include "synth/Engine.h"

synth::EngineConfig config;
config.sampleRate = 48000.0f;
config.numFrames = 512;

auto engine = synth::createEngine(config);
```

Drive it in three stages:

1. Feed incoming control data with `processMIDIEvent`, `processParamEvent`, and `processEngineEvent`.
2. Ask the engine to render audio with `processAudioBlock`.
3. Copy or stream the generated samples through your host audio backend.

Example:

```cpp
engine.processMIDIEvent(noteOnEvent);
engine.processParamEvent(paramEvent);
engine.processEngineEvent(engineEvent);

engine.processAudioBlock(outputBuffer, numChannels, numFrames);
```

The host owns:

- audio device setup
- MIDI collection and timestamping
- thread scheduling
- buffer lifetime
- UI and automation

The engine owns:

- synthesis state
- voice lifecycle
- parameter-to-state synchronization
- modulation
- DSP processing
- preset application

## Event Model

The engine exposes three input paths in [`Events.h`](src/synth/events/Events.h):

- `MIDIEvent`
  Handles note on/off, CC, pitch bend, and related channel events.
- `ParamEvent`
  Updates a scalar parameter by `ParamID` and value.
- `EngineEvent`
  Handles non-scalar structural changes such as FM routing, modulation routes, signal-chain order, FX-chain order, preset application, and panic.

This split is important:

- `ParamEvent` is for continuous or automatable values.
- `EngineEvent` is for topology changes that do not fit the scalar parameter model.

## Processing Model

At the top level, [`Engine::processAudioBlock`](src/synth/Engine.cpp) renders audio in internal 64-sample chunks, regardless of the host buffer size. That gives the engine a cheaper control-rate lane for modulation and other block-rate work without pushing everything down to per-sample recalculation.

The rendering flow is:

1. Slice the host block into internal engine blocks.
2. Process all active voices through [`VoicePool`](src/synth/VoicePool.h).
3. Sum into a shared stereo pool buffer.
4. Run the global FX chain.
5. Copy the result into the host output buffers.

## Synthesis Architecture

[`VoicePool`](src/synth/VoicePool.h) is the main state container for voice-level synthesis. It contains:

- oscillator state for 4 wavetable oscillators
- envelope, LFO, filter, and saturator state
- modulation matrix state
- mono/unison/portamento state
- per-voice metadata such as MIDI note, velocity, and active voice indices

Important design choices visible in the code:

- Fixed maximum voice count: `MAX_VOICES = 64`
- Fixed oscillator count: `NUM_OSCS = 4`
- Dense `activeIndices` tracking for iterating only active voices
- Block-rate modulation prepass with interpolation across the internal engine block
- No platform dependencies inside the synth engine itself

## Parameters And Dirty Sync

Parameters are declared centrally in [`src/synth/params/ParamDefs.h`](src/synth/params/ParamDefs.h). Each parameter defines:

- canonical name
- type
- range
- default value
- update group

When a parameter changes, the engine marks its update group dirty and synchronizes any derived state that depends on it. That keeps the public control surface broad without forcing every parameter write to immediately recompute every downstream coefficient.

Examples of derived updates in the current design:

- filter coefficient recalculation
- envelope time and curve table refresh
- unison spread and gain compensation
- tempo-synced LFO and delay timing
- effect-specific derived state

## Modulation, Routing, And Chains

There are three separate routing systems:

### FM routing

Each oscillator can hold a set of FM routes. These are managed through `EngineEvent` rather than scalar parameters.

### Modulation matrix

[`ModMatrix`](src/synth/ModMatrix.h) supports up to 16 routes. Current sources include:

- envelopes
- LFOs
- velocity
- noise
- mod wheel
- key tracking

Current destinations include:

- filter cutoff and resonance
- oscillator pitch, mix level, scan position, and FM depth
- LFO rate and amplitude

### Signal and FX chain ordering

- [`SignalChain`](src/synth/SignalChain.h) controls per-voice processor order.
- [`FXChain`](libs/dsp/include/dsp/fx/FXChain.h) controls post-mix global effects order.

Both are configurable through `EngineEvent`.

## Presets

The preset system lives under [`src/synth/preset`](src/synth/preset).

A preset contains:

- metadata
- a full snapshot of scalar parameter values
- FM routing per oscillator
- modulation matrix routes
- signal-chain ordering
- FX-chain ordering

[`createEngine`](src/synth/Engine.cpp) applies an init preset during startup, so a fresh engine always begins from a complete known state rather than partially initialized defaults.

## Current Scope

This repository is the synth engine only. It does not currently provide:

- an audio backend
- a MIDI backend
- a UI
- a CLI or standalone executable target
- tests or benchmarks in this repository

Those are expected to live in a host application or integration layer outside this repo.
