
#pragma once

#include "Common.h"

namespace spectrum {

inline Color backgroundColor() { return {0xFF171818}; }

class Palette : public visage::Palette {
public:
    Palette() {
        setColor(TextEditor::TextEditorBackground, 0xFF1E1E1E);
        setColor(TextEditor::TextEditorText, 0xFFFFFFFF);

        setColor(PopupMenu::PopupMenuBackground, 0xFF282828);
        setColor(PopupMenu::PopupMenuBorder, 0xFF454545);
        setColor(PopupMenu::PopupMenuSelection, 0xFF277DCE);

        setValue(PopupMenu::PopupMinWidth, 160.0f);
        setValue(PopupMenu::PopupFontSize, 13.0f);
    }
};

}