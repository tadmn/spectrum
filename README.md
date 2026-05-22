# Spectrum
Free GPU accelerated cross-platform audio spectrum analyzer.

<video
muted
src=https://github.com/user-attachments/assets/8fab4f9f-5aed-48a2-9bb7-50d1d867c713
/>

## Features
- Real-time FFT-based spectrum analysis with thread safety built in
- Logarithmically spaced bands
- Spline interpolation to produce visually pleasing smoothed graphics
- Auto-calibration to ensure consistency
- Customizable frequency weighting
- Parameters can be changed on-the-fly, making it easy to dial in the right settings for your project
- Level ballistics controls
- Normalized output that allows easy integration into any 2D graphics library

## Download

[![Spectrum CI](https://github.com/tadmn/spectrum/actions/workflows/spectrum.yml/badge.svg)](https://github.com/tadmn/spectrum/actions/workflows/spectrum.yml)

Grab the latest installer for your platform: 

- **macOS (arm64, VST3 / CLAP / AUv2):** [Spectrum-macOS.pkg](https://github.com/tadmn/spectrum/releases/latest/download/Spectrum-macOS.pkg)
- **Windows (x86_64, VST3 / CLAP):** [Spectrum-Windows.exe](https://github.com/tadmn/spectrum/releases/latest/download/Spectrum-Windows.exe)
- **Linux (x86_64, VST3 / CLAP):** [Spectrum-Linux.tar.gz](https://github.com/tadmn/spectrum/releases/latest/download/Spectrum-Linux.tar.gz) (Tested on Ubuntu 24.04)

See the [Releases page](https://github.com/tadmn/spectrum/releases) for release notes and older versions.

> **macOS users:** Since the installer is not notarized by Apple, Gatekeeper will block it on first launch with a "Spectrum-macOS.pkg Not Opened" message — this is expected and not a sign of a problem with the file.
>
> To proceed:
> 1. Click **Done** on the warning dialog.
> 2. Open **System Settings → Privacy & Security**.
> 3. Scroll to the **Security** section — you'll see a message about the blocked installer.
> 4. Click **Open Anyway**, then confirm by clicking **Open Anyway** again in the dialog that appears.
>
> You'll only need to do this once per download.

## Build Requirements
### macOS (arm64)
No special requirements needed, just a cmake build environment.

### Windows (x86_64)
You will need to follow the instructions in the README at https://github.com/tadmn/FastFourier to install the Intel IPP library files. These library files are used for the Intel IPP FFT.

### Linux (x86_64)
You will need to follow the instructions in the README at https://github.com/tadmn/FastFourier to install the Intel IPP library files. These library files are used for the Intel IPP FFT.

You will also need to install the following dependencies:
```
sudo apt-get update
sudo apt install libgl1-mesa-dev libxrandr-dev
```

## Using in Your Own Project
The brains of the analyzer are in [AnalyzerProcessor.h](./source/analyzer/AnalyzerProcessor.h). The easiest way to use this is to copy the class .h/.cpp files to your project and then link with the required dependencies. See `spectrum-analyzer-processor` target in [CMakeLists.txt](./CMakeLists.txt) for the list of dependencies. You can then feed the output into your own graphics library of choice. There is an example in the comments at the top of [AnalyzerProcessor.h](./source/analyzer/AnalyzerProcessor.h) that shows how this can be done.
