
#pragma once

#include "Common.h"

class UrlFrame : public Button {
public:
    UrlFrame() {
        url_text_.setFont({ 12, resources::fonts::NotoSans_Regular_ttf });
        url_text_.setJustification(Font::Justification::kCenter);
        url_text_.setMultiLine(false);

        onToggle() += [this](Button*, bool){ openUrlInBrowser(); };
    }

    void setUrl(const String& url) {
        url_text_.setText(url);
        redraw();
    }

    void draw(Canvas& canvas, float /*hoverAmount*/) override {
        canvas.setColor(0xff7AC1FF);
        canvas.text(&url_text_, 0, 0, width(), height());
    }

  private:
    void openUrlInBrowser() const {
        std::string command =
#if defined(__APPLE__)
        "open";
#elif defined(_WIN32)
        "start";
#elif defined(__linux__)
        "xdg-open";
#endif

        command += " https://";
        command += url_text_.text().toUtf8();

        if (const auto result = std::system(command.c_str()); result != 0)
            std::cerr << "Spectrum: Failed to open URL in browser: " << result << std::endl;
    }

    Text url_text_;

    VISAGE_LEAK_CHECKER(UrlFrame)
};