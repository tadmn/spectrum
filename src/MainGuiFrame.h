
#pragma once

#include "LiveValue.h"
#include "Spectrum.h"

#include <visage/graphics.h>
#include <visage/widgets.h>

class Spectrum::MainGuiFrame : public visage::Frame {
  public:
    MainGuiFrame(Spectrum& p) : mPlugin(p), mLine(kNumDisplayBands + 2) {
        addChild(mLine);
        setIgnoresMouseEvents(true, false);
        mPrevDbValues.fill(kMinDbVisible);
    }

    ~MainGuiFrame() override { }

    void resized() override {
        mLine.setBounds(0, 0, width(), height());

        // Tether the first and last points to 0
        mLine.setXAt(0, 0);
        mLine.setYAt(0, mLine.height());
        mLine.setXAt(mLine.numPoints() - 1, mLine.width());
        mLine.setYAt(mLine.numPoints() - 1, mLine.height());
    }

    void draw(visage::Canvas& canvas) override {
        FftComplexOutput fftOut;

        {
            RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> f(mPlugin.mFftComplexOutput);
            fftOut = *f;
        }

        canvas.setColor(0xffffffff);

        LIVE_VALUE(kMinFreq, 20);
        LIVE_VALUE(kMaxFreq, 20'000);

        LIVE_VALUE(kAttack, 15.0);
        LIVE_VALUE(kRelease, 0.85);

        const auto kAttackRate = std::clamp(kAttack * canvas.deltaTime(), 0.0, 1.0);
        const auto kReleaseRate = std::clamp(kRelease * canvas.deltaTime(), 0.0, 1.0);

        const auto logMinFreq = std::log10(kMinFreq);
        const auto logMaxFreq = std::log10(kMaxFreq);
        const auto logDisplayStep = (logMaxFreq - logMinFreq) / kNumDisplayBands;

        const auto deltaFreq = mPlugin.sampleRate() / kFftSize;

        std::array<int, kNumDisplayBands> numBinsInDisplayBand = {};
        std::array<double, kNumDisplayBands> displayBands = {};

        for (int i = 0; i < kNumFftBins; i++) {
            const auto freq = i * deltaFreq;
            if (freq < kMinFreq || freq > kMaxFreq)
                continue;

            double mag = std::abs(fftOut[i]);

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

            // Figure out which band this bin will get added to
            const auto logFreq = std::log10(freq);
            auto index = static_cast<int>((logFreq - logMinFreq) / logDisplayStep);
            assert(index >= 0 && index < kNumDisplayBands);
            index = std::clamp(index, 0, kNumDisplayBands - 1);

            // Add this bin's energy to the selected band
            const auto energy = mag * mag;
            displayBands[index] += energy;
            numBinsInDisplayBand[index]++;
        }

        // This allows us to skip over bands that have no bins assigned to them, thus avoiding
        // gaps in the spectrum
        int lineIndex = 1;

        for (int i = 0; i < kNumDisplayBands; ++i) {
            if (numBinsInDisplayBand[i] == 0)
                continue;

            auto energy = displayBands[i];

            // Average the energy
            energy /= numBinsInDisplayBand[i];

            // Convert to dB
            double dB = kMinDbVisible;
            if (energy > 0.0)
                dB = 10.0 * std::log10(energy);

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

            const auto xBandWidth = static_cast<double>(mLine.width()) / kNumDisplayBands;
            const auto x = i * xBandWidth + xBandWidth / 2.0;
            mLine.setXAt(lineIndex, x);
            mLine.setYAt(lineIndex, y0to1 * mLine.height());

            lineIndex++;
        }

        for (int i = lineIndex; i < mLine.numPoints(); ++i) {
            mLine.setXAt(i, mLine.width());
            mLine.setYAt(i, mLine.height());
        }

        redraw();
    }

  private:
    Spectrum& mPlugin;
    visage::GraphLine mLine;
    std::array<double, kNumDisplayBands> mPrevDbValues {};
};
