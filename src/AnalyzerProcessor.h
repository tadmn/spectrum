
#pragma once

#include "Spectrum.h"

class AnalyzerProcessor {
  public:
    AnalyzerProcessor(int fftSize, int numBands, double sampleRate,
                      double minFrequency, double maxFrequency, double minDb,
                      double attack, double release);

    void process(double deltaTimeSeconds, const FftComplexOutput& fftOutput);

    struct Band { std::vector<int> bins; double dB; double x0to1; double y0to1; };
    const std::vector<Band>& bands() const noexcept { return mBands;}

    void reset();

  private:
    const double mMinDb;
    const double mAttack;
    const double mRelease;
    const double mDeltaFrequency;
    std::vector<Band> mBands;
};