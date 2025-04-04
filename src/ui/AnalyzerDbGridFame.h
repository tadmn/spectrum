
#pragma once

#include <visage/graphics.h>
#include <tb_Math.h>

class AnalyzerDbGridFame : public visage::Frame {
public:
  AnalyzerDbGridFame() {}
  ~AnalyzerDbGridFame() override{}

  void setDbRange(float minDb, float maxDb) {
      assert(minDb < maxDb);
      mMinDb = minDb;
      mMaxDb = maxDb;
  }

  void draw(visage::Canvas& canvas) override {
    canvas.setNativePixelScale();

    {
      // Set the horizontal fade in/out to transparent for aesthetics
      const auto c0 = visage::Color(0xffffff).withAlpha(0);
      const auto c1 = c0.withAlpha(0.2);
      canvas.setBrush(visage::Brush::horizontal(visage::Gradient(c0, c1, c1, c1, c1, c0)));
    }

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
  }

private:
  float mMinDb = -100.f;
  float mMaxDb = 0.f;
};