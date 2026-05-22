
#include "choc/audio/choc_SampleBufferUtilities.h"
#include <choc/audio/choc_Oscillators.h>
#include <numeric>
#include <tb_Denormals.h>
#include <tb_Math.h>

#include "AnalyzerProcessor.h"

namespace {

// Currently, we only support 1 channel
constexpr int k_num_channels = 1;

}

AnalyzerProcessor::AnalyzerProcessor() {
    updateBands();
}

const std::vector<tb::Point>& AnalyzerProcessor::spectrumLine() const {
    if (! smoothed_line_.empty())
        return smoothed_line_;

    return bands_line_;
}

void AnalyzerProcessor::setNonRealtimeParameters(NonRealtimeParameters p) {
    tb_assert(p.sample_rate > 0.0);
    tb_assert(p.target_num_bands >= 1);
    tb_assert(p.fft_size >= 32 && tb::isPowerOf2(p.fft_size));
    tb_assert(p.min_frequency > 0.0f);
    tb_assert(p.max_frequency > 0.0f);
    tb_assert(p.min_frequency < p.max_frequency);
    tb_assert(p.weighting_center_frequency > 0.0f);
    tb_assert(p.line_interpolation_steps >= 0);

    {
        const std::scoped_lock lock(mutex_);
        non_realtime_params_ = p;
        updateBands();
    }
}

void AnalyzerProcessor::setMinDb(float min_dB) {
    min_dB = std::min(min_dB, max_dB_.load(std::memory_order_relaxed) - 0.01f);
    min_dB_.store(min_dB, std::memory_order_relaxed);
}

void AnalyzerProcessor::setMaxDb(float max_dB) {
    max_dB = std::max(min_dB_.load(std::memory_order_relaxed) + 0.01f, max_dB);
    max_dB_.store(max_dB, std::memory_order_relaxed);
}

void AnalyzerProcessor::processAudio(choc::buffer::ChannelArrayView<float> audio) {
    tb_assert(audio.getNumChannels() == k_num_channels);

    // Is realtime safe as long as no "non-real-time" parameters are changed. In that case, this
    // has a small potential to briefly block while the OS notifies the main thread on the unlock
    // call to the mutex
    const std::unique_lock lock(mutex_, std::try_to_lock);
    if (! lock.owns_lock())
        return; // Try to avoid blocking the audio thread as much as possible

    audio = audio.getEnd(std::min(audio.getNumFrames(), static_cast<uint32_t>(fifo_buffer_->capacity())));
    fifo_buffer_->pop(static_cast<int>(audio.getNumFrames()) - fifo_buffer_->freeSpace());
    fifo_buffer_->push(audio);
    if (fifo_buffer_->isFull()) {
        RealtimeObject::ScopedAccess<farbot::ThreadType::realtime> fftBuffer(*transfer_buffer_);
        choc::buffer::copy(choc::buffer::createMonoView(fftBuffer->data(), fftBuffer->size()),
                           fifo_buffer_->getBuffer().getChannel(0));
    }
}

void AnalyzerProcessor::processAudio(float** audioBuffers, int numChannels, int numFrames) {
    tb_assert(numChannels == k_num_channels);
    processAudio(choc::buffer::createChannelArrayView(audioBuffers, numChannels, numFrames));
}

