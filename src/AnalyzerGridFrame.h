
#pragma once

#include <visage/graphics.h>
#include <tb_Math.h>

class AnalyzerGridFrame : public visage::Frame {
  public:
    AnalyzerGridFrame() {
        setIgnoresMouseEvents(true, false);
    }

    ~AnalyzerGridFrame() override { }

    void setFrequencyRange(float min, float max) {
        assert(min > 0.f && max > min);
        mMinFreq = min;
        mMaxFreq = max;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        const auto minLogFreq = std::log10(mMinFreq);
        const auto maxLogFreq = std::log10(mMaxFreq);

        // Find the first decade starting point (10, 100, 1000, etc.)
        const auto decadeStart = std::pow(10.f, std::ceil(minLogFreq));

        // Draw the grid lines
        for (float freq = decadeStart / 10.f; freq <= mMaxFreq; freq *= 10.0f) {
            // For each decade, draw lines at 1x, 2x, 3x, ..., 9x
            for (int i = 1; i < 10; i++) {
                const auto currFreq = i * freq;
                assert(currFreq >= 0.f);

                // Skip frequencies outside our range
                if (currFreq < mMinFreq || currFreq > mMaxFreq)
                    continue;

                const auto lineX = std::round(width() * tb::to0to1(std::log10(currFreq), minLogFreq,
                                                                   maxLogFreq));

                // Ensure we're not drawing outside our canvas area
                if (lineX < 0.f || lineX > width())
                    continue;

                if (i == 1) {
                    // Major lines at 1x, 10x, 100x, etc (decade boundaries)
                    canvas.setColor(visage::Color(0xfeffffff));
                    canvas.segment(lineX, 0, lineX, height(), 1, false);
                } else {
                    // Minor lines at other multiples
                    canvas.setColor(0xfaffffff);
                    canvas.segment(lineX, 0, lineX, height(), 1, false);
                }
            }
        }

        // Add a gradient fade-in & fade-out in the y-axis for aesthetics
        const auto gradientFade = height() / 4;

        {
            const auto brush = visage::Brush::linear(visage::Gradient(0x00000000, 0xff000000),
                                                 { width() / 2, gradientFade }, { width() / 2, 0.f });
            canvas.setColor(brush);
            canvas.fill(0, 0, width(), height());
        }

        {
            const auto brush = visage::Brush::linear(visage::Gradient(0x00000000, 0xff000000),
                                                 { width() / 2, height() - (gradientFade) }, { width() / 2, height() });
            canvas.setColor(brush);
            canvas.fill(0, 0, width(), height());
        }
    }

  private:
    float mMinFreq = 15.f;
    float mMaxFreq = 22'000.f;
};