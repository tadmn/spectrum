#pragma once

#include "common/Common.h"

class FullScreenButton : public Button {
  public:
    void setFullScreen(bool full_screen) {
        full_screen_ = full_screen;
        redraw();
    }

    void draw(Canvas& canvas, float hover_amount) override {
        // Opaque, because the arms of each corner overlap and would otherwise blend into a brighter spot
        canvas.setColor(Color(0xff5d5d5d).interpolateWith(0xffcfcfcf, hover_amount));

        const auto margin = 6.0f;
        const auto thickness = 1.5f;
        const auto arm = 4.0f;
        const auto left = margin;
        const auto top = margin;
        const auto right = width() - margin;
        const auto bottom = height() - margin;

        // Corners that point outwards to go full screen, and inwards to come back
        const auto corner = [&](float corner_x, float corner_y, float x_direction, float y_direction) {
            const auto x = full_screen_ ? corner_x + x_direction * (arm - thickness) : corner_x;
            const auto y = full_screen_ ? corner_y + y_direction * (arm - thickness) : corner_y;
            const auto arm_x = full_screen_ ? -x_direction : x_direction;
            const auto arm_y = full_screen_ ? -y_direction : y_direction;

            canvas.rectangle(arm_x > 0 ? x : x + thickness - arm, y, arm, thickness);
            canvas.rectangle(x, arm_y > 0 ? y : y + thickness - arm, thickness, arm);
        };

        corner(left, top, 1, 1);
        corner(right - thickness, top, -1, 1);
        corner(left, bottom - thickness, 1, -1);
        corner(right - thickness, bottom - thickness, -1, -1);
    }

  private:
    bool full_screen_ = false;

    VISAGE_LEAK_CHECKER(FullScreenButton)
};
