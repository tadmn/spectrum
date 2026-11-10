
#pragma once

#include "Common.h"

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#include <unistd.h>
#endif

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
        std::string url = "https://";
        url += url_text_.text().toUtf8();

#if defined(__APPLE__)
        if (fork() == 0) {
            // Child process
            execl("/usr/bin/open", "open", url.c_str(), nullptr);
            _exit(1);
        }
#elif defined(__linux__)
        if (fork() == 0) {
            // Child process
            execlp("xdg-open", "xdg-open", url.c_str(), nullptr);
            _exit(1);
        }
#elif defined(_WIN32)
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__EMSCRIPTEN__)
        // Opens the link in a new browser tab, so the analyzer page stays open. 'noopener' keeps the new
        // page from getting a handle back to this one through window.opener
        EM_ASM({ window.open(UTF8ToString($0), '_blank', 'noopener'); }, url.c_str());
#endif
    }

    Text url_text_;

    VISAGE_LEAK_CHECKER(UrlFrame)
};