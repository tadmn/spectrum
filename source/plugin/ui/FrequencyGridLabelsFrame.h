
#pragma once

#include <tb_Math.h>

#include "common/Common.h"
#include "embedded/Fonts.h"

class FrequencyGridLabelsFrame : public Frame {
  public:
    FrequencyGridLabelsFrame() {
        setIgnoresMouseEvents(true, true);
    }

    void setFrequencyRange(float min_freq, float max_freq) {
        tb_assert(min_freq > 0.0f && max_freq > min_freq);
        min_freq_ = min_freq;
        max_freq_ = max_freq;
        redraw();
    }

    void draw(Canvas& canvas) override {
        constexpr std::array<std::pair<float, const char*>, 10> freq_labels { { { 20.0f, "20" },
                                                                                { 50.0f, "50" },
                                                                                { 100.0f, "100" },
                                                                                { 200.0f, "200" },
                                                                                { 500.0f, "500" },
                                                                                { 1000.0f, "1k" },
                                                                                { 2000.0f, "2k" },
                                                                                { 5000.0f, "5k" },
                                                                                { 10000.0f, "10k" },
                                                                                { 20000.0f,
                                                                                  "20k" } } };

        canvas.setColor(Color(0xffffff).withAlpha(0.4));

        const Font label_font(11.0f, resources::fonts::DroidSansMono_ttf);

        const auto min_log_freq = std::log10(min_freq_);
        const auto max_log_freq = std::log10(max_freq_);

        // Draw labels for frequencies that are in the visible range
        for (const auto& freq_label : freq_labels) {
            if (freq_label.first >= min_freq_ && freq_label.first <= max_freq_) {
                float label_x = width() * tb::to0to1(std::log10(freq_label.first), min_log_freq, max_log_freq);

                const auto w = 30;

                // Draw label
                canvas.text(freq_label.second, label_font, Font::Justification::kCenter,
                            label_x - w / 2, 0, w, height());
            }
        }
    }

  private:
    float min_freq_ = 15.0f;
    float max_freq_ = 22'000.0f;

    VISAGE_LEAK_CHECKER(FrequencyGridLabelsFrame)
};