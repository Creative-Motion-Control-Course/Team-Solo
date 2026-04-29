---
layout: default
title: "Project 1 Proposal"
---

# Project 1: Modular Signal Plotter


## Concept

This project is inspired by the fact that Stepdance itself was developed from ideas rooted in modular synthesis.
Building on this connection, the project proposes a system that extends this relationship: instead of using modular synthesis to generate sound, it uses modular signals to generate motion.
The plotter translates electrical signals from a modular system into movement, treating control voltage as a source of motion energy rather than sound. Inspired by modular patching, the same signal can take on different roles—such as oscillation, modulation, or accumulation—depending on how it is used within the system.
Rather than visualizing sound directly, the project focuses on how signal structures can shape behavior, resulting in drawings that evolve through oscillation, variation, and repetition

## Core Idea
**One signal, multiple behaviors**

- A single input signal (LFO or CV) is used
- The system reinterprets this signal in different ways
- Each interpretation produces a different drawing style


1. The starting point, end point, length of the path, and the shape of the path. 
2. The starting point, the duration, the direction of movement at particular time of duration, the speed of the movement at particular time of the duration

The difference lies in whether we focus on delivering the final outcome on the paper, which makes a paint stroke a state invariant shape, or focus on the painting process of the plotter, make it more like a machine performance and have the paper painting only as a side profile of the process.

## System Structure
<div style="display:flex; flex-direction:column; align-items:center; gap:10px; font-family:sans-serif;">
  <h3>System Structure</h3>

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

<style>
.box {
  border: 1px solid black;
  padding: 10px 20px;
  border-radius: 8px;
  background: #f5f5f5;
}
</style>


## Implementation
### **1. Inputs**

- Primary Input: Modular LFO (connected to A1)User control:
- Potentiometer → controls intensity (amplitude)
- Button → switches behavior modes

### **2. Signal Mapping**

The same input signal is reused in different ways depending on mode:


### Mode 1 — Oscillation
<p>Signal controls vertical movement and produces smooth wave-like lines.</p>
<p><strong>y = sin(t) × signal</strong></p>

### Mode 2 — Distortion
<p>Signal introduces irregular variation and produces unstable, noisy lines.</p>
<p><strong>y = sin(t) + signal × noise</strong></p>

### Mode 3 — Iterative Drift
<p>Signal accumulates over time and produces gradually shifting structures.</p>
<p><strong>state = state + signal × small_factor</strong></p>
<p><strong>y = state</strong></p>

### 3. Technical Plan

* Use Stepdance for motor control
* Read analog input from A1
* Normalize input (0–1 range)
* Map signal to movement parameters: amplitude, distortion, drift
* Keep implementation simple and stable 


## Hardware Setup
* Modular LFO output → attenuated to safe voltage
* Signal connected to Stepdance A1 input
* Shared ground between systems 



## Expected Output
### 1. Smooth oscillation
# ~~~~~~~~~~~~~~


### 2. Distorted waveform
# ~~--~~~---~~~


### 3. Drifting structure
# ~
#  ~~
#   ~~~



## Design Goals
### Specificity
Designed for oscillatory and signal-driven drawings
Not suitable for precise geometric output

### Variation
Different behaviors through mode switching
Continuous variation from signal changes

### Liveness
Real-time control via modular signal and user input



<iframe width="560" height="315" src="https://www.youtube.com/embed/yiL8TIoEK8A?si=bw2wV2MGOqTHDAz2" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>
