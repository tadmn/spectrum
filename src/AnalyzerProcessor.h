
#pragma once

#include <tb_FifoBuffer.h>
#include <tb_Interpolation.h>
#include <tb_Windowing.h>
#include <farbot/RealtimeObject.hpp>
#include <FastFourier.h>

#include <vector>
#include <complex>
#include <functional>

class AnalyzerProcessor {
  public:
    struct Band {
        std::vector<int> bins = {};
        float dB = -100.f;
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

    void setMinFrequency(float minFreq);
    float minFrequency() const noexcept { return mMinFrequency; }

    void setMaxFrequency(float maxFreq);
    float maxFrequency() const noexcept { return mMaxFrequency; }

    void setMinDb(float minDb);
    float minDb() const noexcept { return mMinDb.load(std::memory_order_relaxed); }

    void setWeightingDbPerOctave(float dbPerOctave);
    float weightingDbPerOctave() const noexcept;

    void setWeightingCenterFrequency(float centerFrequency);
    float weightingCenterFrequency() const noexcept;

    void setLineSmoothingFactor(float factor);
    float lineSmoothingFactor() const noexcept { return mLineSmoothingFactor; }

    void setWindowType(tb::WindowType windowType);
    tb::WindowType windowType() const noexcept { return mWindowType; }

    void setFftHopSize(int hopSize);
    int fftHopSize() const noexcept { return mFftHopSize.load(std::memory_order_relaxed); }

    void setAttackRate(float attackRate);
    float attackRate() const noexcept { return mAttack.load(std::memory_order_relaxed); }

    void setReleaseRate(float releaseRate);
    float releaseRate() const noexcept { return mRelease.load(std::memory_order_relaxed); }

    void processAudio(choc::buffer::ChannelArrayView<float> audio);
    void processAnalyzer(double deltaTimeSeconds);

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
    int mFftSize = 4'096.0;
    float mMinFrequency = 15.f;
    float mMaxFrequency = 22'000.f;
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
};