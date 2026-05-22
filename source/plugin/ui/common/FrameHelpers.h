
#pragma once

#include "Common.h"

inline void spacedText(Canvas& canvas, const std::string& text, float font_size,
                       float spacing, float x, float y, float h) {
    const Font font(font_size, resources::fonts::NotoSans_Bold_ttf, 1);

    // Draw with spacing in-between each character
    for (char c : text) {
        std::u32string ch(1, static_cast<char32_t>(c));
        const float char_width = font.stringWidth(ch);
        canvas.text(std::string(1, c), font, Font::kCenter, x, y, char_width, h);
        x += char_width + spacing;
    }

}

inline void setFrameRecursive(Frame& f, const std::function<void(Frame&)>& fn) {
    for (auto* c : f.children())
        setFrameRecursive(*c, fn);

    fn(f);
}