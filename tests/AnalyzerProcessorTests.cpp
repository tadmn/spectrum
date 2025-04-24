#include "AnalyzerProcessor.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <tb_DspUtilities.h>

// Tests parameter setting behavior including callback triggers
TEST_CASE("AnalyzerProcessor parameter setters", "[analyzer]") {
    AnalyzerProcessor analyzer;
    bool parametersChanged = false;
    bool bandsChanged = false;

    // Set up callbacks to track when parameters and bands are changed
    analyzer.onParametersChanged = [&]() { parametersChanged = true; };
    analyzer.onBandsChanged = [&]() { bandsChanged = true; };

    SECTION("Real-time safe parameters") {
        parametersChanged = false;
        bandsChanged = false;

        // Verify that changing real-time safe parameters triggers only the parameters changed callback
        analyzer.setMinDb(-90.f);
        REQUIRE(analyzer.minDb() == Catch::Approx(-90.f));
        REQUIRE(parametersChanged);
        REQUIRE_FALSE(bandsChanged);

        parametersChanged = false;
        analyzer.setAttackRate(20.f);
        REQUIRE(analyzer.attackRate() == Catch::Approx(20.f));
        REQUIRE(parametersChanged);
        REQUIRE_FALSE(bandsChanged);

        parametersChanged = false;
        analyzer.setReleaseRate(0.75f);
        REQUIRE(analyzer.releaseRate() == Catch::Approx(0.75f));
        REQUIRE(parametersChanged);
        REQUIRE_FALSE(bandsChanged);

        parametersChanged = false;
        analyzer.setFftHopSize(512);
        REQUIRE(analyzer.fftHopSize() == 512);
        REQUIRE(parametersChanged);
        REQUIRE_FALSE(bandsChanged);
    }

    SECTION("Non-real-time parameters") {
        parametersChanged = false;
        bandsChanged = false;

        // Verify that changing non-real-time parameters triggers both callbacks
        analyzer.setTargetNumBands(256);
        REQUIRE(analyzer.targetNumBands() == 256);
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setSampleRate(48'000.0);
        REQUIRE(analyzer.sampleRate() == 48'000.0);
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setFftSize(8'192);
        REQUIRE(analyzer.fftSize() == 8'192);
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setMinFrequency(20.f);
        REQUIRE(analyzer.minFrequency() == 20.f);
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setMaxFrequency(20'000.f);
        REQUIRE(analyzer.maxFrequency() == 20'000.f);
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setLineSmoothingInterpolationSteps(12.f);
        REQUIRE(analyzer.lineSmoothingInterpolationSteps() == 12.f);
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setWindowType(tb::WindowType::Hann);
        REQUIRE(analyzer.windowType() == tb::WindowType::Hann);
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setWeightingDbPerOctave(3.f);
        REQUIRE(analyzer.weightingDbPerOctave() == Catch::Approx(3.f));
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);

        parametersChanged = false;
        bandsChanged = false;
        analyzer.setWeightingCenterFrequency(2'000.f);
        REQUIRE(analyzer.weightingCenterFrequency() == Catch::Approx(2'000.f));
        REQUIRE(parametersChanged);
        REQUIRE(bandsChanged);
    }
}

// Tests audio processing functionality of the analyzer
TEST_CASE("AnalyzerProcessor audio processing", "[analyzer]") {
    AnalyzerProcessor analyzer;

    // Configure analyzer with test parameters
    analyzer.setSampleRate(44'100.0);
    analyzer.setFftSize(1'024);
    analyzer.setFftHopSize(512);
    analyzer.setMinFrequency(20.f);
    analyzer.setMaxFrequency(20'000.f);
    analyzer.setTargetNumBands(128);

    SECTION("Process sine wave at specific frequency") {
        constexpr float testFreq = 1'000.f; // Test with 1kHz sine wave

        analyzer.processAudio(tb::makeSineWave(testFreq, analyzer.sampleRate(), 1, 4'096));

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
            const float freq = analyzer.minFrequency() *
                               std::pow(analyzer.maxFrequency() / analyzer.minFrequency(), point.x);

            // Track the peak (y values are inverted in the spectrumLine)
            const auto peak = 1.f - point.y;
            if (peak > peakMagnitude) {
                peakMagnitude = peak;
                peakFreq = freq;
            }
        }

        // Calculate acceptable tolerance based on FFT resolution
        const float binWidth = static_cast<float>(analyzer.sampleRate()) / analyzer.fftSize();
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

    analyzer.processAudio(tb::makeSineWave(1'000.f, analyzer.sampleRate(), 1, 4'096, 0.5f));

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

    analyzer.setSampleRate(44'100.0);
    analyzer.setFftSize(2'048);
    analyzer.setMinFrequency(20.f);
    analyzer.setMaxFrequency(20'000.f);
    analyzer.setTargetNumBands(64);

    analyzer.processAudio(choc::buffer::ChannelArrayBuffer<float>(1, 4'096));
    analyzer.processAnalyzer(0.01);

    const auto& bands = analyzer.bands();

    // Validate band formation
    REQUIRE_FALSE(bands.empty());
    REQUIRE(bands.size() <= analyzer.targetNumBands());

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
        analyzer.setLineSmoothingInterpolationSteps(1.f);

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
        analyzer.setLineSmoothingInterpolationSteps(8.f);

        analyzer.processAudio(choc::buffer::ChannelArrayBuffer<float>(1, 4'096));
        analyzer.processAnalyzer(0.01);

        const auto& bands = analyzer.bands();
        const auto& line = analyzer.spectrumLine();

        // High smoothing should give us many more points than bands
        REQUIRE(line.size() > bands.size() * 4);
    }
}