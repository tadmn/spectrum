
#pragma once

#include "LiveValue.h"
#include "Spectrum.h"

#include <visage/graphics.h>
#include <visage/widgets.h>

class Spectrum::MainGuiFrame : public visage::Frame {
  public:
    MainGuiFrame(Spectrum& p) : mPlugin(p), mLine(kNumFftBins) {
        addChild(mLine);
        setIgnoresMouseEvents(true, false);
        std::ranges::fill(mPrevDbValues, 0);
    }

    ~MainGuiFrame() override { }

    void resized() override { mLine.setBounds(0, 0, width(), height()); }

    void draw(visage::Canvas& canvas) override {
        FftComplexOutput fftOut;

        {
            RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> f(mPlugin.mFftComplexOutput);
            fftOut = *f;
        }

        canvas.setColor(0xffffffff);

        LIVE_VALUE(kMinFreq, 20);
        LIVE_VALUE(kMaxFreq, 20'000);

        LIVE_VALUE(kAttack, 4.5);
        LIVE_VALUE(kRelease, 0.75);

        const auto kAttackRate = std::clamp(kAttack * canvas.deltaTime(), 0.0, 1.0);
        const auto kReleaseRate = std::clamp(kRelease * canvas.deltaTime(), 0.0, 1.0);

        const auto logMinFreq = std::log10(kMinFreq);
        const auto logMaxFreq = std::log10(kMaxFreq);

        const auto deltaFreq = mPlugin.sampleRate() / kFftSize;

        for (int i = 0; i < kNumFftBins; ++i) {
            const auto freq = i * deltaFreq;

            if (freq <= 0.0) {
                mLine.setXAt(i, 0);
                mLine.setYAt(i, mLine.height());  //todo show actual DC value from fft
            } else {
                // Logarithmic spacing for x-axis
                const auto x = (std::log10(freq) - logMinFreq) / (logMaxFreq - logMinFreq);
                mLine.setXAt(i, x * mLine.width());

                LIVE_VALUE(kMinDbVisible, -100);
                LIVE_VALUE(kMaxDbVisible, 0);

                // Used primarily for ballistics. This will be the target that the
                // ballistics use when the magnitude value is less than `kMinDbVisible`
                LIVE_VALUE(kMinDb, -110);

                double dB = kMinDb;
                if (double mag = std::abs(fftOut[i]); mag >= std::pow(10.0, kMinDbVisible / 20.0)) {
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
                        const auto w = std::pow(10.0, (octaves * kSlopeDb) / 20.0);
                        mag *= w;
                    }

                    dB = 20.0 * std::log10(mag);
                }

                // Calculate ballistics
                {
                    const auto oldDb = mPrevDbValues[i];
                    if (dB > oldDb)
                        dB = kAttackRate * dB + (1.0 - kAttackRate) * oldDb;
                    else
                        dB = kReleaseRate * dB + (1.0 - kReleaseRate) * oldDb;
                }

                mPrevDbValues[i] = dB;

                const auto y0to1 = dB / kMinDbVisible;
                mLine.setYAt(i, y0to1 * mLine.height());
            }
        }

        redraw();
    }

  private:
    Spectrum& mPlugin;
    visage::GraphLine mLine;
    std::array<double, kNumFftBins> mPrevDbValues;
};
