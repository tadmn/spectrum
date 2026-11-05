
#pragma once

#include "AnalyzerFrame.h"
#include "common/Common.h"
#include "common/Palette.h"
#include "DbGridLabelsFrame.h"
#include "FrequencyGridLabelsFrame.h"
#include "FullScreenButton.h"
#include "GridFrame.h"
#include "OverlayFrame.h"
#include "ParametersFrame.h"

class MainFrame : public Frame {
  public:
    MainFrame(State& state, AnalyzerProcessors& analyzer_processors) :
        state_(state), analyzer_(state, analyzer_processors), parameter_panel_(state) {
        addChild(grid_);
        addChild(analyzer_);
        addChild(freq_labels_);
        addChild(dB_labels_);
        addChild(parameter_panel_);

        grid_.setFrequencyRange(k_min_frequency, k_max_frequency);
        freq_labels_.setFrequencyRange(k_min_frequency, k_max_frequency);

        state_listener_ = state.addListener([this] { stateChanged(); });
        stateChanged();
    }

    void setDbRange(float min_dB, float max_dB) {
        grid_.setDbRange(min_dB, max_dB);
        dB_labels_.setDbRange(min_dB, max_dB);
    }

    void draw(Canvas& canvas) override {
        canvas.setColor(spectrum::backgroundColor());
        canvas.fill();
    }

    void resized() override {
        auto b = localBounds();

        b.trimTop(1);
        b.trimBottom(2);
        b.trimLeft(1);
        b.trimRight(1);

        parameter_panel_.setBounds(b);

        b.trimBottom(state_.hide_controls() ? 4 : 28);

        grid_.setBounds(b);
        analyzer_.setBounds(b);
        dB_labels_.setBounds(Bounds(b).trimRight(42));
        freq_labels_.setBounds(b.trimBottom(20));
    }

    void stateChanged() {
        resized();
    }

  private:
    State& state_;

    GridFrame grid_;
    AnalyzerFrame analyzer_;
    FrequencyGridLabelsFrame freq_labels_;
    DbGridLabelsFrame dB_labels_;
    ParameterPanel parameter_panel_;

    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(MainFrame)
};

class TopFrame : public Frame {
  public:
    TopFrame(State& state, AnalyzerProcessors& analyzer_processors) :
        state_(state), main_frame_(state, analyzer_processors) {
        setPalette(&palette_);

        addChild(main_frame_);
        addChild(logo_button_);
        addChild(full_screen_button_);
        addChild(overlay_, false);

        logo_button_.onToggle() = [this](Button*, bool /*on*/) { overlay_.setVisible(true); };

        overlay_.onAnimate() = [this](float overlayAmount) {
            constexpr float kBlurSize = 30.0f;
            main_frame_.setBlurRadius(kBlurSize * overlayAmount);
            logo_button_.setAlphaTransparency(1.0f - overlayAmount);
            full_screen_button_.setAlphaTransparency(1.0f - overlayAmount);
        };

        state_listener_ = state.addListener([this] { stateChanged(); });
        stateChanged();
    }

    void stateChanged() {
        main_frame_.setDbRange(state_.min_dB(), state_.max_dB());
        logo_button_.setVisible(content_visible_ && ! state_.hide_controls());
        updateFullScreenButtonVisibility();
    }

    // Leaves only the black background. Drawing the analyzer also advances it, so only one frame can
    // show it at a time.
    void setContentVisible(bool visible) {
        content_visible_ = visible;
        main_frame_.setVisible(visible);
        stateChanged();
        redraw();
    }

    // Tells this frame that it's the one showing in the full screen window
    void setFullScreen(bool full_screen) {
        full_screen_ = full_screen;
        full_screen_button_.setFullScreen(full_screen);
        updateFullScreenButtonVisibility();
    }

    void onFullScreenClicked(std::function<void()> on_clicked) {
        full_screen_button_.onToggle() = [callback = std::move(on_clicked)](Button*, bool) { callback(); };
    }

    void draw(Canvas& canvas) override {
        canvas.setColor(0xff000000);
        canvas.fill();
    }

    void resized() override {
        logo_button_.setBounds(24, 16, 153, 27);

        const auto button_size = 25;
        const auto button_margin = 6;
        full_screen_button_.setBounds(width() - button_size - button_margin, height() - button_size - button_margin,
                                      button_size, button_size);
        updateFullScreenButtonVisibility();

        overlay_.setBounds(localBounds());
        main_frame_.setBounds(localBounds());
    }

  private:
    void updateFullScreenButtonVisibility() {
        // Stays available when controls are hidden, as it's the way back from full screen. It only
        // makes way for the parameter shelf in a narrow window.
        constexpr auto k_min_width_for_button = 520;
        full_screen_button_.setVisible(content_visible_ && (full_screen_ || state_.hide_controls() || width() >= k_min_width_for_button));
    }

    State& state_;

    spectrum::Palette palette_;

    MainFrame main_frame_;

    overlay::LogoButton logo_button_;
    FullScreenButton full_screen_button_;
    overlay::OverlayFrame overlay_;

    bool full_screen_ = false;
    bool content_visible_ = true;

    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(TopFrame)
};