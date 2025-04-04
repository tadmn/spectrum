
#pragma once

#include <visage/graphics.h>
#include <tb_Math.h>

class AnalyzerFrequencyGridFrame : public visage::Frame {
  public:
    AnalyzerFrequencyGridFrame() {
        setIgnoresMouseEvents(true, false);
    }

    ~AnalyzerFrequencyGridFrame() override { }

    void setFrequencyRange(float minFreq, float maxFreq) {
        assert(minFreq > 0.f && maxFreq > minFreq);
        mMinFreq = minFreq;
        mMaxFreq = maxFreq;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        canvas.setNativePixelScale();

        // Setup horizontal fade in/out to transparent for aesthetics
        const auto c0 = visage::Color(0xffffff).withAlpha(0);
        const auto c1 = c0.withAlpha(1);
        const auto brush = visage::Brush::linear(visage::Gradient(c0, c1, c1, c1, c1, c0),
                                                 { 0.f, nativeHeight() / 2.f },
                                                 { static_cast<float>(nativeWidth()), nativeHeight() / 2.f });

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

                auto lineX = width() * tb::to0to1(std::log10(currFreq), minLogFreq, maxLogFreq);

                // Ensure we're not drawing outside our canvas area
                if (lineX < 0.f || lineX > width())
                    continue;

                lineX *= canvas.dpiScale();
                lineX = std::round(lineX);

                if (i == 1) {
                    // Major lines at 1x, 10x, 100x, etc (decade boundaries)
                    canvas.setBrush(brush.withMultipliedAlpha(0.4));
                    canvas.segment(lineX, 0, lineX, nativeHeight(), 1, false);
                } else {
                    // Minor lines at other multiples
                    canvas.setBrush(brush.withMultipliedAlpha(0.2));
                    canvas.segment(lineX, 0, lineX, nativeHeight(), 1, false);
                }
            }
        }

        canvas.setLogicalPixelScale();

        // Add a gradient fade-out to black at the bottom for aesthetics
        canvas.setColor(visage::Brush::linear(visage::Gradient(0x00000000, 0xff000000),
                                              { width() / 2, height() - (0.4f * height()) },
                                              { width() / 2, height() }));
        canvas.fill(0, 0, width(), height());
    }

  private:
    float mMinFreq = 15.f;
    float mMaxFreq = 22'000.f;
};