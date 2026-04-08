# Modulation

## Table of Contents

- [Overview](#overview)
- [Modulation Layers](#modulation-layers)
- [Sources And Destinations](#sources-and-destinations)
- [Block-Rate Modulation Model](#block-rate-modulation-model)
- [FM Versus Mod Matrix](#fm-versus-mod-matrix)
- [Phase And Voice-Level Modulation Details](#phase-and-voice-level-modulation-details)
- [Practical Guidance](#practical-guidance)

## Overview

The engine has more than one kind of modulation. That distinction matters because not all modulation in the synth is represented the same way or processed at the same stage.

At a high level there are three layers:

- oscillator FM routing
- general modulation-matrix routing
- parameter-derived behavior such as tempo-synced LFO timing

Treating them as separate systems keeps the control model easier to reason about and avoids forcing every dynamic behavior into one oversized abstraction.

## Modulation Layers

### FM routing

FM routing is oscillator-specific and lives in [`src/synth/WavetableOsc.h`](../src/synth/WavetableOsc.h).

Each oscillator stores:

- a set of FM routes
- a per-oscillator FM depth modifier
- per-voice FM state and feedback state

This is phase modulation in the oscillator layer, not a generic control-rate matrix feature.

### Modulation matrix

The general modulation matrix lives in [`src/synth/ModMatrix.h`](../src/synth/ModMatrix.h).

It carries up to `16` routes. Each route is:

- source
- destination
- amount

This is the engine's main patchable control-routing system for envelopes, LFOs, velocity, noise, mod wheel, and key tracking.

### Derived modulation behavior

Some behavior is not represented as an explicit route but still affects modulation outcome. Examples include:

- LFO tempo sync
- LFO fade-in timing
- modulation amount clamping by destination type
- dirty-sync recomputation when timing-related parameters change

These are part of the modulation architecture even though they are not route objects.

## Sources And Destinations

Current modulation sources are defined in [`src/synth/ModMatrix.h`](../src/synth/ModMatrix.h) and include:

- `AmpEnv`
- `FilterEnv`
- `ModEnv`
- `LFO1`
- `LFO2`
- `LFO3`
- `Velocity`
- `Noise`
- `ModWheel`
- `KeyTrack`

Current destinations include:

- `SVFCutoff`
- `LadderCutoff`
- `SVFResonance`
- `LadderResonance`
- oscillator pitch for all 4 oscillators
- oscillator mix level for all 4 oscillators
- oscillator scan position for all 4 oscillators
- oscillator FM depth for all 4 oscillators
- LFO rate and amplitude for all 3 LFOs

This destination list is intentionally patch-oriented rather than exhaustive. It exposes the most musically useful control points without turning the matrix into a raw field-access layer.

## Block-Rate Modulation Model

The engine does not recompute every routed contribution as a fresh full solve on every single sample. Instead, modulation is structured around the internal engine block size.

This is visible in:

- [`src/synth/Types.h`](../src/synth/Types.h), where `ENGINE_BLOCK_SIZE` is `64`
- [`src/synth/ModMatrix.h`](../src/synth/ModMatrix.h), where the matrix stores current destination values, previous destination values, and per-block step values

The key idea is:

1. Compute destination values at engine-block rate.
2. Store the previous block's values.
3. Interpolate across the block while rendering samples.

That gives the engine a useful compromise:

- cheaper than fully rebuilding routed state every sample
- smoother than hard-stepping once per host buffer

This is one of the most important design decisions in the engine because it shapes both CPU behavior and modulation feel.

## FM Versus Mod Matrix

FM and the general modulation matrix are both modulation systems, but they operate at different levels.

FM routing:

- changes oscillator phase behavior
- is oscillator-local
- is represented as explicit route membership on each oscillator
- is configured through `EngineEvent`

Mod matrix routing:

- changes higher-level control destinations
- spans multiple subsystems
- is represented as generic source/destination/amount routes
- is also configured through `EngineEvent`

The split is correct because FM routing is not just another scalar control destination. It changes how an oscillator produces its waveform sample in the first place.

## Phase And Voice-Level Modulation Details

A few details in the current code are worth documenting because they affect patch behavior:

- Oscillators support multiple phase modes:
  `reset`, `free`, `random`, and `spread`
- Per-note initialization resets modulation interpolation state for that voice
- Unison phase setup is tied to oscillator phase mode and random/reset settings
- LFOs support retrigger, delay, attack, and tempo sync

These behaviors mean modulation is not purely global. Some modulation state is voice-local, some is oscillator-local, and some is global to the engine.

That distinction is why the codebase has several modulation-related state containers instead of one monolithic object.

## Practical Guidance

When extending modulation, a few questions help keep the design coherent:

- Is this a scalar destination or a structural route?
- Is this modulation evaluated per voice, per oscillator, or globally?
- Does this need sample-accurate behavior or block-rate interpolation?
- Should this be part of preset state?

If the answer is unclear, the safest bias is usually:

- `ParamEvent` for scalar controls
- `EngineEvent` for route membership or chain edits
- mod matrix for general-purpose routed control
- explicit oscillator-side structures for FM-specific behavior

That preserves the current architecture instead of slowly collapsing everything into one generic but less understandable system.
