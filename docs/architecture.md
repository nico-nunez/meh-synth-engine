# Architecture

## Table of Contents

- [Overview](#overview)
- [Top-Level Structure](#top-level-structure)
- [Render Flow](#render-flow)
- [Voice Model](#voice-model)
- [Subsystem Boundaries](#subsystem-boundaries)
- [Host Responsibilities](#host-responsibilities)
- [Design Constraints](#design-constraints)

## Overview

`meh-synth-engine` is a platform-independent synth core. It does not open audio devices, read MIDI devices, or provide a UI. Its job is to maintain synthesis state, accept host-driven events, and render audio blocks.

The central object is [`synth::Engine`](../src/synth/Engine.h). It owns the voice pool, FX chain, parameter router, and the scratch buffer used during rendering.

## Top-Level Structure

The engine is split into a few clear layers:

- [`src/synth/Engine.h`](../src/synth/Engine.h)
  The host-facing entry point.
- [`src/synth/VoicePool.h`](../src/synth/VoicePool.h)
  Per-voice synthesis state and voice lifecycle management.
- [`src/synth/params`](../src/synth/params)
  Parameter definitions, binding, validation, and dirty-state synchronization.
- [`src/synth/events`](../src/synth/events)
  Input event types for MIDI, scalar parameter changes, and structural engine updates.
- [`src/synth/preset`](../src/synth/preset)
  Full-state capture, apply, serialization, and file I/O.
- [`libs/dsp`](../libs/dsp)
  Reusable DSP primitives and global effects implementations.

The split matters because hosts should talk to `Engine`, not directly mutate deep subsystem state.

## Render Flow

The host render path is intentionally simple:

1. Create an engine with a sample rate and preferred block size.
2. Feed any pending control changes into the engine.
3. Call `processAudioBlock`.
4. Send the rendered samples to the host output path.

Inside the engine, [`Engine::processAudioBlock`](../src/synth/Engine.cpp) does not process the host block as one giant slab. It slices the block into fixed internal chunks of `ENGINE_BLOCK_SIZE` samples, currently `64`.

That gives the engine two useful rates:

- host block rate
  The rate the outside world calls into the engine
- internal engine block rate
  A cheaper control lane for modulation and other derived work that does not need full per-sample recomputation

The processing sequence is:

1. Slice the host block into internal blocks.
2. Process active voices into the engine's stereo pool buffer.
3. Run the global FX chain after voice summing.
4. Copy the finished stereo result into the host output buffers.

This architecture lets the engine keep modulation and coefficient work more localized while still generating sample-accurate audio output.

## Voice Model

[`VoicePool`](../src/synth/VoicePool.h) is the main state container for synthesis. It contains:

- 4 wavetable oscillators
- 3 envelopes
- 3 LFOs
- noise source state
- per-voice stereo pan data
- filter and saturator state
- modulation matrix state
- mono, portamento, sustain, pitch-bend, and unison state
- voice metadata such as active flags, MIDI note, velocity, and note-on order

Two implementation choices matter for readers:

- Active voices are tracked through a dense `activeIndices` array.
  The engine iterates active voices directly instead of scanning every possible voice slot on every render pass.
- Voice stealing is oldest-note-first.
  When all voice slots are active, the allocator reuses the voice with the oldest `noteOnTime`.

The docs should preserve those behavioral guarantees even if the internals are refactored later.

## Subsystem Boundaries

There are two major signal stages in the current architecture:

- voice stage
  Oscillators, envelopes, modulation, filters, saturator, and voice-level signal-chain ordering
- post-mix FX stage
  Distortion, chorus, phaser, delay, and reverb applied after voices are summed

This is why there are two separate chain systems:

- [`SignalChain`](../src/synth/SignalChain.h)
  Orders per-voice processors
- [`FXChain`](../libs/dsp/include/dsp/fx/FXChain.h)
  Orders global post-mix effects

The engine also draws a hard boundary between:

- scalar parameter changes
- structural graph changes such as FM routing, modulation routes, and chain ordering

That separation shows up in the event model and keeps the public API easier to reason about.

## Host Responsibilities

The host is responsible for everything outside the pure synth engine:

- audio device setup
- audio callback scheduling
- MIDI input collection
- any event queueing and timestamp policy
- UI and automation
- preset browsing UX
- transport or tempo integration policy

The engine is responsible for:

- voice allocation and release
- parameter application
- derived-state synchronization
- preset application and capture
- audio generation

## Design Constraints

A few constraints are important enough to treat as architectural, not incidental:

- The engine is designed as a reusable library, not a standalone app.
- The engine keeps platform dependencies out of `src/synth`.
- Internal block processing is a deliberate part of the modulation strategy.
- The parameter surface is centralized rather than being scattered through subsystem-specific setter APIs.
- Presets are full-state snapshots, not loose bags of optional values.

Those choices make the engine easier to embed in multiple hosts and easier to keep deterministic across different control paths.
