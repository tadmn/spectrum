
#pragma once

#include <tb_FifoBuffer.h>
#include <tb_Interpolation.h>
#include <farbot/RealtimeObject.hpp>
#include <FastFourier/FastFourier.h>

#include <vector>
#include <complex>

class AnalyzerProcessor {
  public:
    struct Band {
        std::vector<int> bins = {};
        double dB = -100.0;
    };

    AnalyzerProcessor();

    std::function<void()> onParametersChanged;
    std::function<void()> onBandsChanged;

    const std::vector<tb::Point>& spectrumLine() const;
    const std::vector<Band>& bands() const noexcept { return mBands; }

    void setTargetNumBands(int targetNumberOfBands);

    void setSampleRate(double sampleRate);
    double sampleRate() const noexcept { return mSampleRate; }

    void setFftSize(int fftSize);
    int fftSize() const noexcept { return mFftSize; }

    void setMinFrequency(double minFreq);
    double minFrequency() const noexcept { return mMinFrequency; }

    void setMaxFrequency(double maxFreq);
    double maxFrequency() const noexcept { return mMaxFrequency; }

    void setWeightingDbPerOctave(double dbPerOctave);
    double weightingDbPerOctave() const noexcept;

    void setWeightingCenterFrequency(double centerFrequency);
    double weightingCenterFrequency() const noexcept;

    void setLineSmoothingFactor(double factor);
    double lineSmoothingFactor() const noexcept { return mLineSmoothingFactor; }

    void setFftHopSize(int hopSize);
    int fftHopSize() const noexcept { return mFftHopSize.load(std::memory_order_relaxed); }

    void setAttackRate(double attackRate);
    double attackRate() const noexcept { return mAttack.load(std::memory_order_relaxed); }

    void setReleaseRate(double releaseRate);
    double releaseRate() const noexcept { return mRelease.load(std::memory_order_relaxed); }

    void setMinDb(double minDb);
    double minDb() const noexcept { return mMinDb.load(std::memory_order_relaxed); }

    void process(choc::buffer::ChannelArrayView<float> audio);
    void process(double deltaTimeSeconds);

    void reset();

  private:
    void updateBands();
    void parameterChanged() const;
    int clampFftHopSize(int inHopSize) const;

    using RealtimeObject = farbot::RealtimeObject<std::vector<std::complex<float>>,
                                                  farbot::RealtimeObjectOptions::realtimeMutatable>;

    // "Non-realtime" parameters (i.e. they require a more hefty internal update, with buffers & the
    // band vector being resized, etc.)
    double mSampleRate = 44'100.0;
    int mFftSize = 8'192;
    double mMinFrequency = 15.0;
    double mMaxFrequency = 22'000.0;
    int mTargetNumBands = 320;
    double mWeightingDbPerOctave = 6.0;
    double mWeightingCenterFrequency = 1'000.0;
    double mLineSmoothingFactor = 8.0;

    // "Realtime" parameters. Usually just a lightweight atomic `store`
    std::atomic<int> mFftHopSize = 1024;
    std::atomic<double> mAttack = 15.0;
    std::atomic<double> mRelease = 0.85;
    std::atomic<double> mMinDb = -100.0;

    std::vector<float> mWindow;
    std::unique_ptr<tb::FifoBuffer<float>> mFifoBuffer;
    choc::buffer::ChannelArrayBuffer<float> mFftInBuffer;
    std::unique_ptr<FastFourier> mFft;
    std::unique_ptr<RealtimeObject> mFftComplexOutput;

    std::mutex mMutex;

    std::vector<double> mBinWeights;
    std::vector<Band> mBands;

    std::vector<tb::Point> mBandsLine;
    std::vector<tb::Point> mSmoothedLine;
};