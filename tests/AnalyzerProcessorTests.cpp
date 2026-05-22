#include "AnalyzerProcessor.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <choc/audio/choc_Oscillators.h>

namespace {

choc::buffer::ChannelArrayBuffer<float> makeSineWave(double frequency, double sampleRate, uint32_t numFrames) {
    return choc::oscillator::createChannelArraySine<float>(
        { .numChannels = 1, .numFrames = numFrames },
        frequency, sampleRate);
}

}

// Tests parameter setting behavior
TEST_CASE("AnalyzerProcessor parameter setters", "[analyzer]") {
    AnalyzerProcessor analyzer;

    SECTION("Real-time safe parameters") {
        analyzer.setMinDb(-90.f);
        REQUIRE(analyzer.minDb() == -90.f);

        analyzer.setAttackRate(20.f);
        REQUIRE(analyzer.attackRate() == 20.f);

        analyzer.setReleaseRate(0.75f);
        REQUIRE(analyzer.releaseRate() == 0.75f);
    }

    SECTION("Non-real-time parameters") {
        {
            auto params = analyzer.nonRealtimeParameters();
            params.target_num_bands = 256;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().target_num_bands == 256);
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.sample_rate = 48'000.0;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().sample_rate == 48'000.0);
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.fft_size = 8'192;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().fft_size == 8'192);
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.min_frequency = 20.f;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().min_frequency == 20.f);
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.max_frequency = 20'000.f;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().max_frequency == 20'000.f);
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.line_interpolation_steps = 12;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().line_interpolation_steps == 12);
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.window_type = tb::WindowType::Hann;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().window_type == tb::WindowType::Hann);
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.weighting_db_per_octave = 3.f;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().weighting_db_per_octave == Catch::Approx(3.f));
        }
        {
            auto params = analyzer.nonRealtimeParameters();
            params.weighting_center_frequency = 2'000.f;
            analyzer.setNonRealtimeParameters(params);
            REQUIRE(analyzer.nonRealtimeParameters().weighting_center_frequency == Catch::Approx(2'000.f));
        }
    }
}

