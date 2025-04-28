# Spectrum
Free, liberally licensed, 2-dimensional audio spectrum analyzer.

<video
muted 
src="https://github.com/user-attachments/assets/b17704a0-27c9-4b15-a315-4ad742b80c49"
/>

## Features
- Real-time FFT-based spectrum analysis with thread safety in mind
- Logarithmically spaced bands
- Spline interpolation to produce visually pleasing smoothed graphics
- Auto-calibration to ensure consistency
- Customizable frequency weighting
- Parameters can be changed on-the-fly, making it easy to dial in the right settings for your project
- Normalized output that allows easy integration into any 2D graphics library
- Level ballistics controls

Supported architectures:
```
macOS arm64
Windows x86_64
``` 

## Build Options
By default, the CMake configuration generates a `spectrum-analyzer-processor` static library target, which encompasses
the audio & line data processing object. This object is designed for seamless integration into any audio pipeline and
can be used to render the spectrum with your preferred graphics library. See
[AnalyzerProcessor.h](src/analyzer/AnalyzerProcessor.h) for details.

The following options default to `OFF` but can be used to enable further build targets.

`-DBUILD_TESTS=ON` Enables building the executable to run the unit tests for the project

`-DBUILD_PLUGIN=ON` Enables a target that builds a [CLAP](https://github.com/free-audio/clap) plugin. The plugin uses
the [visage](https://github.com/VitalAudio/visage) library to render its graphics. Note that the CLAP plugin has only been tested on Reaper & Bitwig
Studio on the supported architectures.

## Build Requirements
### macOS (arm64)
No special requirements needed, just a cmake build environment.

### Windows (x86_64)
You will need to follow the instructions at https://github.com/tadmn/FastFourier to install the Intel IPP library files.
These library files are used for the Intel IPP FFT.

## What about AU & VST3 audio plugins?
`clap-wrapper` is used in this project, however only the `CLAP` target is set to be built. The `VST3` & `AUV2`
targets are experimental. I would not advise building the `Standalone` target as it doesn't appear to have feedback
protection built in.
