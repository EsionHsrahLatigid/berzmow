# Berzmow

Berzmow is a feedback-driven noise distortion effect built with JUCE. It blends the incoming stereo signal with generated noise, drives it through saturation, filtering, resonant tone shaping, feedback, and an optional output limiter.

Use conservative output gain before enabling `Danger` or bypassing the limiter.

## Features

- Stereo audio input and output
- Drive, feedback, tone, resonance, noise mix, and output controls
- Danger mode for higher drive, resonance, and feedback ceilings
- Optional limiter bypass with automatic output trim
- Non-finite sample guards and feedback reset on bad state detection
- APVTS parameter/state persistence
- Repository-local VST3, AU, and Standalone artifact staging

## Build

Requirements:

- CMake 3.22+
- A C++17 compiler
- Ninja, Xcode, or another CMake generator
- Network access for the initial JUCE fetch, or a JUCE checkout at `JUCE/`

JUCE is pinned to 8.0.15 when downloaded by CMake.

```sh
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --target Berzmow_Artifacts
ctest --test-dir build/release --output-on-failure
```

Final products are staged under:

```text
artifacts/Release/
├── AU/Berzmow.component          # macOS only
├── Standalone/Berzmow.app        # macOS
└── VST3/Berzmow.vst3
```

Override the staging root with `-DBERZMOW_ARTIFACT_DIR=/absolute/path`.

## Plug-in identity

- Product: `Berzmow`
- Vendor/company: `EsionHsrahLatigid`
- JUCE manufacturer code: `EHL_`
- JUCE plug-in code: `berz`
- Bundle identifier: `jp.ehl.berzmow`
- Required format: VST3
- Additional formats: AU on Apple platforms and Standalone

## JUCE licensing

JUCE 8 modules are dual-licensed under AGPLv3 and the commercial JUCE license. Review the [JUCE 8.0.15 license](https://github.com/juce-framework/JUCE/blob/8.0.15/LICENSE.md) and choose a compatible licensing path before distributing plug-in binaries. This repository does not vendor JUCE or grant a JUCE commercial license.
