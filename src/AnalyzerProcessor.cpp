
#include "AnalyzerProcessor.h"
#include "LiveValue.h"

#include <tb_Math.h>

#include <numeric>

AnalyzerProcessor::AnalyzerProcessor() {
    update();
}

void AnalyzerProcessor::setSampleRate(double sampleRate) {
    const std::scoped_lock lock(mMutex);
    mSampleRate = sampleRate;
    update();
}

void AnalyzerProcessor::setFftSize(int fftSize) {
    const std::scoped_lock lock(mMutex);
    mFftSize = fftSize;
    update();
}

void AnalyzerProcessor::process(choc::buffer::ChannelArrayView<float> audio) {
    // Is realtime safe as long as no changes are made to the analyzer. In the case that
    // changes are made, this has a small potential to briefly block while the OS notifies
    // the main thread on the unlock call to the mutex
    const std::unique_lock lock(mMutex, std::try_to_lock);
    if (! lock.owns_lock())
        return;

    while (audio.getNumFrames() > 0) {
        audio = mFifoBuffer->push(audio);
        if (mFifoBuffer->isFull()) {
            // Make a copy of the accumulated samples, so that when we window them we don't affect the overlapping
            // samples in the next chunk
            choc::buffer::copy(mFftInBuffer, mFifoBuffer->getBuffer());

            // Apply windowing
            choc::buffer::applyGainPerFrame(mFftInBuffer, [this](auto i) { return mWindow[i]; });

            {
                RealtimeObject::ScopedAccess<farbot::ThreadType::realtime> fftOut(*mFftComplexOutput);
                mFft->forward(mFftInBuffer.getIterator(0).sample, fftOut->data());
            }

            mFifoBuffer->pop(mFftHopSize);
        }
    }
}

void AnalyzerProcessor::process(double deltaTimeSeconds) {
    std::vector<std::complex<float>> fftOutput;

    {
        RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> f(*mFftComplexOutput);
        fftOutput = *f;
    }

    const auto kAttackRate = std::clamp(mAttack * deltaTimeSeconds, 0.0, 1.0);
    const auto kReleaseRate = std::clamp(mRelease * deltaTimeSeconds, 0.0, 1.0);

    const auto deltaFreq = mSampleRate / mFftSize;

    for (auto& band : mBands) {
        double bandEnergy = 0.0;
        for (auto bin : band.bins) {
            const auto freq = bin * deltaFreq;
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
    mFifoBuffer->clear();

    for (auto& band : mBands)
        band.dB = mMinDb;
}

void AnalyzerProcessor::update() {
    // Hann window
    mWindow.resize(mFftSize);
    for (size_t i = 0; i < mWindow.size(); ++i)
        mWindow[i] = 0.5 - std::cos((2.0 * i * M_PI) / (mWindow.size() - 1)) / 2;

    mFifoBuffer = std::make_unique<FifoBuffer<float>>(1, mFftSize);
    mFftInBuffer.resize({ .numChannels = 1, .numFrames = static_cast<uint>(mFftSize) });
    mFft = std::make_unique<FastFourier>(mFftSize);
    mFftComplexOutput = std::make_unique<RealtimeObject>(std::vector<std::complex<float>>(mFftSize / 2 + 1));

    const auto logMinFreq = std::log10(mMinFrequency);
    const auto logMaxFreq = std::log10(mMaxFrequency);
    const auto logDisplayStep = (logMaxFreq - logMinFreq) / mNumBands;
    const auto numBins = mFftSize / 2 + 1;

    mBands.clear();
    mBands.resize(mNumBands);

    const auto deltaFreq = mSampleRate / mFftSize;

    // Collect all bin indices into their respective bands
    for (int i = 0; i < numBins; ++i) {
        const auto freq = i * deltaFreq;
        if (freq < mMinFrequency || freq > mMaxFrequency)
            continue;

        auto bandIndex = static_cast<int>((std::log10(freq) - logMinFreq) / logDisplayStep);
        assert(bandIndex >= 0 && bandIndex < mNumBands);
        bandIndex = std::clamp(bandIndex, 0, mNumBands - 1);

        mBands[bandIndex].bins.push_back(i);
    }

    // Now update the x positions for each band. Ignore bands that don't have any bins assigned
    // to them since we'll just throw those away
    for (int i = 0; i < mBands.size(); ++i) {
        const auto numBinsInBand = std::accumulate(mBands[i].bins.begin(), mBands[i].bins.end(), 0);
        if (numBinsInBand == 1) {
            // Just use the actual frequency position of the single bin
            const auto freq = mBands[i].bins[0] * deltaFreq;
            mBands[i].x0to1 = tb::to0to1(std::log10(freq), logMinFreq, logMaxFreq);
        } else if (numBinsInBand > 1) {
            // Use the center position of the band
            const auto w = (1.0 / mNumBands);
            const auto x0to1 = i * w + w * 0.5;
            mBands[i].x0to1 = x0to1;
        }
    }

    // Now remove all the bands that don't have any bins assigned to them
    mBands.erase(std::remove_if(mBands.begin(), mBands.end(),
                                [](const Band& band) { return band.bins.empty(); }),
                 mBands.end());

    reset();

    if (onSettingsChanged)
        onSettingsChanged();
}