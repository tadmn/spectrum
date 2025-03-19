
#include "AnalyzerProcessor.h"

#include "LiveValue.h"

AnalyzerProcessor::AnalyzerProcessor(int fftSize, int numBands, double sampleRate, double minFrequency,
                                     double maxFrequency, double minDb, double attack, double release) :
    mMinDb(minDb), mAttack(attack), mRelease(release), mDeltaFrequency(sampleRate / fftSize) {
    const auto logMinFreq = std::log10(minFrequency);
    const auto logMaxFreq = std::log10(maxFrequency);
    const auto logDisplayStep = (logMaxFreq - logMinFreq) / numBands;
    const auto numBins = fftSize / 2 + 1;

    mBands.clear();
    mBands.resize(numBands);

    // Collect all bin indices into their respective bands
    for (int i = 0; i < numBins; ++i) {
        const auto freq = i * mDeltaFrequency;
        if (freq < minFrequency || freq > maxFrequency)
            continue;

        auto bandIndex = static_cast<int>((std::log10(freq) - logMinFreq) / logDisplayStep);
        assert(bandIndex >= 0 && bandIndex < numBands);
        bandIndex = std::clamp(bandIndex, 0, numBands - 1);

        mBands[bandIndex].bins.push_back(i);
    }

    // Now update the x positions for each band. Ignore bands that don't have any bins assigned
    // to them since we'll just throw those away
    for (int i = 0; i < mBands.size(); ++i) {
        const auto numBinsInBand = std::accumulate(mBands[i].bins.begin(), mBands[i].bins.end(), 0);
        if (numBinsInBand == 1) {
            // Just use the actual frequency position of the single bin
            const auto freq = mBands[i].bins[0] * mDeltaFrequency;
            mBands[i].x0to1 = (logMinFreq + std::log10(freq)) / (logMaxFreq - logMinFreq);
        } else if (numBinsInBand > 1) {
            // Use the center position of the band
            const auto w = (1.0 / numBands);
            const auto x0to1 = i * w + w * 0.5;
            mBands[i].x0to1 = x0to1;
        }
    }

    // Now remove all the bands that don't have any bins assigned to them
    mBands.erase(std::remove_if(mBands.begin(), mBands.end(),
                                [](const Band& band) { return band.bins.empty(); }),
                 mBands.end());

    reset();
}

void AnalyzerProcessor::process(double deltaTimeSeconds, const FftComplexOutput& fftOutput) {
    const auto kAttackRate = std::clamp(mAttack * deltaTimeSeconds, 0.0, 1.0);
    const auto kReleaseRate = std::clamp(mRelease * deltaTimeSeconds, 0.0, 1.0);

    for (auto& band : mBands) {
        double bandEnergy = 0.0;
        for (auto bin : band.bins) {
            const auto freq = bin * mDeltaFrequency;
            double mag = std::abs(fftOutput[bin]);

            // This compensates for FFT settings (algorithm, window, size, etc..).
            // In the future I'll make a function that auto-calibrates for a given
            // center frequency
            LIVE_VALUE(kMagNorm, 0.00047);
            mag *= kMagNorm;

            // dB/Octave slope weighting
            {
                LIVE_VALUE(kSlopeDb, 6);
                LIVE_VALUE(kCenterFreq, 1'000);
                const auto octaves = std::log(freq / kCenterFreq);
                const auto weight = std::pow(10.0, (octaves * kSlopeDb) / 20.0);
                mag *= weight;
            }

            const auto binEnergy = mag * mag;
            bandEnergy += binEnergy;
        }

        bandEnergy /= band.bins.size();  // Average the energy in the band

        // Convert to dB
        double dB = mMinDb;
        if (bandEnergy > 0.0)
            dB = 10.0 * std::log10(bandEnergy);

        // Calculate ballistics
        {
            const auto oldDb = band.dB;
            if (dB > oldDb)
                dB = kAttackRate * dB + (1.0 - kAttackRate) * oldDb;
            else
                dB = kReleaseRate * dB + (1.0 - kReleaseRate) * oldDb;

            band.dB = dB;
        }

        band.y0to1 = dB / mMinDb;
    }
}

void AnalyzerProcessor::reset() {
    for (auto& band : mBands)
        band.dB = mMinDb;
}
