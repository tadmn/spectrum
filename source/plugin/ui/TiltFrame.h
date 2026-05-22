
#pragma once

#include "common/FadeFrame.h"
#include "common/TextSlider.h"

class TiltFrame : public FadeFrame {
public:
    TiltFrame(State& plugin_state) : state_(plugin_state) {
        addChild(slope_slider_);
        addChild(center_freq_slider_);

        slope_slider_.onTextEnter() += [this](const String& text) {
            state_.setWeightingDbPerOctave(text.withPrecision(1).toFloat());
        };

        center_freq_slider_.onTextEnter() += [this](const String& text) {
            state_.setWeightingCenterFrequency(text.toInt());
        };

        slope_slider_.onSliderDrag() += [this](float delta) {
            state_.setWeightingDbPerOctave(state_.weighting_db_per_octave() + delta * 0.02f);
        };

        center_freq_slider_.onSliderDrag() += [this](float delta) {
            const auto log = std::log2(state_.weighting_center_frequency()) + delta * 0.02f;
            state_.setWeightingCenterFrequency(std::lround(std::pow(2.0f, log)));
        };

        state_listener_ = state_.addListener([this]{ stateChanged(); });
        stateChanged();
    }

    void resized() override {
        slope_slider_.setBounds(10, 8, 31, 19);
        center_freq_slider_.setBounds(54, 35, 54, 19);
    }

    void drawBackground(Canvas& canvas, float /*hover_amount*/) override {
        const auto gap = 2;

        const auto w = width();
        const auto h = height() - gap;

        // Background
        {
            const auto rounding = 10;

            canvas.setColor(0xff454545);
            canvas.roundedRectangle(0, 0, w, h, rounding);

            canvas.setBrush(Brush::linear(0xff323232, 0xff282828, { 0, 0 }, { w, h }));
            canvas.roundedRectangle(1, 1, w - 2, h - 2, rounding);
        }

        // Text
        {
            canvas.setColor(0xff9a9a9a);
            const Font font(11, resources::fonts::NotoSans_Regular_ttf);
            canvas.text("dB / Octave", font, Font::kCenter, 44, 8, 68, 19);
            canvas.text("Fc • Hz", font, Font::kCenter, 7, 37, 43, 15);
        }
    }

    bool textEditorOpen() const {
        return slope_slider_.textEditorOpen() || center_freq_slider_.textEditorOpen();
    }

  private:
    void stateChanged() {
        slope_slider_.setText({ state_.weighting_db_per_octave(), 1 });
        center_freq_slider_.setText(std::lround(state_.weighting_center_frequency()));
    }

    State& state_;
    TextSlider slope_slider_, center_freq_slider_;
    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(TiltFrame)
};
