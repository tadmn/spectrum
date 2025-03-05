
#pragma once

#include "Spectrum.h"
#include <visage/graphics.h>
#include "visage_widgets/graph_line.h"

class Spectrum::MainGuiFrame : public visage::Frame
{
public:
    MainGuiFrame(Spectrum & p) : mPlugin(p), mLine(kNumFftBins)
    {
        addChild(mLine);
        setIgnoresMouseEvents(true, false);
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

        constexpr auto kMinFreq = 10.0;
        constexpr auto kMaxFreq = 25'000.0;

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

                auto const mag = 20.0 * std::log10(std::abs(fftOut[(uint)i]));
                auto const y = 1.0 - mag / 35.0;
                mLine.setYAt((int)i, static_cast<float>(y * mLine.height()));
            }
        }

        redraw();
    }


private:
    Spectrum & mPlugin;
    visage::GraphLine mLine;
};
