#pragma once

#include <choc_SampleBuffers.h>
#include <complex>
#include <farbot/RealtimeObject.hpp>
#include <FastFourier.h>
#include <functional>
#include <tb_FifoBuffer.h>
#include <tb_Interpolation.h>
#include <tb_Windowing.h>
#include <vector>

/**
 * @class AnalyzerProcessor
 * @brief Real-time audio spectrum analyzer that processes audio data and outputs a vector of line
 * points that can be used to render the analyzer in a graphics library of your choice.
 *
 * This class implements an FFT-based audio analyzer that divides the frequency spectrum into
 * logarithmically spaced bands, applies various processing options (windowing, weighting,
 * smoothing, auto-normalization), and provides data for spectrum visualization with ballistics
 * (attack/release).
 *
 * @note Currently only a channel count of 1 is supported.
 *
 * Example:
 *
 * (main thread)
 * AnalyzerProcessor analyzer;
 *
 * // Usually called on the audio pipeline's `prepare` callback
 * analyzer.setSampleRate(sampleRate);
 * analyzer.reset();
 *
 * (audio thread rendering callback)
 * analyzer.processAudio(bufferPointers, numChannels, numSamplesPerChannel);
 *
 * (main thread, called every draw callback)
 * analyzer.processAnalyzer(timeInSecondsSinceLastDrawCallback);
 * const auto& line = analyzer.spectrumLine();
 * canvas.startLine();
 * for (auto point : line)
 *     canvas.drawLineTo(point.x * canvas.width(), point.y * canvas.height()));
 */
class AnalyzerProcessor {
  public:
    /**
     * @struct Band
     * @brief Represents a frequency band with associated FFT bins and amplitude. Energy of
     * associated bins are averaged into a dB magnitude value
     */
    struct Band {
        std::vector<int> bins = {};  ///< FFT bin indices that belong to this band
        float dB = -100.f;  ///< Current amplitude of the band in dB
    };

    /**
     * @brief Constructs analyzer with default settings and initializes bands.
     */
    AnalyzerProcessor();

    /**
     * @brief Callback triggered when any parameter changes. This will get called when any of the
     * `set` methods below are called.
     */
    std::function<void()> onParametersChanged;

    /**
     * @brief Callback triggered when band configuration changes. Some parameters are considered
     * "non-real-time"
     */
    std::function<void()> onBandsChanged;

    /**
     * @brief Returns the current spectrum line data for visualization.
     *
     * The points will have x values on the range [0, 1], where 0.0 is the minimum frequency set
     * via `setMinFrequency` and 1.0 is the maximum frequency set via `setMaxFrequency`. X
     * frequency values are logarithmically spaced. The y values will be on the range
     * [1, -infinity], where 1.0 represents the minimum dB set via `setMinDb`, the value 0.0
     * represents 0.0 dB FS (full scale), and negative numbers represent values over 0 dB (these
     * values do not get clamped). This range is well suited for 2d graphics, where `1.0 * height`
     * is the bottom of the screen and `0.0 * height` is the top.
     *
     * If line smoothing is enabled (via `setLineSmoothingFactor`), then the line will have more
     * (interpolated) points, and it will be smoother and less jagged.
     *
     * @return Vector of points representing the spectrum (smoothed if enabled).
     */
    const std::vector<tb::Point>& spectrumLine() const;

    /**
     * @brief Access the frequency bands data.
     *
     * This could be useful in case you may want to display extra information, like the peak dB
     * values for each band.
     *
     * @return Vector of Band objects. Note that this can be less than the target number of bands
     * set via `setTargetNumBands`.
     */
    const std::vector<Band>& bands() const noexcept { return mBands; }

    /**
     * @brief Sets the target number of frequency bands.
     *
     * The actual number of bands will most likely be less. The processor will split the
     * spectrum into logarithmically spaced bands and assign the FFT bins into each band. Bands
     * that don't have any bins assigned to them (in the lower end of the spectrum) will be
     * discarded. Furthermore, if a band has only one bin assigned to it, it will use the
     * frequency (x-axis) value of the bin and not the band. This improves accuracy in the low
     * end and reduces CPU load.
     *
     * To get the actual number of bands, use `bands().size()`
     *
     * @param targetNumberOfBands Desired number of bands (min: 1).
     */
    void setTargetNumBands(int targetNumberOfBands);

