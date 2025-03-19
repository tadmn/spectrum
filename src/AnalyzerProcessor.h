
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

    std::function<void()> onSettingsChanged;

    void setSampleRate(double sampleRate);
    void setFftSize(int fftSize);

    void process(choc::buffer::ChannelArrayView<float> audio);
    void process(double deltaTimeSeconds);

    const std::vector<Band>& bands() const noexcept { return mBands; }

    void reset();

  private:
    void update();

    using RealtimeObject = farbot::RealtimeObject<std::vector<std::complex<float>>,
                                                  farbot::RealtimeObjectOptions::realtimeMutatable>;

    double mSampleRate = 44'100.0;
    int mFftSize = 2048;
    int mFftHopSize = 256;
    double mMinFrequency = 20.0;
    double mMaxFrequency = 20'000.0;
    int mNumBands = 320;
    double mMinDb = -100.0;
    double mAttack = 15.0;
    double mRelease = 0.85;

    std::vector<float> mWindow;
    std::unique_ptr<FifoBuffer<float>> mFifoBuffer;
    choc::buffer::ChannelArrayBuffer<float> mFftInBuffer;
    std::unique_ptr<FastFourier> mFft;
    std::unique_ptr<RealtimeObject> mFftComplexOutput;

    std::mutex mMutex;

    std::vector<Band> mBands;
};