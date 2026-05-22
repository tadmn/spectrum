
#pragma once

#include "Common.h"
#include "PopupTextEditor.h"

class TextSlider : public Frame {
  public:
    TextSlider() {
        text_.setJustification(Font::Justification::kCenter);
        setFont({ 11, resources::fonts::DroidSansMono_ttf });
    }

    void setText(const String& text) {
        text_.setText(text);
        redraw();
    }

    void setFont(const Font& font) {
        text_.setFont(font);
        redraw();
    }

    void setJustification(Font::Justification justification) {
        text_.setJustification(justification);
    }

    auto& onSliderDrag() { return on_slider_drag_; }
    auto& onSliderDragBegin() { return on_slider_drag_begin_; }
    auto& onSliderDragEnd() { return on_slider_drag_end_; }
    auto& onTextEnter() { return on_text_enter_; }

    void draw(Canvas& canvas) override {
        // Background
        canvas.setColor(0xff1e1e1e);
        canvas.roundedRectangle(0, 0, width(), height(), 5);

        // Text
        canvas.setColor(0xffffffff);
        canvas.text(&text_, 0, 0, width(), height());
    }

    void mouseUp(const MouseEvent& e) override {
        if (dragging_) {
            dragging_ = false;
            on_slider_drag_end_.callback();
        } else {
            if (e.repeatClickCount() >= 2)
                showEditor();
        }
    }

    void mouseDrag(const MouseEvent& e) override {
        if (! dragging_) {
            dragging_ = true;
            on_slider_drag_begin_.callback();
        }

        auto delta_y = e.relativePosition().y;
        if (e.isShiftDown())
            delta_y *= 0.1f;

        on_slider_drag_.callback(-1.0f * delta_y);
    }

    void showEditor() {
        text_editor_open_ = true;
        PopupTextEditor::show(
            *this, localBounds(), text_,
            [this](const String& text) { on_text_enter_.callback(text); },
            [this] { text_editor_open_ = false; });
    }

    bool textEditorOpen() const { return text_editor_open_; }

  private:
    CallbackList<void(float)> on_slider_drag_;
    CallbackList<void()> on_slider_drag_begin_;
    CallbackList<void()> on_slider_drag_end_;
    CallbackList<void(const String&)> on_text_enter_;
    Text text_;
    bool dragging_ = false;
    bool text_editor_open_ = false;

    VISAGE_LEAK_CHECKER(TextSlider)
};