    /**
     * @brief Gets the target number of frequency bands.
     * @return Current target number of bands.
     */
    int targetNumBands() const noexcept { return mTargetNumBands; }

    /**
     * @brief Sets the sample rate for analysis.
     * @param sampleRate Sample rate in Hz.
     */
    void setSampleRate(double sampleRate);

    /**
     * @brief Gets the current sample rate.
     * @return Sample rate in Hz.
     */
    double sampleRate() const noexcept { return mSampleRate; }

    /**
     * @brief Sets the FFT size (will be clamped to nearest power of 2).
     * @param fftSize FFT size in samples.
     */
    void setFftSize(int fftSize);

    /**
     * @brief Gets the current FFT size.
     * @return FFT size in samples.
     */
    int fftSize() const noexcept { return mFftSize; }

    /**
     * @brief Sets the minimum frequency to analyze.
     * @param minFreq Minimum frequency in Hz.
     */
    void setMinFrequency(float minFreq);

    /**
     * @brief Gets the minimum analyzed frequency.
     * @return Minimum frequency in Hz.
     */
    float minFrequency() const noexcept { return mMinFrequency; }

    /**
     * @brief Sets the maximum frequency to analyze.
     * @param maxFreq Maximum frequency in Hz.
     */
    void setMaxFrequency(float maxFreq);

    /**
     * @brief Gets the maximum analyzed frequency.
     * @return Maximum frequency in Hz.
     */
    float maxFrequency() const noexcept { return mMaxFrequency; }

    /**
     * @brief Sets the minimum dB FS level (noise floor).
     * @param minDb Minimum dB value.
     */
    void setMinDb(float minDb);

    /**
     * @brief Gets the minimum dB level.
     * @return Minimum dB value.
     */
    float minDb() const noexcept { return mMinDb.load(std::memory_order_relaxed); }

    /**
     * @brief Sets the frequency weighting slope in dB per octave.
     * @param dbPerOctave Weighting slope in dB/octave.
     */
    void setWeightingDbPerOctave(float dbPerOctave);

    /**
     * @brief Gets the frequency weighting slope.
     * @return Weighting in dB/octave.
     */
    float weightingDbPerOctave() const noexcept;

    /**
     * @brief Sets the center frequency for the weighting curve and for auto-normalization.
     *
     * This frequency will be used to auto-normalize the analyzer. When "non-real-time"
     * parameter changes are made, such as the FFT size, the analyzer will auto-normalize the
     * output to this value, meaning that this a sine wave at 0 dB FS at this frequency should
     * as much as possible correspond with 0 dB FS on the output line. This compensates for
     * differences that can affect magnitude output, such as FFT algorithm and windowing type.
     *
     * @param centerFrequency Center frequency in Hz.
     */
    void setWeightingCenterFrequency(float centerFrequency);

    /**
     * @brief Gets the weighting curve center frequency.
     * @return Center frequency in Hz.
     */
    float weightingCenterFrequency() const noexcept;

    /**
     * @brief Sets the line smoothing factor for visualization, reducing jaggedness.
     *
     * The factor will be multiplied against the number of bands to obtain the size of the output
     * line. The band points will then be put through a spline interpolator to fit the size of that
     * output line.
     *
     * @param factor Smoothing factor.
     */
    void setLineSmoothingFactor(float factor);

    /**
     * @brief Gets the line smoothing factor.
     * @return Current smoothing factor.
     */
    float lineSmoothingFactor() const noexcept { return mLineSmoothingFactor; }

    /**
     * @brief Sets the FFT window type.
     * @param windowType Window function to use.
     */
    void setWindowType(tb::WindowType windowType);

    /**
     * @brief Gets the current window type.
     * @return Window type.
     */
    tb::WindowType windowType() const noexcept { return mWindowType; }

    /**
     * @brief Sets the FFT hop size. This is the distance between each FFT.
     *
     * @param hopSize Hop size in samples.
     */
    void setFftHopSize(int hopSize);

    /**
     * @brief Gets the current FFT hop size.
     * @return Hop size in samples.
     */
    int fftHopSize() const noexcept { return mFftHopSize.load(std::memory_order_relaxed); }

