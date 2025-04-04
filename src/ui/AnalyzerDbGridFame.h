
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

    // Find the first multiple of 10 above mMinDb
    const auto firstTickDb = static_cast<int>(std::ceil(mMinDb / 10.f)) * 10;

    // Draw lines for each 10 dB step
    for (int dbValue = firstTickDb; dbValue <= mMaxDb; dbValue += 10) {
      // Calculate vertical position (convert from dB to y coordinate)
      float y0to1 = tb::to0to1(static_cast<float>(dbValue), mMinDb, mMaxDb);
      float lineY = height() - (y0to1 * height());

      const auto y = std::round(lineY * canvas.dpiScale());

      // Draw the horizontal line
      const visage::Color color(0xffffffff);

      const float w = nativeWidth();
      const float h = nativeHeight();

      canvas.setBrush(visage::Brush::linear(color.withAlpha(0), color.withAlpha(0.2),
                                            { 0, h / 2.f }, { 0.2f * w, h / 2.f }));
      canvas.segment(0, y, nativeWidth(), y, 1, false);
    }
  }

private:
  float mMinDb = -100.f;
  float mMaxDb = 0.f;
};