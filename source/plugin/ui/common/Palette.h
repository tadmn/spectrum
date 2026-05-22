
#pragma once

#include "Common.h"

namespace spectrum {

inline Color backgroundColor() { return {0xff171818}; }

class Palette : public visage::Palette {
public:
    Palette() {
        setColor(TextEditor::TextEditorBackground, 0xff1e1e1e);
        setColor(TextEditor::TextEditorText, 0xffffffff);
    }
};

}