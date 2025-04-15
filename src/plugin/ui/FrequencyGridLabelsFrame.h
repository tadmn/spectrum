
#pragma once

#include "embedded/Fonts.h"

#include <visage/graphics.h>
#include <tb_Math.h>

class AnalyzerFrequencyGridLabelsFrame : public visage::Frame {
  public:
    AnalyzerFrequencyGridLabelsFrame() { }
    ~AnalyzerFrequencyGridLabelsFrame() override { }

    void setFrequencyRange(float minFreq, float maxFreq) {
        tb_assert(minFreq > 0.f && maxFreq > minFreq);
        mMinFreq = minFreq;
        mMaxFreq = maxFreq;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        constexpr std::array<std::pair<float, const char*>, 10> kFreqLabels { { { 20.f, "20" },
                                                                                { 50.f, "50" },
                                                                                { 100.f, "100" },
                                                                                { 200.f, "200" },
                                                                                { 500.f, "500" },
                                                                                { 1000.f, "1k" },
                                                                                { 2000.f, "2k" },
                                                                                { 5000.f, "5k" },
                                                                                { 10000.f, "10k" },
                                                                                { 20000.f,
                                                                                  "20k" } } };

        canvas.setColor(0x80ffffff);

        const visage::Font labelFont(11.f, resources::fonts::DroidSansMono_ttf);

        const auto minLogFreq = std::log10(mMinFreq);
        const auto maxLogFreq = std::log10(mMaxFreq);

        // Draw labels for frequencies that are in the visible range
        for (const auto& freqLabel : kFreqLabels) {
            if (freqLabel.first >= mMinFreq && freqLabel.first <= mMaxFreq) {
                float labelX = width() * tb::to0to1(std::log10(freqLabel.first), minLogFreq, maxLogFreq);

                const auto w = 30;

                // Draw label
                canvas.text(freqLabel.second, labelFont, visage::Font::Justification::kCenter,
                            labelX - w / 2, 0, w, height());
            }
        }
    }

  private:
    float mMinFreq = 15.f;
    float mMaxFreq = 22'000.f;
};