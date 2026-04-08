# Presets And Routing

## Table of Contents

- [Overview](#overview)
- [Preset Philosophy](#preset-philosophy)
- [What A Preset Contains](#what-a-preset-contains)
- [Apply And Capture](#apply-and-capture)
- [FM Routing](#fm-routing)
- [Modulation Matrix](#modulation-matrix)
- [Signal And FX Chains](#signal-and-fx-chains)
- [Preset Storage And I/O](#preset-storage-and-io)

## Overview

The engine has two related concerns that are easy to confuse:

- full-state preset management
- live routing changes inside the synthesis graph

They are connected because presets need to capture more than just scalar parameter values. A preset must also remember structural choices such as FM routes, modulation routes, and processor ordering.

## Preset Philosophy

In this engine, a preset is a complete state snapshot, not a partial patch.

That means a preset is expected to describe:

- scalar parameter values
- routing decisions
- signal-chain ordering
- FX-chain ordering
- metadata for humans

This is a stronger and more useful model than treating presets as loose dictionaries with many omitted fields. The main benefit is predictable initialization: applying a preset should move the engine into a known state rather than layering changes on top of whatever was already there.

## What A Preset Contains

[`Preset`](../src/synth/preset/Preset.h) currently contains:

- metadata
- `paramValues` for the scalar parameter surface
- FM routes per oscillator
- modulation matrix routes
- signal-chain ordering
- FX-chain ordering

The default engine startup path uses an init preset produced by `createInitPreset()`. That means a newly created engine starts from the same control path used for normal preset application.

That is a good invariant: startup state and preset state are not two separate concepts.

## Apply And Capture

Preset application lives in [`PresetApply.cpp`](../src/synth/preset/PresetApply.cpp).

Applying a preset currently does four major things:

1. Writes all scalar parameter values through the parameter router.
2. Rebuilds oscillator FM route sets.
3. Rebuilds modulation routes.
4. Replaces signal-chain and FX-chain ordering, then forces a full dirty sync.

Capturing a preset does the reverse:

1. Reads scalar parameter values back out of the router.
2. Copies FM route state from the oscillators.
3. Copies modulation matrix routes.
4. Copies signal-chain and FX-chain ordering.

The key design point is that presets span both parameter state and graph state. If you only snapshot scalar values, you do not capture the actual patch.

## FM Routing

FM routing is oscillator-specific. Each oscillator stores its own set of FM routes, and those routes are managed as structure rather than as scalar params.

That is why FM routing uses `EngineEvent` instead of `ParamEvent`.

This is the right abstraction because route membership and source selection are not naturally expressed as simple floating-point controls.

## Modulation Matrix

[`ModMatrix`](../src/synth/ModMatrix.h) supports up to `16` routes.

Each route contains:

- source
- destination
- amount

Current source categories include:

- envelopes
- LFOs
- velocity
- noise
- mod wheel
- key tracking

Current destination categories include:

- filter cutoff and resonance
- oscillator pitch
- oscillator mix
- wavetable scan position
- oscillator FM depth
- LFO rate and amplitude

The modulation matrix is part of preset state because routing choices are core to the patch, not transient UI state.

## Signal And FX Chains

The engine distinguishes between:

- per-voice signal-chain ordering
- post-mix FX-chain ordering

Per-voice ordering is handled by [`SignalChain`](../src/synth/SignalChain.h). Post-mix ordering is handled by [`FXChain`](../libs/dsp/include/dsp/fx/FXChain.h).

These are separate because they operate at different points in the render path and have different sonic implications.

Examples:

- Moving `saturator` in the signal chain changes each voice before summing.
- Moving `delay` in the FX chain changes the global post-mix texture after voice summing.

Presets must capture both orderings because processor order is part of the patch identity.

## Preset Storage And I/O

Preset file I/O lives in [`PresetIO.h`](../src/synth/preset/PresetIO.h) and related implementation files.

The current preset layer supports:

- saving presets to disk
- loading by explicit path
- loading by name from user and factory locations
- listing available presets
- ensuring preset directories exist

That file layer should be thought of as an adapter around the engine's preset model, not the preset model itself. The engine-level concept is the `Preset` structure; file I/O is just one way to persist and retrieve it.
