# Events And Params

## Table of Contents

- [Overview](#overview)
- [Three Input Paths](#three-input-paths)
- [When To Use Each Event Type](#when-to-use-each-event-type)
- [Parameter Surface](#parameter-surface)
- [Parameter Routing](#parameter-routing)
- [Dirty Sync](#dirty-sync)
- [Implications For Hosts](#implications-for-hosts)

## Overview

The engine accepts control data through three separate paths defined in [`src/synth/events/Events.h`](../src/synth/events/Events.h):

- `MIDIEvent`
- `ParamEvent`
- `EngineEvent`

That split is intentional. It keeps the public API explicit about the difference between performance input, scalar automation, and structural graph changes.

## Three Input Paths

### `MIDIEvent`

`MIDIEvent` represents note and channel performance data:

- note on
- note off
- control change
- pitch bend
- other channel-oriented MIDI message types

The engine uses these events to drive note lifecycle, MIDI CC mappings, and pitch bend state.

### `ParamEvent`

`ParamEvent` carries:

- a `ParamID`
- a floating-point value

Use this path for scalar, automatable values such as:

- oscillator mix
- filter cutoff
- envelope times
- LFO rate
- FX parameters
- mono, unison, and portamento toggles

This is the engine's broadest control surface.

### `EngineEvent`

`EngineEvent` is for changes that are not well modeled as a single scalar write. Current examples include:

- FM routing changes
- modulation matrix route changes
- signal-chain ordering
- FX-chain ordering
- preset application
- panic

If a control change affects structure or topology rather than just a number, it probably belongs here.

## When To Use Each Event Type

Use `MIDIEvent` when the source is expressive performance data.

Use `ParamEvent` when the host wants to automate or set a single named parameter.

Use `EngineEvent` when the host wants to rebuild routing or apply a larger structural change in one step.

That distinction is worth preserving because it keeps parameter automation simple while avoiding awkward attempts to squeeze graph changes into fake scalar params.

## Parameter Surface

The full parameter surface is declared in [`src/synth/params/ParamDefs.h`](../src/synth/params/ParamDefs.h). Each parameter definition includes:

- canonical string name
- storage type
- min and max range
- default value
- update group

This gives the project a single source of truth for the control surface.

Examples of covered areas:

- oscillator configuration
- envelopes
- filters
- saturator
- mono and portamento
- unison
- tempo
- global FX

The practical value is consistency. Presets, UI, automation, and internal sync logic can all refer back to the same parameter definitions.

## Parameter Routing

[`ParamRouter`](../src/synth/params/ParamRouter.h) binds each `ParamID` to live engine state. It also stores MIDI CC bindings.

The router exists so the rest of the engine can treat parameter writes as logical control operations rather than hardcoding a long list of direct field assignments at every call site.

From a host perspective, the key point is this:

- you target a stable `ParamID`
- the router knows where that value lives in the current engine state

That reduces API sprawl and makes the parameter surface easier to scale.

## Dirty Sync

A parameter write does not directly imply that every downstream coefficient or derived field is recomputed immediately. Instead, parameters are grouped by update impact.

When a parameter changes, the engine:

1. Writes the value through the router.
2. Marks the parameter's `UpdateGroup` as dirty.
3. Calls [`syncDirtyParams`](../src/synth/params/ParamSync.h) to refresh any derived state that depends on those dirty groups.

This design keeps the engine from doing unnecessary work on every scalar update while still preserving correctness for values that require recomputation.

Examples of dirty-sync work in the current codebase:

- filter coefficient recalculation
- envelope time and curve-table refresh
- unison derived-state updates
- portamento coefficient updates
- tempo-synced LFO and delay recomputation
- effect-specific derived values

## Implications For Hosts

The host should think in terms of logical control messages, not direct field mutation.

A few practical rules follow from that:

- Prefer `ParamEvent` for automatable numeric controls.
- Treat `EngineEvent` as the place for route and chain edits.
- Do not bypass the router and dirty-sync path unless you are deliberately working inside engine internals.
- Expect presets to apply through the same centralized control model rather than through ad hoc initialization code.

That keeps all major control paths aligned and reduces the chance of one path leaving engine state half-updated.