void AnalyzerProcessor::processAnalyzer(double deltaTimeSeconds) {
    const tb::FlushDenormalsToZero flushDenormals;

    const auto attack = std::clamp(attack_.load(std::memory_order_relaxed) * deltaTimeSeconds, 0.0, 1.0);
    const auto release = std::clamp(release_.load(std::memory_order_relaxed) * deltaTimeSeconds, 0.0, 1.0);

    const auto min_dB = static_cast<double>(min_dB_.load(std::memory_order_relaxed));
    const auto maxDb = static_cast<double>(max_dB_.load(std::memory_order_relaxed));

    // Grab the output of the FFT from the audio thread
    {
        // RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> f(*mFftComplexOutput);
        RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> fftBuffer(*transfer_buffer_);
        auto fft_buffer = choc::buffer::createMonoView(fftBuffer->data(), fftBuffer->size());
        choc::buffer::copyRemappingChannels(fft_in_buffer_, fft_buffer);
    }

    // Apply windowing
    applyGainPerFrame(fft_in_buffer_, [this](auto i) { return window_[i]; });

    // Run FFT
    fft_->forward(fft_in_buffer_.getIterator(0).sample, fft_output_.data());

    for (int i = 0; i < bands_.size(); ++i) {
        auto& band = bands_[i];
        double band_energy = 0.0;
        for (auto bin : band.bins) {
            double mag = std::abs(fft_output_[bin]);

            mag *= bin_weights_[bin]; // Includes dB/octave slope & FFT normalization factors

            const auto bin_energy = mag * mag;

            // Take the max bin to ensure we include the peak
            band_energy = std::max(band_energy, bin_energy);
        }

        // Convert energy to dB
        double dB = min_dB;
        if (band_energy > 0.0)
            dB = std::max(min_dB, 10.0 * std::log10(band_energy));

        // Calculate ballistics
        {
            const double old_dB = band.dB;
            if (dB > old_dB)
                dB = attack * dB + (1.0 - attack) * old_dB;
            else
                dB = release * dB + (1.0 - release) * old_dB;

            band.dB = dB;
        }

        int bands_line_index = i;
        if (! smoothed_line_.empty())
            bands_line_index += 2; // Because we added extra control points onto the smoothed line

        bands_line_[bands_line_index].y = static_cast<float>((dB - min_dB) / (maxDb - min_dB));
    }

    if (! smoothed_line_.empty()) {
        tb::catmullRom::spline(smoothed_line_, bands_line_, nonRealtimeParameters().line_interpolation_steps,
                       tb::catmullRom::Type::Uniform);
    }
}

void AnalyzerProcessor::reset() {
    fifo_buffer_->clear();

    const auto min_dB = min_dB_.load(std::memory_order_relaxed);
    for (auto& band : bands_)
        band.dB = min_dB;
}