    /**
     * @brief Sets the attack rate for level ballistics.
     * @param attackRate Attack rate (higher values = faster attack).
     */
    void setAttackRate(float attackRate);

    /**
     * @brief Gets the attack rate.
     * @return Current attack rate.
     */
    float attackRate() const noexcept { return mAttack.load(std::memory_order_relaxed); }

    /**
     * @brief Sets the release rate for level ballistics.
     * @param releaseRate Release rate (higher values = faster release).
     */
    void setReleaseRate(float releaseRate);

    /**
     * @brief Gets the release rate.
     * @return Current release rate.
     */
    float releaseRate() const noexcept { return mRelease.load(std::memory_order_relaxed); }

    /**
     * @brief Processes incoming audio data for analysis.
     *
     * Call this on the real-time audio thread. This call is real-time safe unless a
     * "non-real-time" parameter is changed. See the implementation comments for details.
     *
     * @param audio Audio buffer to analyze.
     *
     * @note Currently only a channel count of 1 is supported.
     */
    void processAudio(choc::buffer::ChannelArrayView<float> audio);

    /**
     * @brief Processes incoming audio data for analysis using raw buffer pointers.
     *
     * This is an alternative version of processAudio that accepts raw float buffer pointers
     * instead of choc::buffer::ChannelArrayView. It's useful when integrating with APIs
     * or frameworks that provide audio data in this format.
     *
     * @param audioBuffers Array of pointers to audio channel data. Each pointer points to
     *                    an array of float samples for a single channel.
     * @param numChannels Number of audio channels in the input (size of audioBuffers array).
     * @param numFrames Number of audio frames (samples per channel) in each buffer.
     *
     * @note Currently only a channel count of 1 is supported.
     */
    void processAudio(float** audioBuffers, int numChannels, int numFrames);

    /**
     * @brief Updates spectrum analysis with time-based parameters.
     *
     * Call this on your graphics drawing callback. The analyzer will pull the latest FFT
     * data from the audio thread and process the band magnitudes, ballistics, smoothing, etc.
     *
     * @param deltaTimeSeconds Time elapsed since last `processAnalyzer` call in seconds.
     */
    void processAnalyzer(double deltaTimeSeconds);

    /**
     * @brief Resets the analyzer state. Band dB values will get reset to the minimum dB value.
     */
    void reset();

  private:
    void updateBands();
    void parameterChanged() const;
    int clampFftHopSize(int inHopSize) const;

    using RealtimeObject = farbot::RealtimeObject<std::vector<std::complex<float>>, farbot::RealtimeObjectOptions::realtimeMutatable>;

    // "Non-realtime" parameters (i.e. they require a more hefty internal update, with buffers & the
    // band vector being resized, etc.)
    double mSampleRate = 44'100.0;
    int mFftSize = 4'096.0;
    float mMinFrequency = 15.f;
    float mMaxFrequency = 30'000.f;
    int mTargetNumBands = 320;
    float mWeightingDbPerOctave = 6.f;
    float mWeightingCenterFrequency = 1'000.f;
    float mLineSmoothingFactor = 8.f;
    tb::WindowType mWindowType = tb::WindowType::BlackmanHarris;

    // "Realtime" parameters. Usually just a lightweight atomic `store`
    std::atomic<int> mFftHopSize = 1024;
    std::atomic<float> mAttack = 15.f;
    std::atomic<float> mRelease = 0.85f;
    std::atomic<float> mMinDb = -100.f;

    std::vector<float> mWindow;
    std::unique_ptr<tb::FifoBuffer<float>> mFifoBuffer;
    choc::buffer::ChannelArrayBuffer<float> mFftInBuffer;
    std::unique_ptr<FastFourier> mFft;
    std::unique_ptr<RealtimeObject> mFftComplexOutput;

    std::mutex mMutex;

    std::vector<float> mBinWeights;
    std::vector<Band> mBands;

    std::vector<tb::Point> mBandsLine;
    std::vector<tb::Point> mSmoothedLine;

  public:
    // Prevent copying & moving
    AnalyzerProcessor(const AnalyzerProcessor&) = delete;
    AnalyzerProcessor& operator=(const AnalyzerProcessor&) = delete;
};