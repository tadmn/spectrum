
#pragma once

#include "embedded/Fonts.h"

#include <tb_Math.h>
#include <visage/graphics.h>

class DbGridLabelsFrame : public visage::Frame {
  public:
    DbGridLabelsFrame() { setIgnoresMouseEvents(true, false); }

    ~DbGridLabelsFrame() override { }

    void setDbRange(float minDb, float maxDb) {
        tb_assert(minDb < maxDb);
        mMinDb = minDb;
        mMaxDb = maxDb;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(0x80ffffff);
        const visage::Font labelFont(11.f, resources::fonts::DroidSansMono_ttf);

        // Draw labels for each 10 dB step
        const auto firstTickDb = static_cast<int>(std::ceil(mMinDb / 10.f)) * 10;
        for (int dbValue = firstTickDb; dbValue <= mMaxDb; dbValue += 10) {
            float y0to1 = tb::to0to1(static_cast<float>(dbValue), mMinDb, mMaxDb);
            float y = height() - (y0to1 * height());
            const auto h = 20.f;
            y = y - h / 2;

          if (y < 0.f || y + h > height())
          continue;

          canvas.text(visage::String(dbValue), labelFont, visage::Font::Justification::kCenter, 0,
                      y, width(), h);
        }
    }

  private:
    float mMinDb = -100.f;
    float mMaxDb = 0.f;
};