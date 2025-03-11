
#pragma once

#include "Spectrum.h"
#include "LiveValue.h"

#include <visage/widgets.h>
#include <visage/graphics.h>

class Spectrum::MainGuiFrame : public visage::Frame
{
public:
    MainGuiFrame(Spectrum & p) : mPlugin(p), mLine(kNumFftBins)
    {
        addChild(mLine);
        setIgnoresMouseEvents(true, false);
        std::ranges::fill(mPrevMags0to1, 0);
    }

    ~MainGuiFrame() override {}

    void resized() override
    {
        mLine.setBounds(0, 0, width(), height());
    }

    void draw(visage::Canvas &canvas) override
    {
        FftComplexOutput fftOut;

        {
            RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> f(mPlugin.mFftComplexOutput);
            fftOut = *f;
        }

        canvas.setColor(0xffffffff);

        LIVE_VALUE(kMinFreq, 10);
        LIVE_VALUE(kMaxFreq, 25'000);

        LIVE_VALUE(kAttack, 4.5);
        LIVE_VALUE(kRelease, 0.75);

        auto const kAttackRate = std::clamp(kAttack * canvas.deltaTime(), 0.0, 1.0);
        auto const kReleaseRate = std::clamp(kRelease * canvas.deltaTime(), 0.0, 1.0);

        auto const logMinFreq = std::log10(kMinFreq);
        auto const logMaxFreq = std::log10(kMaxFreq);

        auto const deltaFreq = mPlugin.sampleRate() / kFftSize;

        for (int i = 0; i < (int)kNumFftBins; ++i)
        {
            auto const freq = i * deltaFreq;

            if (freq < kMinFreq)
            {
                mLine.setXAt(i, 0.f);
                mLine.setYAt(i, mLine.height());
            }
            else if (freq > kMaxFreq)
            {
                mLine.setXAt(i, mLine.width());
                mLine.setYAt(i, mLine.height());
            }
            else
            {
                auto const x = (std::log10(freq) - logMinFreq) / (logMaxFreq - logMinFreq);
                mLine.setXAt((int)i, static_cast<float>(x * mLine.width()));

                auto y = 0.0;

                double mag = std::abs(fftOut[(uint)i]);
                if (mag > 0.0)
                {
                    // A-weighting
                    {
                        const auto f2 = freq * freq;
                        const auto f4 = f2 * f2;
                        const auto w = (148'693'636.0 * f4) /
                            ((f2 + 424.36) * std::sqrt((f2 + 11'599.29) * (f2 + 544'496.41) * (f2 + 148'693'636.0)));

                        mag *= w;
                    }

                    LIVE_VALUE(kBias, -0.6f);
                    LIVE_VALUE(kScale_dB, 90.0);

                    auto const dB = 20.0 * std::log10(mag);
                    y = kBias + dB / kScale_dB; // [0,1+]
                }

                auto const oldY = mPrevMags0to1[(uint)i];

                if (y > oldY)
                {
                    y = kAttackRate * y + (1.0 - kAttackRate) * oldY;
                } else
                {
                    y = kReleaseRate * y + (1.0 - kReleaseRate) * oldY;
                }

                mPrevMags0to1[(uint)i] = y;
                mLine.setYAt((int)i, static_cast<float>((1.0 - y) * mLine.height()));
            }
        }

        redraw();
    }


private:
    Spectrum & mPlugin;
    visage::GraphLine mLine;
    std::array<double, kNumFftBins> mPrevMags0to1;
};
