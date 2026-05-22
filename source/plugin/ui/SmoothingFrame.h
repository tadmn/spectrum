
#pragma once

#include "common/FadeFrame.h"
#include "common/TextSlider.h"

class SmoothingFrame : public FadeFrame {
  public:
    SmoothingFrame(State& plugin_state) : state_(plugin_state) {
        addChild(attack_slider_);
        addChild(release_slider_);
        addChild(curve_slider_);

        attack_slider_.onTextEnter() += [this](const String& text) { state_.setAttackRate(text.toFloat()); };
        release_slider_.onTextEnter() += [this](const String& text) { state_.setReleaseRate(text.toFloat()); };
        curve_slider_.onTextEnter() += [this](const String& text) { state_.setLineSmoothingInterpolationSteps(text.toInt()); };

        attack_slider_.onSliderDrag() += [this](float delta) { state_.setAttackRate(state_.attack_rate() + delta * 0.1f); };
        release_slider_.onSliderDrag() += [this](float delta) { state_.setReleaseRate(state_.release_rate() + delta * 0.1f); };

        curve_slider_.onSliderDragBegin() += [this] { curve_slider_value_ = 0.0f; };
        curve_slider_.onSliderDrag() += [this](float delta) {
            curve_slider_value_ += delta;
            constexpr auto step = 17.0f;
            if (std::abs(curve_slider_value_) >= step) {
                const auto quotient = std::trunc(curve_slider_value_ / step);
                state_.setLineSmoothingInterpolationSteps(state_.line_smoothing_interpolation_steps() +
                                                          static_cast<int>(quotient));
                curve_slider_value_ = std::fmod(curve_slider_value_, step);
            }
        };

        state_listener = state_.addListener([this] { stateChanged(); });
        stateChanged();
    }

    void resized() override {
        attack_slider_.setBounds(60, 10, 48, 19);
        release_slider_.setBounds(60, 37, 48, 19);
        curve_slider_.setBounds(60, 71, 48, 19);
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

            canvas.setBrush(Brush::radial(0xff3a3a3a, 0xff292828, {width() / 2, 2 * h / 3}, width() / 2));
            canvas.roundedRectangle(1, 1, width() - 2, h - 2, rounding);
        }

        // Text
        {
            canvas.setColor(0xff9a9a9a);
            const Font font(11, resources::fonts::NotoSans_Regular_ttf);
            canvas.text("Attack", font, Font::kLeft, 9, 10, 51, 19);
            canvas.text("Release", font, Font::kLeft, 9, 37, 51, 19);
            canvas.text("Curve", font, Font::kLeft, 9, 71, 40, 19);
        }

        // Aesthetic Elements
        {
            const auto y = 63;
            canvas.setBrush(Brush::radial(0xff646363, 0x003D3D3D, {width() / 2, y}, 55));
            canvas.rectangle(3, y, 110, 1);
        }
    }

    bool textEditorOpen() const {
        return attack_slider_.textEditorOpen() || release_slider_.textEditorOpen() ||
               curve_slider_.textEditorOpen();
    }

  private:
    void stateChanged() {
        attack_slider_.setText({ state_.attack_rate(), 2 });
        release_slider_.setText({ state_.release_rate(), 2 });
        curve_slider_.setText(state_.line_smoothing_interpolation_steps());
    }

    State& state_;

    TextSlider attack_slider_, release_slider_, curve_slider_;
    float curve_slider_value_ = 0.0f;

    std::unique_ptr<State::Listener> state_listener;

    VISAGE_LEAK_CHECKER(SmoothingFrame)
};
