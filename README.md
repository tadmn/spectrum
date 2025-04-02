# Spectrum
Free, open-source, liberally-licensed, 2-dimensional audio spectrum analyzer.

The `Spectrum` plugin used the [visage](https://github.com/VitalAudio/visage) library to render the analyzer.

If you want to use a different graphics library, slot [AnalyzerProcessor](src/AnalyzerProcessor.h) into your audio
pipeline. It will output vectors of X/Y coordinates (on the range [0, 1]) that can then be sent to your graphics
layer. See the source code documentation for examples/tips on how to do this.

### Build Requirements
#### macOS
```
brew install cmake
brew install ninja
```

### Building
```
mkdir build
cd build
cmake .. -G Ninja
ninja
```
At the end of the build process (for both `Debug` & `Release` builds), the CLAP plugin will be automatically installed
to the location:
```
macOS: ~/Library/Audio/Plug-Ins/CLAP/spectrum.clap
```

### What about AU & VST3?
`clap-wrapper` is used in this project, however only the `CLAP` target is set to be built. The `VST3` & `AUV2`
targets are experimental. I would not advise building the `Standalone` target as it doesn't appear to have feedback
protection built in.