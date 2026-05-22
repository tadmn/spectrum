
#pragma once

#include "Common.h"

class PopupTextEditor {
public:
    static void show(Frame& parent, Bounds bounds, const Text& text,
                     std::function<void(const String&)> on_enter_key, std::function<void()> on_close) {
        auto frame = std::make_unique<PopupTextEditorFrame>();
        PopupTextEditorFrame* frame_ptr = frame.get();
        frame_ptr->show(std::move(frame), parent, bounds, text, on_enter_key, on_close);
    }

private:
    class PopupTextEditorFrame : public Frame {
    public:
        PopupTextEditorFrame() {
            setIgnoresMouseEvents(true, true);
            addChild(editor_, false);

            palette_.setColor(TextEditor::TextEditorBackground, 0xffffffff);
            palette_.setColor(TextEditor::TextEditorText, 0xff000000);
            palette_.setColor(TextEditor::TextEditorSelection, 0xffb3d7ff);
            palette_.setColor(TextEditor::TextEditorCaret, 0xff000000);

            palette_.setValue(TextEditor::TextEditorRounding, 2.0f);
            palette_.setValue(TextEditor::TextEditorMarginY, 1.0f);
            palette_.setValue(TextEditor::TextEditorMarginX, 1.0f);
        }

        void show(std::unique_ptr<PopupTextEditorFrame> self, Frame& parent, Bounds bounds,
                  const Text& text, std::function<void(const String&)> on_enter_key,
                  std::function<void()> on_close) {
            parent.addChild(std::move(self));

            setOnTop(true);
            setBounds(parent.localBounds());

            editor_.setPalette(&palette_);
            editor_.setBounds(bounds);
            editor_.setText(text.text());
            editor_.setJustification(text.justification());
            editor_.setFont(text.font().withSize(text.font().size() + 1));
            editor_.requestKeyboardFocus();
            editor_.selectAll();
            editor_.setVisible(true);

            editor_.onEnterKey() = [on_enter_key, this] {
                on_enter_key(editor_.text());
                close();
            };

            editor_.onEscapeKey() = [this] { close(); };

            editor_.onFocusChange() = [this](bool focused, bool) {
                if (! focused)
                    close();
            };

            timer_.onTimerCallback() = [this, on_close, &parent] {
                if (on_close)
                    on_close();

                parent.removeChild(this); // This will delete ourselves
            };
        }

        void close() { timer_.startTimer(1); }

        void hierarchyChanged() override {
            if (parent() == nullptr)
                close();
        }

        void focusChanged(bool is_focused, bool /*was_clicked*/) override {
            if (! is_focused)
                close();
        }

      private:
        Palette palette_;
        TextEditor editor_;
        EventTimer timer_;

        VISAGE_LEAK_CHECKER(PopupTextEditorFrame)
    };
};