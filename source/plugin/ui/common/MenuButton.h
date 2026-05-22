
#pragma once

#include "Common.h"

class MenuButton : public Button {
public:
    MenuButton() = default;

    void setText(const std::string& text) {
        text_ = text;
        redraw();
    }

    void draw(Canvas& canvas, float hoverAmount) override {
        canvas.setColor(Color(0xff5d5d5d).interpolateWith(0xff8c8c8c, hoverAmount));
        canvas.roundedRectangleBorder(0, 0, width(), height(), 6, 1);

        canvas.setColor(0xffffffff);
        canvas.text(text_, { 11, resources::fonts::NotoSans_Regular_ttf }, Font::kCenter,
                    0, 0, width(), height());
    }

private:
    std::string text_;

    VISAGE_LEAK_CHECKER(MenuButton)
};
