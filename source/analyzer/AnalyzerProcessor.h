#pragma once

#include <choc/audio/choc_SampleBuffers.h>
#include <complex>
#include <farbot/RealtimeObject.hpp>
#include <FastFourier.h>
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
 * @note Currently, only a channel count of 1 (mono) is supported.
 *
 * Example:
 *
 * // Main thread
 * AnalyzerProcessor analyzer;
 *
 * // Usually called on the audio pipeline's prepare callback
 * auto params = analyzer.nonRealtimeParameters();
 * params.sample_rate = current_sample_rate;
 * analyzer.setNonRealtimeParameters(params);
 *
 * // Audio thread rendering callback
 * analyzer.processAudio(bufferPointers, numChannels, numSamplesPerChannel);
 *
 * // Main thread, called every draw callback
 * analyzer.processAnalyzer(timeInSecondsSinceLastDrawCallback);
 * const auto& line = analyzer.spectrumLine();
 * canvas.startLine();
 * for (const auto& point : line)
 *     canvas.drawLineTo(point.x * canvas.width(), (1.0f - point.y) * canvas.height());
 */
class AnalyzerProcessor {
  public:
    struct Band {
        std::vector<int> bins = {};  ///< FFT bin indices that belong to this band
        float dB = -100.f;           ///< Current amplitude of the band in dB FS
    };

    AnalyzerProcessor();

    /**
     * @brief Returns the current spectrum line data for visualization.
     *
     * The x values will be normalized to 0.0, 1.0, where 0.0 is the minimum frequency set
     * via setMinFrequency and 1.0 is the maximum frequency set via setMaxFrequency. X
     * frequency values are logarithmically spaced. Note that some points with x values below 0.0
     * and x values above 1.0 will be added. These bands are added so that the line doesn't abruptly
     * "cutoff" at the minimum and maximum frequency.
     *
     * The y values will be normalized to 0.0, 1.0, where 1.0 represents the minimum dB FS set via
     * setMinDb, and the value 0.0 represents the "maximum" dB FS value set via setMaxDb. There will
     * be no y values below 0.0. Values above 1.0 will be presented if the internally calculated dB
     * value is above the maximum dB.
     *
     * If line smoothing is enabled (via setLineSmoothingInterpolationSteps), then the line will
     * have more (interpolated) points, and it will be smoother and less jagged.
     *
     * @return Vector of points representing the spectrum.
     */
    const std::vector<tb::Point>& spectrumLine() const;

    /**
     * @brief Access the frequency bands data.
     *
     * This could be useful in case you may want to display extra information, like the peak dB
     * values for each band.
     *
     * @return Vector of Band objects. Note that this vector may have a different size than the
     * target number of bands set via setTargetNumBands.
     */
    const std::vector<Band>& bands() const noexcept { return bands_; }

    // ---------------------------------------------------------------------------------------------
    // "Non-realtime" parameters
    //
    // Changing these parameters requires a more hefty internal update, with buffers & the band
    // vector being resized. There will be a brief pause / reset in the analyzer display.
    // ---------------------------------------------------------------------------------------------
    struct NonRealtimeParameters {
        double sample_rate               = 44'100.0;
        int fft_size                     = 4'096;
        float min_frequency              = 15.0f;
        float max_frequency              = 30'000.0f;
        int target_num_bands             = 320;
        float weighting_db_per_octave    = 6.0f;
        float weighting_center_frequency = 1'000.0f;
        int line_interpolation_steps     = 4;
        tb::WindowType window_type       = tb::WindowType::BlackmanHarris;
    };

    void setNonRealtimeParameters(NonRealtimeParameters params);
    const NonRealtimeParameters& nonRealtimeParameters() const noexcept { return non_realtime_params_; }

    // ---------------------------------------------------------------------------------------------
    // Realtime parameters
    //
    // Changing these parameters will not cause the analyzer display to pause / reset.
    // ---------------------------------------------------------------------------------------------
    static constexpr float k_default_attack  = 18.0f;
    static constexpr float k_default_release = 0.85f;
    static constexpr float k_default_min_dB  = -75.0f;
    static constexpr float k_default_max_dB  = 5.0f;

    void setMinDb(float min_dB);
    float minDb() const noexcept { return min_dB_.load(std::memory_order_relaxed); }

    void setMaxDb(float max_dB);
    float maxDb() const noexcept { return max_dB_.load(std::memory_order_relaxed); }

    void setAttackRate(float attack_rate) { attack_.store(attack_rate, std::memory_order_relaxed); }
    float attackRate() const noexcept { return attack_.load(std::memory_order_relaxed); }

    void setReleaseRate(float release_rate) { release_.store(release_rate, std::memory_order_relaxed); }
    float releaseRate() const noexcept { return release_.load(std::memory_order_relaxed); }
    // ---------------------------------------------------------------------------------------------

    /**
     * @brief Processes incoming audio data for analysis.
     *
     * Call this on the real-time audio thread. This call is real-time safe unless a
     * "non-real-time" parameter is changed. See the implementation comments for details.
     *
     * @param audio Audio buffer to analyze.
     *
     * @note Currently, only a channel count of 1 is supported.
     */
    void processAudio(choc::buffer::ChannelArrayView<float> audio);

    /**
     * @brief Processes incoming audio data for analysis using raw buffer pointers.
     *
     * This is an alternative version of processAudio that accepts raw float buffer pointers
     * instead of choc::buffer::ChannelArrayView. It's useful when integrating with APIs
     * or frameworks that provide audio data in this format.
     *
     * @param audio_buffers Array of pointers to audio channel data. Each pointer points to
     *                    an array of float samples for a single channel.
     * @param channels Number of audio channels in the input (size of audioBuffers array).
     * @param frames Number of audio frames (samples per channel) in each buffer.
     *
     * @note Currently, only a channel count of 1 is supported.
     */
    void processAudio(float** audio_buffers, int channels, int frames);

    /**
     * @brief Updates spectrum analysis with time-based parameters.
     *
     * Call this on your graphics drawing callback. The analyzer will pull the latest FFT
     * data from the audio thread and process the band magnitudes, ballistics, smoothing, etc.
     *
     * @param delta_time_seconds Time since the last processAnalyzer call in seconds.
     */
    void processAnalyzer(double delta_time_seconds);

    /**
     * @brief Resets the analyzer state. Band dB values will get reset to the minimum dB value.
     */
    void reset();

  private:
    void updateBands();

    using RealtimeObject = farbot::RealtimeObject<std::vector<float>, farbot::RealtimeObjectOptions::realtimeMutatable>;

    NonRealtimeParameters non_realtime_params_;

    // Realtime parameters
    std::atomic<float> attack_  = k_default_attack;
    std::atomic<float> release_ = k_default_release;
    std::atomic<float> min_dB_  = k_default_min_dB;
    std::atomic<float> max_dB_  = k_default_max_dB;

    std::unique_ptr<tb::FifoBuffer<float>> fifo_buffer_;

    std::mutex mutex_;
    std::unique_ptr<RealtimeObject> transfer_buffer_;

    std::vector<float> window_;
    choc::buffer::ChannelArrayBuffer<float> fft_in_buffer_;
    std::unique_ptr<FastFourier> fft_;
    std::vector<std::complex<float>> fft_output_;
    std::vector<float> bin_weights_;
    std::vector<Band> bands_;
    std::vector<tb::Point> bands_line_;
    std::vector<tb::Point> smoothed_line_;

  public:
    // Prevent copying & moving
    AnalyzerProcessor(const AnalyzerProcessor&) = delete;
    AnalyzerProcessor& operator=(const AnalyzerProcessor&) = delete;
};