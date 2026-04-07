---
layout: default
title: "Project 1"
---

# Project 1 Proposal: Modular Signal Plotter


## Concept

This project is inspired by the fact that Stepdance itself was developed from ideas rooted in modular synthesis and it is a procedual machine.
Building on this connection, the project proposes a system that extends this relationship: instead of using modular synthesis to generate sound, it uses modular signals to generate motion.
The plotter translates electrical signals from a modular system into movement, treating control voltage as a source of motion energy rather than sound. Inspired by modular patching, the same signal can take on different roles—such as oscillation, modulation, or accumulation—depending on how it is used within the system.
Rather than visualizing sound directly, the project focuses on how signal structures can shape behavior, resulting in drawings that evolve through oscillation, variation, and repetition.

## Core Idea
**One signal, multiple behaviors**

- A single input signal (LFO or CV) is used
- The system reinterprets this signal in different ways
- Each interpretation produces a different drawing style

**Transforming sound properties into motion properties**

“Sound has 4 properties, pitch, loudness, timbre and duration, and silence has one – duration.” For plotters, this is similar on pen down (for sound) and pen up (for silence). When the pen is down, the properties of motion includes:
- The starting point
- The duration
- The direction of movement at particular time of duration
- The speed of the movement at particular time of the duration

Here we focus on the painting process of the plotter, which is more similar to a machine performance. The paper painting result is a side profile of the performance process.

**Procedual motion synthesizer**

Inspired by metasound from unreal engine and the idea of procedural machine, the system is designed to be a procedural motion synthesizer where as the same signal flow through the system, it generates different motion performance. The procedual map is premeditated, but is to be triggered by the user input and thus making a half-realtime performance.

![Metasound photo](https://cdn.shopify.com/s/files/1/1793/8985/files/Modular-synth-wavetable-waveform_600x600.jpg?v=1674725110)https://cdn.shopify.com/s/files/1/1793/8985/files/Modular-synth-wavetable-waveform_600x600.jpg?v=1674725110)

## System Structure

**One signal mapping structure**
<div style="display:flex; flex-direction:column; align-items:center; gap:10px; font-family:sans-serif;">
  <div class="box">Modular CV (LFO)</div>
  <div>↓</div>

  <div class="box">Voltage scaling (safe range)</div>
  <div>↓</div>

  <div class="box">Stepdance (Teensy input A1)</div>
  <div>↓</div>

  <div class="box">Motion mapping (code)</div>
  <div>↓</div>

  <div class="box">Plotter drawing</div>
</div>


**Performance interface**

The premeditated procedual map can be logged into a device like a DJ pad for the performer to trigger. And there will be another papersized interface for user to determine the starting point of the plotter, which process will be the silent part of the performance.

![Pad picture](https://media.sweetwater.com/m/products/image/3e9704f51eHcFbkvkrDthr6gfIHGdEiFHolVO0Cq.png?ha=3e9704f51e6d41d3b0a064ddf1190a2163de224a&quality=82&width=750)

## Design

Explain your design process. What choices did you make and why?

## Implementation (Plan for now)
### **1. Inputs**

- Primary Input: Modular LFO (connected to A1)User control:
- Potentiometer → controls intensity (amplitude)
- Button → switches behavior modes

### **2. Signal Mapping**

The same input signal is reused in different ways depending on mode:


### Mode 1 — Oscillation
<p>Signal controls vertical movement and produces smooth wave-like lines.</p>
<p><strong>y = sin(t) × signal</strong></p>

![oscillation photo](https://cdn.shopify.com/s/files/1/0572/6987/8945/files/oscilliation_480x480.png?v=1655824738)

### Mode 2 — Distortion
<p>Signal introduces irregular variation and produces unstable, noisy lines.</p>
<p><strong>y = sin(t) + signal × noise</strong></p>

![distortion photo](https://sound-au.com/articles/distortion-f212.gif)

### Mode 3 — Iterative Drift
<p>Signal accumulates over time and produces gradually shifting structures.</p>
<p><strong>state = state + signal × small_factor</strong></p>
<p><strong>y = state</strong></p>


### Hardware Setup

Describe your hardware configuration.

![Hardware setup photo](assets/placeholder.jpg)

### Code Overview

Highlight key parts of your code and explain your approach:

```cpp
// Paste and explain relevant code snippets here
```

## Results

Show your project in action. Embed a video of it working:

<iframe width="560" height="315" src="https://www.youtube.com/embed/VIDEO_ID" frameborder="0" allowfullscreen></iframe>

*Replace the iframe above with your actual video URL, or use a local video:*

<!--
<video width="560" controls>
  <source src="assets/demo-video.mp4" type="video/mp4">
</video>
-->

## Reflection

What did you learn? What would you do differently?
