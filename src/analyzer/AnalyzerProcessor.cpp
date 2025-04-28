
#include "AnalyzerProcessor.h"

#include <numbers>
#include <numeric>
#include <tb_Math.h>
#include <tb_DspUtilities.h>

namespace {

// Currently, we only support 1 channel.
constexpr int kNumChannels = 1;

}

AnalyzerProcessor::AnalyzerProcessor() {
    updateBands();
}

const std::vector<tb::Point>& AnalyzerProcessor::spectrumLine() const {
    if (! mSmoothedLine.empty())
        return mSmoothedLine;

    return mBandsLine;
}

void AnalyzerProcessor::setTargetNumBands(int targetNumberOfBands) {
    targetNumberOfBands = std::max(1, targetNumberOfBands);

    {
        const std::scoped_lock lock(mMutex);
        mTargetNumBands = targetNumberOfBands;
        updateBands();
    }

    parameterChanged();
}

void AnalyzerProcessor::setSampleRate(double sampleRate) {
    tb_assert(sampleRate > 0.0);

    {
        const std::scoped_lock lock(mMutex);
        mSampleRate = sampleRate;
        updateBands();
    }

    parameterChanged();
}

void AnalyzerProcessor::setFftSize(int fftSize) {
    fftSize = std::max(tb::closestPowerOf2(fftSize), 2);

    {
        const std::scoped_lock lock(mMutex);
        mFftSize = fftSize;
        mFftHopSize = clampFftHopSize(mFftHopSize);
        updateBands();
    }

    parameterChanged();
}

void AnalyzerProcessor::setMinFrequency(float minFreq) {
    minFreq = std::max(minFreq, 1.f);

    {
        const std::scoped_lock lock(mMutex);
        mMinFrequency = minFreq;
        updateBands();
    }

    parameterChanged();
}

void AnalyzerProcessor::setMaxFrequency(float maxFreq) {
    maxFreq = std::max(maxFreq, 1.f);

    {
        const std::scoped_lock lock(mMutex);
        mMaxFrequency = maxFreq;
        updateBands();
    }

    parameterChanged();
}

void AnalyzerProcessor::setWeightingDbPerOctave(float dbPerOctave) {
    {
        std::scoped_lock lock(mMutex);
        mWeightingDbPerOctave = dbPerOctave;
        updateBands();
    }

    parameterChanged();
}

float AnalyzerProcessor::weightingDbPerOctave() const noexcept {
    return mWeightingDbPerOctave;
}

void AnalyzerProcessor::setWeightingCenterFrequency(float centerFrequency) {
    if (centerFrequency <= 0.f)
        centerFrequency = mWeightingCenterFrequency; // Ignore negative values

    {
        std::scoped_lock lock(mMutex);
        mWeightingCenterFrequency = centerFrequency;
        updateBands();
    }

    parameterChanged();
}

float AnalyzerProcessor::weightingCenterFrequency() const noexcept {
    return mWeightingCenterFrequency;
}

void AnalyzerProcessor::setLineSmoothingInterpolationSteps(int numInterpolationSteps) {
    numInterpolationSteps = std::max(0, numInterpolationSteps);

    {
        const std::scoped_lock lock(mMutex);
        mLineInterpolationSteps = numInterpolationSteps;
        updateBands();
    }

    parameterChanged();
}

void AnalyzerProcessor::setWindowType(tb::WindowType windowType) {
    {
        const std::scoped_lock lock(mMutex);
        mWindowType = windowType;
        updateBands();
    }

    parameterChanged();
}

void AnalyzerProcessor::setFftHopSize(int hopSize) {
    hopSize = clampFftHopSize(hopSize);
    mFftHopSize.store(hopSize, std::memory_order_relaxed);
    parameterChanged();
}

void AnalyzerProcessor::setAttackRate(float attackRate) {
    mAttack.store(attackRate, std::memory_order_relaxed);
    parameterChanged();
}

void AnalyzerProcessor::setReleaseRate(float releaseRate) {
    mRelease.store(releaseRate, std::memory_order_relaxed);
    parameterChanged();
}

void AnalyzerProcessor::setMinDb(float minDb) {
    mMinDb.store(minDb, std::memory_order_relaxed);
    parameterChanged();
}

