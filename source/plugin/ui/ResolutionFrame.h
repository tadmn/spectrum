
#pragma once

#include <tb_Windowing.h>

#include "../State.h"

#include "common/FadeFrame.h"
#include "common/MenuButton.h"
#include "common/TextSlider.h"

class ResolutionFrame : public FadeFrame {
public:
    ResolutionFrame(State& state) : state_(state) {
        addChild(bands_slider_);
        addChild(fft_menu_button_);
        addChild(window_menu_button_);

        bands_slider_.onTextEnter() += [this](const String& text) {
            state_.setTargetNumBands(text.toInt());
        };

        bands_slider_.onSliderDrag() += [this](float delta) {
            state_.setTargetNumBands(state_.target_num_bands() + delta);
        };

        fft_menu_button_.onToggle() += [this](Button*, bool){ showFftWindow(); };
        window_menu_button_.onToggle() += [this](Button*, bool){ showWindowMenu(); };

        state_listener_ = state.addListener([this] { handleStateChange(); });
        handleStateChange();
    }

    void resized() override {
        bands_slider_.setBounds(51, 8, 54, 19);
        fft_menu_button_.setBounds(51, 37, 54, 19);
        window_menu_button_.setBounds(8, 66, 100, 19);
    }

    void drawBackground(Canvas& canvas, float /*hoverAmount*/) override {
        const auto gap = 2;

        const auto w = width();
        const auto h = height() - gap;

        // Background
        {
            const auto rounding = 10;

            canvas.setColor(0xff454545);
            canvas.roundedRectangle(0, 0, width(), h, rounding);

            canvas.setBrush(Brush::linear(0xff333333, 0xff232323, { w / 2, 0 }, { w, h }));
            canvas.roundedRectangle(1, 1, width() - 2, h - 2, rounding);
        }

        // Text
        {
            canvas.setColor(0xff9a9a9a);
            const Font font(11, resources::fonts::NotoSans_Regular_ttf);
            canvas.text("Bands", font, Font::kLeft, 9, 10, 34, 15);
            canvas.text("FFT", font, Font::kLeft, 9, 39, 34, 15);
        }
    }

    bool textEditorOpen() const { return bands_slider_.textEditorOpen(); }

  private:
    void handleStateChange() {
        bands_slider_.setText(std::to_string(state_.target_num_bands()));
        fft_menu_button_.setText(std::to_string(state_.fft_size()));

        {
            auto window_name = std::string(magic_enum::enum_name(state_.window_type()));
            if (state_.window_type() == tb::WindowType::BlackmanHarris)
                window_name = "Blackman Harris";

            window_menu_button_.setText(window_name);
        }
    }

    void showFftWindow() {
        PopupMenu menu;
        for (int i = 12; i <= 16; i++) {
            const int fft_size = 1 << i;
            menu.addOption(fft_size, std::to_string(fft_size));
        }

        menu.onSelection() = [this](int id) { state_.setFftSize(id); };
        menu.show(&fft_menu_button_);
    }

    void showWindowMenu() {
        PopupMenu menu;
        for (auto e : magic_enum::enum_entries<tb::WindowType>()) {
            auto option_name = std::string(e.second);
            if (e.first == tb::WindowType::BlackmanHarris)
                option_name = "Blackman Harris"; // Add in the space for aesthetics

            menu.addOption(magic_enum::enum_index(e.first).value(), option_name);
        }


        menu.onSelection() = [this](int id) {
            state_.setWindowType(magic_enum::enum_cast<tb::WindowType>(id).value());
        };

        menu.show(&window_menu_button_);
    }

    State& state_;

    TextSlider bands_slider_;
    MenuButton fft_menu_button_;
    MenuButton window_menu_button_;

    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(ResolutionFrame)
};