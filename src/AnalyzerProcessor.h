
#pragma once

#include "FifoBuffer.h"

#include <choc_SampleBuffers.h>
#include <farbot/RealtimeObject.hpp>
#include <FastFourier/FastFourier.h>

#include <vector>
#include <complex>

class AnalyzerProcessor {
  public:
    struct Band {
        std::vector<int> bins = {};
        double dB = -100.0;
        double x0to1 = 0.0;
        double y0to1 = 0.0;
    };

    AnalyzerProcessor();

    std::function<void()> onParametersChanged;
    std::function<void()> onBandsChanged;

    void setSampleRate(double sampleRate);

    void setFftSize(int fftSize);
    int fftSize() const noexcept { return mFftSize; }

    void setFftHopSize(int hopSize);
    int fftHopSize() const noexcept { return mFftHopSize.load(std::memory_order_relaxed); }

    void setAttackRate(double attackRate);
    double attackRate() const noexcept { return mAttack; }

    void setReleaseRate(double releaseRate);
    double releaseRate() const noexcept { return mRelease; }

    void process(choc::buffer::ChannelArrayView<float> audio);
    void process(double deltaTimeSeconds);

    const std::vector<Band>& bands() const noexcept { return mBands; }

    void reset();

  private:
    void updateBands();
    void parameterChanged() const;
    int clampFftHopSize(int inHopSize) const;

    using RealtimeObject = farbot::RealtimeObject<std::vector<std::complex<float>>,
                                                  farbot::RealtimeObjectOptions::realtimeMutatable>;

    // "Non-realtime" parameters (i.e. they require a more hefty internal update, with buffers & the
    // band vector being resized, etc)
    double mSampleRate = 44'100.0;
    int mFftSize = 2048;
    double mMinFrequency = 20.0;
    double mMaxFrequency = 20'000.0;
    int mNumBands = 320;
    double mMinDb = -100.0;

    // "Realtime" parameters. Usually just a lightweight atomic `store`
    std::atomic<int> mFftHopSize = 256;
    std::atomic<double> mAttack = 15.0;
    std::atomic<double> mRelease = 0.85;
    double mWeightingDbPerOctave = 6.0;
    double mWeightingCenterFrequency = 1'000.0;

    std::vector<float> mWindow;
    std::unique_ptr<FifoBuffer<float>> mFifoBuffer;
    choc::buffer::ChannelArrayBuffer<float> mFftInBuffer;
    std::unique_ptr<FastFourier> mFft;
    std::unique_ptr<RealtimeObject> mFftComplexOutput;

    std::mutex mMutex;

    std::vector<Band> mBands;
};