
#pragma once

#include "Common.h"

#include <visage/graphics.h>
#include <tb_Math.h>

class GridFrame : public visage::Frame {
public:
  GridFrame() {
    setMasked(true);
  }

  ~GridFrame() override { }

  void setFrequencyRange(float minFreq, float maxFreq) {
    tb_assert(minFreq > 0.f && minFreq < maxFreq);
    mMinFreq = minFreq;
    mMaxFreq = maxFreq;
    redraw();
  }

  void setDbRange(float minDb, float maxDb) {
    tb_assert(minDb < maxDb);
    mMinDb = minDb;
    mMaxDb = maxDb;
    redraw();
  }

  void draw(visage::Canvas& canvas) override {
    canvas.setColor(common::backgroundColor());
    canvas.fill(0, 0, width(), height());

    drawFreqGrid(canvas);
    drawDbGrid(canvas);

    canvas.setBlendMode(visage::BlendMode::Mult);
    const visage::Color c0(0x00ffffff);
    const visage::Color c1(0xffffffff);
    canvas.setBrush(visage::Brush::vertical(visage::Gradient(c0, c1, c1, c1, c1, c0)));
    canvas.fill(0, 0, width(), height());
    canvas.setBrush(visage::Brush::horizontal(visage::Gradient(c0, c1, c1, c1, c1, c0)));
    canvas.fill(0, 0, width(), height());
  }

private:
  void drawFreqGrid(visage::Canvas& canvas) {
        canvas.setNativePixelScale();

        // Setup horizontal fade in/out to transparent for aesthetics
        const auto minLogFreq = std::log10(mMinFreq);
        const auto maxLogFreq = std::log10(mMaxFreq);

        // Find the first decade starting point (10, 100, 1000, etc.)
        const auto decadeStart = std::pow(10.f, std::ceil(minLogFreq));

        // Draw the grid lines
        for (float freq = decadeStart / 10.f; freq <= mMaxFreq; freq *= 10.0f) {
            // For each decade, draw lines at 1x, 2x, 3x, ..., 9x
            for (int i = 1; i < 10; i++) {
                const auto currFreq = i * freq;
                tb_assert(currFreq >= 0.f);

                // Skip frequencies outside our range
                if (currFreq < mMinFreq || currFreq > mMaxFreq)
                    continue;

                auto lineX = width() * tb::to0to1(std::log10(currFreq), minLogFreq, maxLogFreq);

                // Ensure we're not drawing outside our canvas area
                if (lineX < 0.f || lineX > width())
                    continue;

                lineX *= canvas.dpiScale();
                lineX = std::round(lineX);

              const visage::Color lineColor(0xffffffff);
                if (i == 1) {
                    // Major lines at 1x, 10x, 100x, etc (decade boundaries)
                    canvas.setColor(lineColor.withAlpha(0.4));
                    canvas.segment(lineX, 0, lineX, nativeHeight(), 1, false);
                } else {
                    // Minor lines at other multiples
                    canvas.setColor(lineColor.withAlpha(0.2));
                    canvas.segment(lineX, 0, lineX, nativeHeight(), 1, false);
                }
            }
        }

    canvas.setLogicalPixelScale();
    }

  void drawDbGrid(visage::Canvas& canvas) {
    canvas.setNativePixelScale();

    canvas.setColor(visage::Color(0xffffff).withAlpha(0.2));

    // Draw lines for each 10 dB step
    const auto firstTickDb = static_cast<int>(std::ceil(mMinDb / 10.f)) * 10;
    for (int dbValue = firstTickDb; dbValue <= mMaxDb; dbValue += 10) {
      // Calculate vertical position (convert from dB to y coordinate)
      float y0to1 = tb::to0to1(static_cast<float>(dbValue), mMinDb, mMaxDb);
      float lineY = height() - (y0to1 * height());

      const auto y = std::round(lineY * canvas.dpiScale());

      // Draw the horizontal line
      canvas.segment(0, y, nativeWidth(), y, 1, false);
    }

    canvas.setLogicalPixelScale();
  }

  float mMinFreq = 15.f;
  float mMaxFreq = 22'000.f;

  float mMinDb = -100.f;
  float mMaxDb = 0.f;
};
