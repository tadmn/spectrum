
#pragma once

#include "AnalyzerFrame.h"
#include "common/Common.h"
#include "common/Palette.h"
#include "DbGridLabelsFrame.h"
#include "FrequencyGridLabelsFrame.h"
#include "GridFrame.h"
#include "OverlayFrame.h"
#include "ParametersFrame.h"

class MainFrame : public Frame {
  public:
    MainFrame(State& state, AnalyzerProcessor& analyzerProcessor) :
        state_(state), analyzer_(analyzerProcessor), parameter_panel_(state) {
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
    TopFrame(State& state, AnalyzerProcessor& p) : state_(state), main_frame_(state, p) {
        setPalette(&palette_);

        addChild(main_frame_);
        addChild(logo_button_);
        addChild(overlay_, false);

        logo_button_.onToggle() = [this](Button*, bool /*on*/) { overlay_.setVisible(true); };

        overlay_.onAnimate() = [this](float overlayAmount) {
            constexpr float kBlurSize = 30.0f;
            main_frame_.setBlurRadius(kBlurSize * overlayAmount);
            logo_button_.setAlphaTransparency(1.0f - overlayAmount);
        };

        state_listener_ = state.addListener([this] { stateChanged(); });
        stateChanged();
    }

    void stateChanged() {
        main_frame_.setDbRange(state_.min_dB(), state_.max_dB());
        logo_button_.setVisible(! state_.hide_controls());
    }

    void draw(Canvas& canvas) override {
        canvas.setColor(0xff000000);
        canvas.fill();
    }

    void resized() override {
        logo_button_.setBounds(24, 16, 153, 27);
        overlay_.setBounds(localBounds());
        main_frame_.setBounds(localBounds());
    }

  private:
    State& state_;

    spectrum::Palette palette_;

    MainFrame main_frame_;

    overlay::LogoButton logo_button_;
    overlay::OverlayFrame overlay_;

    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(TopFrame)
};