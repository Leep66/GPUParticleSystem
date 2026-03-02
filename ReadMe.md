# GPU Particle System

A real-time particle simulation implemented entirely on the GPU using DirectX 11 and HLSL compute shaders.

This project explores how to move large-scale simulation from CPU to GPU and design a scalable rendering pipeline capable of handling millions of particles.

---
## Overview

This particle system simulates and renders large numbers of particles fully on the GPU.  
All particle updates, spawning, and lifetime management are executed in compute shaders, while the CPU only issues indirect draw calls.

The system is designed to support extremely large particle counts while maintaining interactive framerate.

---

## Features

- Real-time simulation of **10+ million particles**
- Fully GPU-driven update (no CPU per-particle processing)
- Compute shader particle lifecycle management
- Indirect instanced rendering
- Camera-facing billboards
- Dynamic spawning and recycling
- Multiple force fields (gravity, directional, attraction)
- Sub-emitter behavior on particle death

---

## Simulation

Each particle stores:

- Position
- Velocity
- Lifetime
- Color
- Alive state

Particles follow a lifecycle:

Spawn → Simulate → Fade → Recycle

The simulation includes:
- Gravity force
- External directional forces
- Noise motion
- Attractor points

All updates run in parallel on the GPU.

---

## GPU Architecture

The system is built as a GPU pipeline:

1. **Spawn Pass**
   - New particles created using atomic counters

2. **Simulation Pass**
   - Compute shader updates velocity and position

3. **Cull & Compact**
   - Dead particles recycled

4. **Render Pass**
   - GPU issues indirect draw call
   - No CPU particle loop

The CPU never iterates through particles.

---

## Rendering

- Billboard rendering (camera-facing quads)
- Texture atlas support
- Additive blending
- Depth buffer soft particles
- Per-pixel lighting
- Color over lifetime

---

## Performance

- Supports **10 million+ particles**
- Stable real-time framerate (~60 FPS on modern GPU)
- No CPU bottleneck
- Memory-efficient structured buffers

---

## Technical Highlights

- Compute shaders (HLSL)
- Structured buffers
- Atomic counters
- Indirect draw calls
- Parallel simulation
- Data-oriented design

---

## Tech Stack

- C++
- DirectX 11
- HLSL
- GPU Compute Shader
- Real-time rendering

---

## What I Implemented

This project was implemented entirely by me, including:

- GPU simulation pipeline
- Particle spawning & recycling
- Rendering system
- Lighting & blending
- Performance optimization
- Debug visualization

---

## Author

**Leep**  
Game Programmer
