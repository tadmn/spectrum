
#pragma once

#include "common/Common.h"
#include "common/Shelf.h"
#include "embedded/Fonts.h"

#include "RangeFrame.h"
#include "ResolutionFrame.h"
#include "SmoothingFrame.h"
#include "TiltFrame.h"

class PanelButton : public Frame {
  public:
    PanelButton(const std::string text) : text_(text) {
        hover_.setTargetValue(1.0f);
    }

    void setHighlighted(bool highlighted) {
        hover_.target(highlighted);
        redraw();
    }

    void draw(Canvas& canvas) override {
        const auto hoverAmount = hover_.update();

        canvas.setColor(Color(0x454545).withAlpha(hoverAmount));
        canvas.bottomRoundedRectangle(0, 0, width(), height() - 2, 5);

        canvas.setColor(Color(0xffffff).withAlpha(1.0f));
        canvas.text(text_, { 11, resources::fonts::NotoSans_Regular_ttf }, Font::kCenter,
                    0, 0, width(), height());

        if (hover_.isAnimating())
            redraw();
    }

  private:
    std::string text_;
    Animation<float> hover_;

    VISAGE_LEAK_CHECKER(PanelButton)
};

class ParameterPanel : public Frame {
  public:
    ParameterPanel(State& state) : state_(state),
        resolution_button_("Resolution"), range_button_("Range"), tilt_button_("Tilt"), smoothing_button_("Smoothing"),
        resolution_frame_(state), range_frame_(state), tilt_frame_(state), smoothing_frame_(state) {
        addChild(shelf_);
        addChild(resolution_button_);
        addChild(range_button_);
        addChild(tilt_button_);
        addChild(smoothing_button_);

        addChild(resolution_frame_, false);
        addChild(range_frame_, false);
        addChild(tilt_frame_, false);
        addChild(smoothing_frame_, false);

        {
            auto handler = [this](const MouseEvent& e) { handleMouse(e, false); };
            resolution_button_.onMouseEnter() += handler;
            resolution_frame_.onMouseEnter() += handler;
            range_button_.onMouseEnter() += handler;
            range_frame_.onMouseEnter() += handler;
            tilt_button_.onMouseEnter() += handler;
            tilt_frame_.onMouseEnter() += handler;
            smoothing_button_.onMouseEnter() += handler;
            smoothing_frame_.onMouseEnter() += handler;
            onMouseEnter() += handler;
            onMouseMove() += handler;
        }

        {
            auto handler = [this](const MouseEvent& e) { handleMouse(e, true); };
            resolution_button_.onMouseExit() += handler;
            range_button_.onMouseExit() += handler;
            tilt_button_.onMouseExit() += handler;
            smoothing_button_.onMouseExit() += handler;
            onMouseExit() += handler;
        }

        onMouseUp() += [this](const MouseEvent& e) {
            if (e.shouldTriggerPopup())
                showRightClickMenu(e.position);
        };

        state_listener_ = state.addListener([this] { stateChanged(); });
        stateChanged();
    }

    void resized() override {
        auto b = localBounds();

        {
            const auto h = 30;
            auto b1 = b.trimBottom(h);

            {
                const auto w = 460;
                shelf_.setBounds(b1.xCenter() - w / 2, b1.y(), w, b1.height());
            }

            {
                auto b2 = shelf_.bounds();
                b2.trimTop(2);
                const auto trim = 55;
                b2.trimLeft(trim);
                b2.trimRight(trim);
                const auto spacing = 20;
                const auto w = (b2.width() - spacing * 3) / 4;
                resolution_button_.setBounds(b2.trimLeft(w));
                b2.trimLeft(spacing);
                range_button_.setBounds(b2.trimLeft(w));
                b2.trimLeft(spacing);
                tilt_button_.setBounds(b2.trimLeft(w));
                b2.trimLeft(spacing);
                smoothing_button_.setBounds(b2.trimLeft(w));
            }
        }

        {
            auto center_frame_above_button = [&](auto& frame, auto& button, int w, int h) {
                const auto gap = 2;
                h += gap;
                frame.setBounds(button.bounds().xCenter() - w / 2, shelf_.y() - h, w, h);
            };

            center_frame_above_button(resolution_frame_, resolution_button_, 116, 96);
            center_frame_above_button(range_frame_, range_button_, 92, 88);
            center_frame_above_button(tilt_frame_, tilt_button_, 116, 64);
            center_frame_above_button(smoothing_frame_, smoothing_button_, 116, 96);
        }
    }

  private:
    void handleMouse(const MouseEvent& e, bool is_exiting) {
        auto show_only = [this](Frame* frame_to_show, Frame* button_to_highlight) {
            timer_.stopTimer();
            
            resolution_frame_.makeVisible(&resolution_frame_ == frame_to_show);
            range_frame_.makeVisible(&range_frame_ == frame_to_show);
            tilt_frame_.makeVisible(&tilt_frame_ == frame_to_show);
            smoothing_frame_.makeVisible(&smoothing_frame_ == frame_to_show);

            resolution_button_.setHighlighted(&resolution_button_ == button_to_highlight);
            range_button_.setHighlighted(&range_button_ == button_to_highlight);
            tilt_button_.setHighlighted(&tilt_button_ == button_to_highlight);
            smoothing_button_.setHighlighted(&smoothing_button_ == button_to_highlight);
        };

        auto start_hide_timer = [this, show_only] {
            timer_.onTimerCallback() = [this, show_only] {
                if (! resolution_frame_.textEditorOpen() && ! range_frame_.textEditorOpen() &&
                    ! tilt_frame_.textEditorOpen() && ! smoothing_frame_.textEditorOpen()) {
                    show_only(nullptr, nullptr); // Hide all frames
                    }
            };

            timer_.startTimer(200);
        };

        if (is_exiting)
            start_hide_timer();
        else {
            if (e.event_frame == &resolution_button_ || e.event_frame == &resolution_frame_) {
                show_only(&resolution_frame_, &resolution_button_);
            } else if (e.event_frame == &range_button_ || e.event_frame == &range_frame_) {
                show_only(&range_frame_, &range_button_);
            } else if (e.event_frame == &tilt_button_ || e.event_frame == &tilt_frame_) {
                show_only(&tilt_frame_, &tilt_button_);
            } else if (e.event_frame == &smoothing_button_ || e.event_frame == &smoothing_frame_) {
                show_only(&smoothing_frame_, &smoothing_button_);
            } else {
                start_hide_timer();
            }
        }
    }

    void showRightClickMenu(const Point& position) {
        PopupMenu menu;
        menu.addOption(0, "Reset to default parameters");
        menu.addOption(1, state_.hide_controls() ? "Show controls" : "Hide controls");
        menu.onSelection() = [this](int id) {
            if (id == 0) {
                state_.resetToDefaults();
            } else if (id == 1) {
                state_.setHideControls(! state_.hide_controls());
            }
        };
        menu.show(this, position);
    }

    void stateChanged() {
        for (auto child : children())
            child->setVisible(! state_.hide_controls());

        resized();
    }

    State& state_;

    Shelf shelf_;
    PanelButton resolution_button_, range_button_, tilt_button_, smoothing_button_;
    ResolutionFrame resolution_frame_;
    RangeFrame range_frame_;
    TiltFrame tilt_frame_;
    SmoothingFrame smoothing_frame_;
    EventTimer timer_;

    std::unique_ptr<State::Listener> state_listener_;

    VISAGE_LEAK_CHECKER(ParameterPanel)
};