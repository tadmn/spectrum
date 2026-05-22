
#pragma once

#include <tb_Math.h>

#include "common/Common.h"

class GridFrame : public Frame {
  public:
    GridFrame() {
        setMasked(true);
        setIgnoresMouseEvents(true, true);
    }

    void setFrequencyRange(float min_freq, float max_freq) {
        tb_assert(min_freq > 0.0f && min_freq < max_freq);
        min_freq_ = min_freq;
        max_freq_ = max_freq;
        redraw();
    }

    void setDbRange(float min_dB, float max_dB) {
        tb_assert(min_dB < max_dB);
        min_dB_ = min_dB;
        max_dB_ = max_dB;
        redraw();
    }

    void draw(Canvas& canvas) override {
        canvas.setColor(spectrum::backgroundColor());
        canvas.fill();

        drawFreqGrid(canvas);
        drawDbGrid(canvas);

        canvas.setBlendMode(BlendMode::Mult);
        const Color c0(0x00ffffff);
        const Color c1(0xffffffff);
        canvas.setBrush(Brush::vertical(Gradient(c0, c1, c1, c1, c1, c0)));

        // The `+ 1` here is to cover a visual bug on Windows & Linux
        canvas.fill(0, 0, width(), height() + 1);

        canvas.setBrush(Brush::horizontal(Gradient(c0, c1, c1, c1, c1, c0)));
        canvas.fill(0, 0, width(), height());
    }

  private:
    void drawFreqGrid(Canvas& canvas) {
        canvas.setNativePixelScale();

        // Setup horizontal fade in/out to transparent for aesthetics
        const auto min_log_freq = std::log10(min_freq_);
        const auto max_log_freq = std::log10(max_freq_);

        // Find the first decade starting point (10, 100, 1000, etc.)
        const auto decade_start = std::pow(10.f, std::ceil(min_log_freq));

        // Draw the grid lines
        for (float freq = decade_start / 10.0f; freq <= max_freq_; freq *= 10.f) {
            // For each decade, draw lines at 1x, 2x, 3x, ..., 9x
            for (int i = 1; i < 10; i++) {
                const auto curr_freq = i * freq;
                tb_assert(curr_freq >= 0.0f);

                // Skip frequencies outside our range
                if (curr_freq < min_freq_ || curr_freq > max_freq_)
                    continue;

                auto line_x = width() * tb::to0to1(std::log10(curr_freq), min_log_freq, max_log_freq);

                // Ensure we're not drawing outside our canvas area
                if (line_x < 0.0f || line_x > width())
                    continue;

                line_x *= canvas.dpiScale();
                line_x = std::round(line_x);

                const Color line_color(0xffffffff);
                if (i == 1) {
                    // Major lines at 1x, 10x, 100x, etc (decade boundaries)
                    canvas.setColor(line_color.withAlpha(0.4f));
                    canvas.segment(line_x, 0, line_x, nativeHeight(), 1, false);
                } else {
                    // Minor lines at other multiples
                    canvas.setColor(line_color.withAlpha(0.2f));
                    canvas.segment(line_x, 0, line_x, nativeHeight(), 1, false);
                }
            }
        }

        canvas.setLogicalPixelScale();
    }

    void drawDbGrid(Canvas& canvas) {
        canvas.setNativePixelScale();

        canvas.setColor(Color(0xffffff).withAlpha(0.2f));

        // Draw lines for each 10 dB step
        const auto first_tick_dB = static_cast<int>(std::ceil(min_dB_ / 10.0f)) * 10;
        for (int dB = first_tick_dB; dB <= max_dB_; dB += 10) {
            // Calculate vertical position (convert from dB to y coordinate)
            float y_0to1 = tb::to0to1(static_cast<float>(dB), min_dB_, max_dB_);
            float line_y = height() - (y_0to1 * height());

            const auto y = std::round(line_y * canvas.dpiScale());

            // Draw the horizontal line
            canvas.segment(0, y, nativeWidth(), y, 1, false);
        }

        canvas.setLogicalPixelScale();
    }

    float min_freq_ = 15.0f;
    float max_freq_ = 22'000.0f;

    float min_dB_ = -100.0f;
    float max_dB_ = 0.0f;

    VISAGE_LEAK_CHECKER(GridFrame)
};
