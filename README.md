# TetraOP

TetraOP is a wavetable synth with four oscillators, FM, Unison, ring modulation and more.

[]

The goal of this synth is to build a versatile synth similar to Ableton Operator, it combines four wavetable oscillators with phase and ring modulation using predefined layouts or a custom modulation matrix.

It is a well executed synth with decent performance and SIMD across voices, it is released as freeware for the KVR developers challenge because frankly I was not sure what to make of it, it is a bit too crafted for a freebie but at the same time not powerful enough to compete with other commercial synths.

Its built using Juce and makes extensive use of the [Gin library](https://github.com/FigBug/Gin/tree/master) for voicing, utilities and working with wavetables.

## Features

* Wavetable based synthesis
* 4 operators with FM and RM routing
* 10 predefined FM layouts
* FM and RM routing matrix
* 16 Unison voices per operator
* 5 Unison modes
* 8 Phase distortion modes
* 2 Filters with 5 types and 4 modes each
* Drag-and-drop modulation system
* Envelopes, LFOs, Macros and other modulation sources

## Build

```bash
git clone --recurse-submodules https://github.com/tiagolr/tetraop.git

# windows
cmake -G "Visual Studio 18 2026" -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake -B build
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