// Tests audio processing functionality of the analyzer
TEST_CASE("AnalyzerProcessor audio processing", "[analyzer]") {
    AnalyzerProcessor analyzer;

    {
        AnalyzerProcessor::NonRealtimeParameters params;
        params.sample_rate = 44'100.0;
        params.fft_size = 1'024;
        params.min_frequency = 20.f;
        params.max_frequency = 20'000.f;
        params.target_num_bands = 128;
        analyzer.setNonRealtimeParameters(params);
    }

    SECTION("Process sine wave at specific frequency") {
        constexpr float testFreq = 1'000.f; // Test with 1kHz sine wave

        const auto& p = analyzer.nonRealtimeParameters();
        analyzer.processAudio(makeSineWave(testFreq, p.sample_rate, 4'096));

        // Run analyzer multiple times to ensure stable line data
        for (int i = 0; i < 10; i++)
            analyzer.processAnalyzer(0.01);

        const auto& bands = analyzer.bands();
        REQUIRE_FALSE(bands.empty());

        const auto& line = analyzer.spectrumLine();
        REQUIRE_FALSE(line.empty());

        // Locate the peak frequency in the spectrum
        float peakMagnitude = -std::numeric_limits<float>::infinity();
        float peakFreq = 0.f;

        for (const auto& point : line) {
            // Convert normalized x position to frequency using logarithmic scale
            const float freq = p.min_frequency *
                               std::pow(p.max_frequency / p.min_frequency, point.x);

            // Track the peak (y values are inverted in the spectrumLine)
            const auto peak = point.y;
            if (peak > peakMagnitude) {
                peakMagnitude = peak;
                peakFreq = freq;
            }
        }

        // Calculate acceptable tolerance based on FFT resolution
        const float binWidth = static_cast<float>(p.sample_rate) / p.fft_size;
        const float tolerance = binWidth * 2.f;

        INFO("Peak frequency: " << peakFreq << ", Expected: " << testFreq);
        INFO("Tolerance: " << tolerance << " Hz (FFT bin width: " << binWidth << " Hz)");

        // Peak should be at the test frequency within tolerance
        REQUIRE(peakFreq == Catch::Approx(testFreq).margin(tolerance));
    }
}

// Tests that the reset function properly clears the analyzer state
TEST_CASE("AnalyzerProcessor reset functionality", "[analyzer]") {
    AnalyzerProcessor analyzer;

    {
        const auto& p = analyzer.nonRealtimeParameters();
        auto sin = makeSineWave(1'000.f, p.sample_rate, 4'096);
        choc::buffer::applyGain(sin, 0.5f);
        analyzer.processAudio(sin);
    }

    // Process analyzer to populate bands
    for (int i = 0; i < 10; i++)
        analyzer.processAnalyzer(0.01);

    // Verify we have some non-minimum values in the bands
    bool hasNonMinimumValues = false;
    for (const auto& band : analyzer.bands()) {
        if (band.dB > analyzer.minDb() + 1.f) {
            hasNonMinimumValues = true;
            break;
        }
    }

    REQUIRE(hasNonMinimumValues);

    // Reset the analyzer and verify all bands are at minimum
    analyzer.reset();

    for (const auto& band : analyzer.bands())
        REQUIRE(band.dB == Catch::Approx(analyzer.minDb()).margin(0.001f));
}

// Tests the frequency band formation and distribution
TEST_CASE("AnalyzerProcessor frequency band distribution", "[analyzer]") {
    AnalyzerProcessor analyzer;

    {
        AnalyzerProcessor::NonRealtimeParameters params;
        params.sample_rate = 44'100.0;
        params.fft_size = 2'048;
        params.min_frequency = 20.f;
        params.max_frequency = 20'000.f;
        params.target_num_bands = 64;
        analyzer.setNonRealtimeParameters(params);
    }

    analyzer.processAudio(choc::buffer::ChannelArrayBuffer<float>(1, 4'096));
    analyzer.processAnalyzer(0.01);

    const auto& bands = analyzer.bands();

    // Validate band formation
    REQUIRE_FALSE(bands.empty());
    REQUIRE(bands.size() <= static_cast<size_t>(analyzer.nonRealtimeParameters().target_num_bands));

    // Verify each band has at least one bin
    for (const auto& band : bands) {
        REQUIRE_FALSE(band.bins.empty());
    }

    // Verify bands are in ascending frequency order
    for (size_t i = 1; i < bands.size(); ++i) {
        REQUIRE(bands[i].bins.front() > bands[i - 1].bins.back());
    }
}

// Tests the line smoothing functionality at different settings
TEST_CASE("AnalyzerProcessor line smoothing", "[analyzer]") {
    AnalyzerProcessor analyzer;

    SECTION("No smoothing") {
        auto params = analyzer.nonRealtimeParameters();
        params.line_interpolation_steps = 0;
        analyzer.setNonRealtimeParameters(params);

        analyzer.processAudio(choc::buffer::ChannelArrayBuffer<float>(1, 4'096));
        analyzer.processAnalyzer(0.01);

        const auto& bands = analyzer.bands();
        const auto& line = analyzer.spectrumLine();

        // The line size should have 2 more values, since we add a point at the beginning and end
        // that are tethered to the bottom left and bottom right.
        REQUIRE(line.size() == bands.size());
    }

    SECTION("High smoothing") {
        // Higher smoothing should produce significantly more points
        auto params = analyzer.nonRealtimeParameters();
        params.line_interpolation_steps = 8;
        analyzer.setNonRealtimeParameters(params);

        analyzer.processAudio(choc::buffer::ChannelArrayBuffer<float>(1, 4'096));
        analyzer.processAnalyzer(0.01);

        const auto& bands = analyzer.bands();
        const auto& line = analyzer.spectrumLine();

        // High smoothing should give us many more points than bands
        REQUIRE(line.size() > bands.size() * 4);
    }
}