void AnalyzerProcessor::processAudio(choc::buffer::ChannelArrayView<float> audio) {
    tb_assert(audio.getNumChannels() == kNumChannels);

    // Is realtime safe as long as no "non-real-time" parameters are changed. In that case, this
    // has a small potential to briefly block while the OS notifies the main thread on the unlock
    // call to the mutex
    const std::unique_lock lock(mMutex, std::try_to_lock);
    if (! lock.owns_lock())
        return; // Try to avoid blocking the audio thread as much as possible

    while (audio.getNumFrames() > 0) {
        audio = mFifoBuffer->push(audio);
        if (mFifoBuffer->isFull()) {
            // Make a copy of the accumulated samples, so that when we window them we don't affect
            // the overlapping samples in the next chunk
            copy(mFftInBuffer, mFifoBuffer->getBuffer());

            // Apply windowing
            applyGainPerFrame(mFftInBuffer, [this](auto i) { return mWindow[i]; });

            // Run the FFT into our realtime object buffer so that the main thread can access it
            {
                RealtimeObject::ScopedAccess<farbot::ThreadType::realtime> fftOut(*mFftComplexOutput);
                mFft->forward(mFftInBuffer.getIterator(0).sample, fftOut->data());
            }

            mFifoBuffer->pop(mFftHopSize.load(std::memory_order_relaxed));
        }
    }
}

void AnalyzerProcessor::processAudio(float** audioBuffers, int numChannels, int numFrames) {
    tb_assert(numChannels == kNumChannels);
    processAudio(choc::buffer::createChannelArrayView(audioBuffers, numChannels, numFrames));
}

void AnalyzerProcessor::processAnalyzer(double deltaTimeSeconds) {
    const auto attack = std::clamp(mAttack.load(std::memory_order_relaxed) * deltaTimeSeconds, 0.0, 1.0);
    const auto release = std::clamp(mRelease.load(std::memory_order_relaxed) * deltaTimeSeconds, 0.0, 1.0);

    const auto minDb = static_cast<double>(mMinDb.load(std::memory_order_relaxed));

    std::vector<std::complex<float>> fftOutput;

    // Grab the output of the FFT from the audio thread
    {
        RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> f(*mFftComplexOutput);
        fftOutput = *f;
    }

    for (int i = 0; i < mBands.size(); ++i) {
        auto& band = mBands[i];
        double bandEnergy = 0.0;
        for (auto bin : band.bins) {
            double mag = std::abs(fftOutput[bin]);

            // Includes dB/octave slope & FFT normalization factors
            mag *= mBinWeights[bin];

            const auto binEnergy = mag * mag;
            bandEnergy += binEnergy;
        }

        bandEnergy /= band.bins.size();  // Average the energy in the band

        // Convert energy to dB
        double dB = minDb;
        if (bandEnergy > 0.0)
            dB = std::max(minDb, 10.0 * std::log10(bandEnergy));

        // Calculate ballistics
        {
            const double oldDb = band.dB;
            if (dB > oldDb)
                dB = attack * dB + (1.0 - attack) * oldDb;
            else
                dB = release * dB + (1.0 - release) * oldDb;

            band.dB = dB;
        }

        int bandsLineIndex = i;
        if (! mSmoothedLine.empty())
            bandsLineIndex += 2; // Because we added extra control points onto the smoothed line

        mBandsLine[bandsLineIndex].y = dB / minDb;
    }

    if (! mSmoothedLine.empty())
        tb::catmullRom::spline(mSmoothedLine, mBandsLine, mLineInterpolationSteps,
                               tb::catmullRom::Type::Uniform);
}

void AnalyzerProcessor::reset() {
    mFifoBuffer->clear();

    const auto minDb = mMinDb.load(std::memory_order_relaxed);
    for (auto& band : mBands)
        band.dB = minDb;
}

