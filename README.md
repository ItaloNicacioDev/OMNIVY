OMNIVY — Professional Multi-Engine Software Synthesizer
=========================================================

Codename: OMNIVY
Product: OMNIVY Synthesizer
Category: Professional Multi-Engine Software Synthesizer
Main format: VST3 x64
Platform: Windows 11 x64
Main technology: Modern C++ + JUCE
License: AGPLv3

Identity: Beyond Synthesis.

## Vision

OMNIVY is a professional virtual synthesizer of multi-engine architecture,
capable of competing functionally with high-level synthesizers.

OMNIVY is not a clone of any existing synthesizer. The goal is to create a
sound synthesis platform that brings together, in a single instrument:

- Multiple synthesis engines.
- Six independent oscillators.
- Simultaneous combination of different synthesis technologies.
- Advanced wavetable.
- Granular synthesis.
- FM and PM synthesis.
- Additive synthesis.
- Virtual analog synthesis.
- Sample playback and manipulation.
- Advanced modulation.
- Visual modulation matrix.
- Advanced filters.
- FX rack.
- Professional arpeggiator.
- Musical sequencer.
- Notes and chords composer.
- Interactive virtual keyboard.
- 2D and 3D waveform visualization.
- Own preset system.
- Premium and customizable visual interface.

The product should be conceived as a real musical instrument, usable in
professional production, and not as a technology demonstration.

## Philosophy

OMNIVY prioritizes:

1. Sound quality.
2. Synthesis flexibility.
3. Freedom of routing.
4. Real-time performance.
5. Musical usability.
6. High-quality visual interface.
7. Compatibility with projects and presets.
8. Stability in professional DAWs.

## Development status

Phase 1 — Environment preparation and first functional build:
- CMake configured.
- JUCE 9.0.5 available.
- MSVC x64 toolchain (Visual Studio Build Tools 2026).
- VST3 plugin compiling and producing audio.
- Standalone application compiling.

## License

OMNIVY is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OMNIVY is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with OMNIVY. If not, see <https://www.gnu.org/licenses/>.

## Dependencies

- JUCE 9.0.5 (https://github.com/juce-framework/JUCE)
  - Dual-licensed: AGPLv3 / Commercial.
  - Used under AGPLv3 terms in this project.
- CMake 3.30.5
- Ninja (optional)
- MSVC 14.51.36256 (Visual Studio Build Tools 2026)
- Windows SDK 10.0.26100.0

## Building

### Prerequisites

- Visual Studio Build Tools 2026 (MSVC x64)
- Windows SDK 10
- CMake 3.30.5+
- Git

### Build commands

```cmd
call E:\VS_TOOLS\VC\Auxiliary\Build\vcvars64.bat
cmake -S . -B Builds -G "Visual Studio 17 2022" -A x64
cmake --build Builds --config Release
```

### Output

- VST3 plugin: Builds/Release/Omnivy.vst3
- Standalone: Builds/Release/Omnivy.exe

## Documentation

- docs/architecture/OVERVIEW.md — System architecture
- docs/architecture/AUDIO_ENGINE.md — Audio engine architecture
- docs/architecture/MODULATION.md — Modulation system
- docs/development/BUILD.md — Build instructions
- docs/development/ENVIRONMENT.md — Environment and paths
- docs/licensing/DEPENDENCIES.md — Dependencies and licenses
