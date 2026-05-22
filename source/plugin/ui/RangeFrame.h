
#pragma once

#include "common/FadeFrame.h"
#include "common/TextSlider.h"

class RangeFrame : public FadeFrame {
public:
    RangeFrame(State& state) : state_(state) {
        addChild(amp_min_slider_);
        addChild(amp_max_slider_);

        amp_min_slider_.setJustification(Font::kCenter);
        amp_max_slider_.setJustification(Font::kCenter);

        amp_min_slider_.onSliderDrag() += [this](float delta) {
            state_.setMinDb(state_.min_dB() + delta * 0.16f);
        };

        amp_max_slider_.onSliderDrag() += [this](float delta) {
            state_.setMaxDb(state_.max_dB() + delta * 0.16f);
        };

        amp_min_slider_.onTextEnter() += [this](const String& text) { state_.setMinDb(text.toInt()); };
        amp_max_slider_.onTextEnter() += [this](const String& text) { state_.setMaxDb(text.toInt()); };

        state_listener_ = state_.addListener([this] { handleStateChange(); });
        handleStateChange();
    }

    void resized() override {
        amp_max_slider_.setBounds(11, 33, 48, 19);
        amp_min_slider_.setBounds(11, 60, 48, 19);
    }

    void drawBackground(Canvas& canvas, float /*hover_amount*/) override {
        const auto gap = 2;
        const auto h = height() - gap;

        // Background
        {
            const auto rounding = 10;

            canvas.setColor(0xff454545);
            canvas.roundedRectangle(0, 0, width(), h, rounding);

            canvas.setBrush(Brush::radial(0xff3a3a3a, 0xff2e2e2e, { width() / 2, h / 2 }, width() / 2));
            canvas.roundedRectangle(1, 1, width() - 2, h - 2, rounding);
        }

        // Text
        {
            {
                const Font font(11, resources::fonts::NotoSans_Regular_ttf);
                canvas.setColor(0xff9a9a9a);
                canvas.text("Amplitude", font, Font::kCenter, 21, 9, 54, 15);
            }

            {
                const Font font(10, resources::fonts::NotoSans_Regular_ttf);
                canvas.setColor(0xff9a9a9a);
                canvas.text("dB", font, Font::kCenter, 67, 33, 13, 19);
                canvas.text("dB", font, Font::kCenter, 67, 60, 13, 19);
            }
        }
    }

    bool textEditorOpen() const {
        return amp_min_slider_.textEditorOpen() || amp_max_slider_.textEditorOpen();
    }

private:
    void handleStateChange() {
        amp_min_slider_.setText(std::lround(state_.min_dB()));
        amp_max_slider_.setText(std::lround(state_.max_dB()));
    }

    State& state_;
    TextSlider amp_min_slider_, amp_max_slider_;

    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(RangeFrame)
};