void AnalyzerProcessor::updateBands() {
    const auto numBins = mFftSize / 2 + 1;

    mWindow = tb::window<float>(mWindowType, mFftSize);
    mFifoBuffer = std::make_unique<tb::FifoBuffer<float>>(kNumChannels, mFftSize);
    mFftInBuffer.resize({ .numChannels = kNumChannels, .numFrames = static_cast<uint32_t>(mFftSize) });
    mFft = std::make_unique<FastFourier>(mFftSize);
    mFftComplexOutput = std::make_unique<RealtimeObject>(std::vector<std::complex<float>>(numBins));

    // Calculate the normalization factor. This is based on such variables such as FFT
    // algorithm, samplerate, windowing functions, etc.
    double normalizationFactor = 1.0;

    {
        // First generate a sinusoid at the weighting center frequency
        auto signal = tb::makeSineWave(mWeightingCenterFrequency, mSampleRate, 1, mFftSize);
        choc::buffer::applyGainPerFrame(signal, [this](auto i) { return mWindow[i]; });

        // Run the FFT and then extract the peak magnitude
        std::vector<std::complex<float>> fftOut(numBins);
        mFft->forward(signal.getIterator(0).sample, fftOut.data());
        double maxMag = 0.0;
        for (auto v : fftOut) {
            const auto mag = std::abs(v);
            if (mag > maxMag)
                maxMag = mag;
        }

        normalizationFactor = 1.0 / maxMag;
    }

    const auto deltaFreq = mSampleRate / mFftSize;

    const double visualMinLog = std::log10(mMinFrequency);
    const double visualMaxLog = std::log10(mMaxFrequency);

    const double logBinWidth = std::log10(deltaFreq);
    const double logBandWidth = (visualMaxLog - visualMinLog) / mTargetNumBands;

    // Here we have a separate set of min/max values. This is because we want to add a band on
    // both the left and right side, out of the visual range, so that when we ultimately draw the
    // line it won't abruptly cut off on the ends.
    const auto minLog = visualMinLog - std::max(logBinWidth, logBandWidth);
    const auto maxLog = visualMaxLog + std::max(logBinWidth, logBandWidth);

    mBands.clear();
    mBands.resize(std::ceil((maxLog - minLog) / logBandWidth));

    mBinWeights.clear();
    mBinWeights.resize(numBins);

    // Assign all bin indices into their respective bands
    for (int i = 0; i < numBins; ++i) {
        const auto freq = i * deltaFreq;

        // Allow the zero-frequency bin to get through
        const auto logFreq = freq > 0.0 ? std::log10(freq) : minLog;

        if (logFreq < minLog || logFreq > maxLog)
            continue;

        auto bandIndex = static_cast<int>((logFreq - minLog) / logBandWidth);
        tb_assert(bandIndex >= 0 && bandIndex < mBands.size());
        mBands[bandIndex].bins.push_back(i);

        // Calculate dB/octave slope weighting
        const auto octaves = std::log2(freq / mWeightingCenterFrequency);
        const auto weight = std::pow(10.0, (octaves * mWeightingDbPerOctave) / 20.0);

        // Assign the value to our weights buffer. Make sure to also include the FFT
        // normalization factor we calculated earlier.
        mBinWeights[i] = weight * normalizationFactor;
    }

    // Now update the x positions for each band
    constexpr auto kInvalidX = std::numeric_limits<float>::max();
    mBandsLine.clear();
    mBandsLine.resize(mBands.size(), {kInvalidX, 1.0});

    for (int i = 0; i < mBands.size(); ++i) {
        const auto numBinsInBand = mBands[i].bins.size();
        if (numBinsInBand == 1) {
            // Just use the actual frequency position of the single bin
            const auto binFreq = mBands[i].bins[0] * deltaFreq;
            const auto logBinFreq = binFreq > 0.0 ? std::log10(binFreq) : minLog;
            const auto x0to1 = (logBinFreq - visualMinLog) / (visualMaxLog - visualMinLog);
            mBandsLine[i].x = x0to1;
        } else if (numBinsInBand > 1) {
            const auto logCenterFreq = minLog + i * logBandWidth + 0.5 * logBandWidth;
            const auto x0to1 = (logCenterFreq - visualMinLog) / (visualMaxLog - visualMinLog);
            mBandsLine[i].x = x0to1;
        }
    }

    // Now remove all the bands that don't have any bins assigned to them so we don't have gaps
    std::erase_if(mBands, [](const Band& b) { return b.bins.empty(); });
    std::erase_if(mBandsLine, [](const tb::Point& p) { return p.x == kInvalidX; });

    // If smoothing is enabled, we need to prep the smoothed line and add some control points
    mSmoothedLine.clear();
    if (mLineInterpolationSteps > 0) {
        // Here we're adding 2 control points on the front and end of the line. This ensures the
        // spline function has enough control points to work with on the ends and reduces the
        // chances of encountering interpolation artifacts at the ends.

        constexpr float kFudgeFactor = 0.0001f;

        const auto firstX = mBandsLine.front().x;
        mBandsLine.insert(mBandsLine.begin(), { firstX - kFudgeFactor, 1.f });
        mBandsLine.insert(mBandsLine.begin(), { firstX - kFudgeFactor - kFudgeFactor, 1.f });

        const auto lastX = mBandsLine.back().x;
        mBandsLine.push_back({ lastX + kFudgeFactor, 1.f });
        mBandsLine.push_back({ lastX + kFudgeFactor + kFudgeFactor, 1.f });

        // Set the proper line size now so that we avoid re-allocating while running
        mSmoothedLine.resize(tb::catmullRom::outLineSize(mBandsLine.size(), mLineInterpolationSteps));
    }

    reset();

    if (onBandsChanged)
        onBandsChanged();
}

void AnalyzerProcessor::parameterChanged() const {
    if (onParametersChanged)
        onParametersChanged();
}

int AnalyzerProcessor::clampFftHopSize(int inHopSize) const {
    return std::clamp(inHopSize, 1, mFftSize);
}