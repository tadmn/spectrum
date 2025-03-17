# Spectrum
Free, open-source, liberally-licensed, 2-dimensional audio spectrum analyzer.

### Building
```
mkdir build
cd build
cmake .. -G Ninja
ninja
```
At the end of the build process (for both `Debug` & `Release` builds), the CLAP plugin will be automatically installed to the location:
```
macOS: ~/Library/Audio/Plug-Ins/CLAP/spectrum.clap
```