#pragma once

#include "Common.h"

#include "embedded/Fonts.h"

class RadioButton : public Button {
public:
    explicit RadioButton(std::string text) : text_(std::move(text)) { }

    void setSelected(bool selected) {
        selected_ = selected;
        redraw();
    }

    void setEnabled(bool enabled) {
        setActive(enabled);
        redraw();
    }

    void draw(Canvas& canvas, float hoverAmount) override {
        const auto diameter = 12;
        const auto circle_y = (height() - diameter) / 2;

        canvas.setColor(isActive() ? Color(0xff5d5d5d).interpolateWith(0xff8c8c8c, hoverAmount) : Color(0xff454545));
        canvas.ring(0, circle_y, diameter, 1);

        if (selected_) {
            const auto dot_diameter = 6;
            const auto inset = (diameter - dot_diameter) / 2;
            canvas.setColor(0xff63aff2);
            canvas.circle(inset, circle_y + inset, dot_diameter);
        }

        const auto text_color = selected_ ? Color(0xffffffff) : Color(0xff9a9a9a).interpolateWith(0xffffffff, hoverAmount);
        canvas.setColor(isActive() ? text_color : Color(0xff5d5d5d));

        const auto text_x = diameter + 7;
        canvas.text(text_, { 11, resources::fonts::NotoSans_Regular_ttf }, Font::kLeft,
                    text_x, 0, width() - text_x, height());
    }

private:
    std::string text_;
    bool selected_ = false;

    VISAGE_LEAK_CHECKER(RadioButton)
};