void AnalyzerProcessor::updateBands() {
    const tb::FlushDenormalsToZero flushDenormals;

    const auto& p = nonRealtimeParameters();

    const auto num_bins = p.fft_size / 2 + 1;

    window_ = tb::window<float>(p.window_type, p.fft_size);
    fifo_buffer_ = std::make_unique<tb::FifoBuffer<float>>(k_num_channels, p.fft_size);
    fft_in_buffer_.resize({ .numChannels = k_num_channels, .numFrames = static_cast<uint32_t>(p.fft_size) });
    fft_ = std::make_unique<FastFourier>(p.fft_size);
    transfer_buffer_ = std::make_unique<RealtimeObject>(std::vector<float>(p.fft_size));
    fft_output_.resize(num_bins);

    // Calculate the normalization factor. This is based on such variables such as FFT
    // algorithm, samplerate, windowing functions, etc.
    double normalization_factor = 1.0;

    {
        // First, generate a sinusoid at the weighting center frequency
        auto signal = choc::oscillator::createChannelArraySine<float>(
            { .numChannels = 1, .numFrames = static_cast<uint32_t>(p.fft_size) },
            p.weighting_center_frequency, p.sample_rate);
        choc::buffer::applyGainPerFrame(signal, [this](auto i) { return window_[i]; });

        // Run the FFT and then extract the peak magnitude
        std::vector<std::complex<float>> fft_out(num_bins);
        fft_->forward(signal.getIterator(0).sample, fft_out.data());
        double max_mag = 0.0;
        for (auto v : fft_out) {
            const auto mag = std::abs(v);
            if (mag > max_mag)
                max_mag = mag;
        }

        normalization_factor = 1.0 / max_mag;
    }

    const auto delta_freq = p.sample_rate / p.fft_size;

    tb_assert(p.min_frequency > 0.0f && p.min_frequency < p.max_frequency);
    const double visual_min_log = std::log2(p.min_frequency);
    const double visual_max_log = std::log2(p.max_frequency);

    const double log_bin_width = std::log2(delta_freq);
    const double log_band_width = (visual_max_log - visual_min_log) / p.target_num_bands;

    // Here we have a separate set of min/max values. This is because we want to add a band on
    // both the left and right side, out of the visual range, so that when we ultimately draw the
    // line it won't abruptly cut off on the ends.
    const auto min_log = visual_min_log - std::max(log_bin_width, log_band_width);
    const auto max_log = visual_max_log + std::max(log_bin_width, log_band_width);

    bands_.clear();
    bands_.resize(std::ceil((max_log - min_log) / log_band_width));

    bin_weights_.clear();
    bin_weights_.resize(num_bins);

    // Assign all bin indices into their respective bands
    for (int i = 0; i < num_bins; ++i) {
        const auto freq = i * delta_freq;

        // Allow the zero-frequency bin to get through
        const auto log_freq = freq > 0.0 ? std::log2(freq) : min_log;

        if (log_freq < min_log || log_freq > max_log)
            continue;

        const auto band_index = static_cast<int>((log_freq - min_log) / log_band_width);
        tb_assert(band_index >= 0 && band_index < bands_.size());
        bands_[band_index].bins.push_back(i);

        // Calculate dB/octave slope weighting
        const auto octaves = log_freq - std::log2(p.weighting_center_frequency);
        const auto weight = std::pow(10.0, (octaves * p.weighting_db_per_octave) / 20.0);

        // Assign the value to our weights buffer. Make sure to also include the FFT
        // normalization factor we calculated earlier.
        bin_weights_[i] = static_cast<float>(weight * normalization_factor);
        tb_assert(std::isfinite(bin_weights_[i]));
    }

    // Now update the x positions for each band
    constexpr auto invalid_x = std::numeric_limits<float>::max();
    bands_line_.clear();
    bands_line_.resize(bands_.size(), {invalid_x, 1.0});

    for (int i = 0; i < bands_.size(); ++i) {
        const auto num_bins_in_band = bands_[i].bins.size();
        if (num_bins_in_band == 1) {
            // Just use the actual frequency position of the single bin
            const auto bin_freq = bands_[i].bins[0] * delta_freq;
            const auto log_bin_freq = bin_freq > 0.0 ? std::log2(bin_freq) : min_log;
            const auto x_0to1 = (log_bin_freq - visual_min_log) / (visual_max_log - visual_min_log);
            bands_line_[i].x = static_cast<float>(x_0to1);
        } else if (num_bins_in_band > 1) {
            const auto log_center_freq = min_log + i * log_band_width + 0.5 * log_band_width;
            const auto x_0to1 = (log_center_freq - visual_min_log) / (visual_max_log - visual_min_log);
            bands_line_[i].x = static_cast<float>(x_0to1);
        }
    }

    // Now remove all the bands that don't have any bins assigned to them so we don't have gaps
    std::erase_if(bands_, [](const Band& band) { return band.bins.empty(); });
    std::erase_if(bands_line_, [](const tb::Point& point) { return point.x == invalid_x; });

    // If smoothing is enabled, we need to prep the smoothed line and add some control points
    smoothed_line_.clear();
    if (p.line_interpolation_steps > 0) {
        // Here we're adding 2 control points on the front and end of the line. This ensures the
        // spline function has enough control points to work with on the ends and reduces the
        // chances of encountering interpolation artifacts at the ends.
        //
        // The offset for these extra points is derived from the spacing between the real,
        // neighboring points rather than a fixed constant. With low FFT sizes the bands near the
        // low end of the (log-scaled) frequency axis can end up extremely close together in x, so
        // a fixed offset can be larger than, comparable to, or even smaller than the real spacing
        // there. That mismatch causes the Catmull-Rom tangent at the endpoint to become unstable,
        // producing a visible cusp. Scaling the offset from the local spacing keeps the added
        // points consistent with the curvature the spline is already following, and a small
        // minimum guards against a zero/near-zero delta if two points happen to coincide.

        constexpr float min_fudge_factor = 0.0001f;

        const auto first_x = bands_line_.front().x;
        const auto second_x = bands_line_[1].x;
        const auto start_delta = std::max(second_x - first_x, min_fudge_factor);
        bands_line_.insert(bands_line_.begin(), { first_x - start_delta, 0.0f });
        bands_line_.insert(bands_line_.begin(), { first_x - 2.0f * start_delta, 0.0f });

        const auto last_x = bands_line_.back().x;
        const auto second_last_x = bands_line_[bands_line_.size() - 2].x;
        const auto end_delta = std::max(last_x - second_last_x, min_fudge_factor);
        bands_line_.push_back({ last_x + end_delta, 0.0f });
        bands_line_.push_back({ last_x + 2.0f * end_delta, 0.0f });

        // Set the proper line size now so that we avoid re-allocating while running
        smoothed_line_.resize(tb::catmullRom::outLineSize(bands_line_.size(), p.line_interpolation_steps));
    }

    reset();
}