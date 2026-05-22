
#pragma once

#include <tb_Math.h>

#include "common/Common.h"
#include "embedded/Fonts.h"

class DbGridLabelsFrame : public Frame {
  public:
    DbGridLabelsFrame() { setIgnoresMouseEvents(true, true); }

    void setDbRange(float min_dB, float max_dB) {
        tb_assert(min_dB < max_dB);
        min_dB_ = min_dB;
        max_dB_ = max_dB;
        redraw();
    }

    void draw(Canvas& canvas) override {
        canvas.setColor(Color(0xffffff).withAlpha(0.4f));
        const Font label_font(11, resources::fonts::DroidSansMono_ttf);

        // Draw labels for each 10 dB step
        const auto first_tick_dB = static_cast<int>(std::ceil(min_dB_ / 10.0f)) * 10;
        for (int dB = first_tick_dB; dB <= max_dB_; dB += 10) {
            float y_0to1 = tb::to0to1(static_cast<float>(dB), min_dB_, max_dB_);
            float y = height() - (y_0to1 * height());
            const auto h = 20.0f;
            y = y - h / 2;
            
            if (y < 0.0f || y + h > height())
                continue;

            canvas.text(String(dB), label_font, Font::Justification::kCenter, 0,
                        y, width(), h);
        }
    }

  private:
    float min_dB_ = -100.0f;
    float max_dB_ = 0.0f;

    VISAGE_LEAK_CHECKER(DbGridLabelsFrame)
};