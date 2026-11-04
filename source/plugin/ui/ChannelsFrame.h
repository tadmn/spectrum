#pragma once

#include "../State.h"

#include "common/FadeFrame.h"
#include "common/RadioButton.h"

class ChannelsFrame : public FadeFrame {
  public:
    ChannelsFrame(State& state) : state_(state) {
        addChild(summed_button_);
        addChild(separate_button_);

        summed_button_.onToggle() += [this](Button*, bool) { selectMode(ChannelMode::Summed); };
        separate_button_.onToggle() += [this](Button*, bool) { selectMode(ChannelMode::Separate); };

        state_listener_ = state.addListener([this] { stateChanged(); });
        stateChanged();
    }

    void resized() override {
        const auto margin = 12;
        summed_button_.setBounds(margin, 8, width() - 2 * margin, 19);
        separate_button_.setBounds(margin, 29, width() - 2 * margin, 19);
    }

    void drawBackground(Canvas& canvas, float /*hover_amount*/) override {
        const auto gap = 2;

        const auto w = width();
        const auto h = height() - gap;
        const auto rounding = 10;

        canvas.setColor(0xff454545);
        canvas.roundedRectangle(0, 0, w, h, rounding);

        canvas.setBrush(Brush::linear(0xff333333, 0xff232323, { w / 2, 0 }, { w, h }));
        canvas.roundedRectangle(1, 1, w - 2, h - 2, rounding);
    }

  private:
    void selectMode(ChannelMode mode) {
        if (state_.channel_mode() != mode)
            state_.setChannelMode(mode);
    }

    void stateChanged() {
        // A mono instance can only be summed, even if its saved mode is separate
        const auto separate = state_.shows_separate_channels();
        summed_button_.setSelected(! separate);
        separate_button_.setSelected(separate);
        separate_button_.setEnabled(state_.channel_count() > 1);
    }

    State& state_;

    RadioButton summed_button_ { "Summed" };
    RadioButton separate_button_ { "Separate" };

    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(ChannelsFrame)
};
