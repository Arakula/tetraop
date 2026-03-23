# TetraOP

TetraOP is a _utility synthesizer_ designed to be versatile for a wide range of sounds, it combines Wavetables and FM synthesis in an easy to use virtual instrument with a balance between good performance and great sound.

The main reference for this synth is Ableton Operator, a handy synth for electronic drums and other sounds that is only available for the Live DAW, it also borrows from other synthesizers like Serum, Vital and Dune.

Other than allowing for complex FM routings and morphing wavetables it comes with a drag-and-drop modulation system that is easy to use with modulators like Macros, LFOs, Envelopes, Random Generators, MPE etc..

Performance is well optimized with SIMD across polyphonic voices and unison, for an FM synth that reads Wavetables with cubic interpolation and oversampling it performs well even with priority given to sound quality over CPU usage.

## What TetraOP is not

TetraOP is not a do-it-all synthesizer or the ultimate synth or attempts to be so, it does not do sample re-synthesis or granular synthesis or have multiple rendering engines, it does not edit wavetables (at least for now).

## Features

* Wavetable based synthesis
* 4 operators with FM and RM routing
* 10 predefined FM layouts
* Custom FM and RM routing
* 16 Unison voices per operator
* 5 Unison modes
* 8 Phase distortion modes
* 2 Filters with 5 types and 4 modes each
* Drag-and-drop modulation system
* Envelopes, LFOs, Macros and other modulation sources
* MPE and Micro-Tuning (Scala files and MTS)

## Build

```bash
git clone --recurse-submodules https://github.com/tiagolr/tetraop.git

# windows
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake --build build

# linux
sudo apt update
sudo apt-get install libx11-dev libfreetype-dev libfontconfig1-dev libasound2-dev libxrandr-dev libxinerama-dev libxcursor-dev
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake --build ./build --config Release

# macOS
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -S . -B ./build
cmake --build ./build --config Release
```
