# Docs

## Table of Contents

- [Overview](#overview)
- [Available Docs](#available-docs)

## Overview

This directory holds deeper documentation for the synth engine's behavior and design boundaries. The goal is to explain the engine's mental model, control flow, and extension surface without turning the docs into a line-by-line restatement of the code.

Start with the root [`README.md`](../README.md) for build and embedding basics. Use the docs here when you want subsystem detail.

## Available Docs

- [`architecture.md`](architecture.md)
  Engine structure, render flow, voice lifecycle, and subsystem boundaries.
- [`events-and-params.md`](events-and-params.md)
  How hosts drive the engine, how params are represented, and how dirty-sync works.
- [`host-integration.md`](host-integration.md)
  What a host must provide, how to deliver events, and a concrete render-loop pattern.
- [`modulation.md`](modulation.md)
  How FM, the modulation matrix, and block-rate interpolation fit together.
- [`presets-and-routing.md`](presets-and-routing.md)
  Preset structure, FM/mod routing, signal-chain ordering, and FX-chain ordering.
