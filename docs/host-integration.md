# Host Integration

## Table of Contents

- [Overview](#overview)
- [What The Host Must Provide](#what-the-host-must-provide)
- [Basic Lifetime](#basic-lifetime)
- [Event Delivery Model](#event-delivery-model)
- [Render Loop Example](#render-loop-example)
- [Buffer And Channel Expectations](#buffer-and-channel-expectations)
- [Realtime Considerations](#realtime-considerations)
- [Integration Boundaries](#integration-boundaries)

## Overview

This engine is designed to be embedded in another application. The host owns device I/O, timing, threading, and event collection; the engine owns synthesis state and audio rendering.

The goal of this doc is not to prescribe one framework. It is to show the minimum contract a host must satisfy to drive the engine correctly.

## What The Host Must Provide

A host integration needs to provide:

- a sample rate
- an output buffer and channel count
- a render callback or equivalent processing loop
- a path for MIDI events
- a path for scalar parameter changes
- a path for structural engine events

The engine does not require a specific transport or plugin API. It only needs ordered control messages and a render call.

## Basic Lifetime

The typical lifecycle is:

1. Decide the sample rate and preferred block size.
2. Create the engine with [`synth::createEngine`](../src/synth/Engine.h).
3. Collect events on the host side.
4. Drain those events into the engine before rendering each audio block.
5. Call `processAudioBlock`.
6. Destroy the engine when the host shuts down.

Minimal creation example:

```cpp
#include "synth/Engine.h"

synth::EngineConfig config;
config.sampleRate = 48000.0f;
config.numFrames = 512;

synth::Engine engine = synth::createEngine(config);
```

## Event Delivery Model

The engine expects the host to deliver three categories of input:

- [`MIDIEvent`](../src/synth/events/Events.h)
- [`ParamEvent`](../src/synth/events/Events.h)
- [`EngineEvent`](../src/synth/events/Events.h)

The repo also includes simple ring buffers in [`src/synth/events/EventQueues.h`](../src/synth/events/EventQueues.h) for these event types. They are useful when the host wants a lightweight producer/consumer boundary between UI, MIDI, or control threads and the audio/render thread.

The intended pattern is straightforward:

1. Push events into queues from the non-audio side.
2. Pop and apply them on the render side before audio generation.

That keeps engine mutation aligned with the render thread instead of having multiple threads poke engine state directly.

## Render Loop Example

This example shows the intended integration shape:

```cpp
#include "synth/Engine.h"
#include "synth/events/EventQueues.h"

synth::Engine engine = synth::createEngine({512, 48000.0f});

synth::events::MIDIEventQueue midiQueue;
synth::events::ParamEventQueue paramQueue;
synth::events::EngineEventQueue engineQueue;

void render(float** outputs, size_t numChannels, size_t numFrames) {
  synth::events::MIDIEvent midiEvent;
  while (midiQueue.pop(midiEvent))
    engine.processMIDIEvent(midiEvent);

  synth::events::ParamEvent paramEvent;
  while (paramQueue.pop(paramEvent))
    engine.processParamEvent(paramEvent);

  synth::events::EngineEvent engEvent;
  while (engineQueue.pop(engEvent))
    engine.processEngineEvent(engEvent);

  engine.processAudioBlock(outputs, numChannels, numFrames);
}
```

The important behavior is not the exact queue type. It is the sequencing:

- apply pending control changes first
- render audio second

That keeps the engine state coherent for the full block.

## Buffer And Channel Expectations

[`Engine::processAudioBlock`](../src/synth/Engine.h) renders into host-provided buffers shaped as `float**`.

Current output behavior is:

- `0` channels: render work still happens internally, but nothing is written out
- `1` channel: the engine writes a mono downmix
- `2` or more channels: the engine writes left and right into channels `0` and `1`

The current code does not implement a full multichannel layout beyond stereo writes.

Hosts should also note that the engine internally uses its own scratch stereo buffer and its own internal processing block size. The host does not need to match `ENGINE_BLOCK_SIZE`.

## Realtime Considerations

The safest integration approach is:

- collect UI and MIDI changes outside the audio callback
- queue them
- apply them on the render thread immediately before `processAudioBlock`

That matters because the engine is designed around centralized mutation through:

- event processing
- parameter routing
- dirty-sync updates

By keeping those state changes on the render side, the host avoids race conditions around live engine state.

The ring buffers in [`EventQueues.h`](../src/synth/events/EventQueues.h) are simple and intentionally cheap. They use power-of-two fixed sizes and wrap with a bitmask rather than modulo arithmetic.

## Integration Boundaries

The host should not:

- mutate deep engine internals directly from arbitrary threads
- bypass `processParamEvent` for normal parameter automation
- treat routing edits as scalar params
- assume the engine owns device timing or transport

The host should:

- own device and thread orchestration
- feed ordered events into the engine
- treat `Engine` as the control and render boundary

That keeps the integration simple and preserves the separation this repo was extracted to provide.
