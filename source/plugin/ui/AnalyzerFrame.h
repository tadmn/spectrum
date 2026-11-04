#pragma once

#include "AnalyzerProcessor.h"

#include "../State.h"

#include "common/Common.h"
#include "embedded/Fonts.h"

#include <algorithm>
#include <array>

class AnalyzerFrame : public Frame {
  public:
    AnalyzerFrame(State& state, AnalyzerProcessors& analyzer_processors) :
        state_(state), analyzer_processors_(analyzer_processors) {
        setIgnoresMouseEvents(true, true);
    }

    void draw(Canvas& canvas) override {
        const auto separate_channels = state_.shows_separate_channels();

        // Only channels that have a style get drawn, which is currently limited to stereo
        const size_t num_lines = separate_channels ? std::min(analyzer_processors_.size(), channel_styles_.size()) : 1;

        // The other lines were idle up until now, so have them split off from the current line
        if (separate_channels && ! was_separate_channels_) {
            for (size_t i = 1; i < num_lines; ++i)
                analyzer_processors_[i]->copyBandLevelsFrom(*analyzer_processors_.front());
        }

        was_separate_channels_ = separate_channels;

        // Less fill with multiple lines, so that each line stays visible through the others
        const auto fill_alpha = num_lines > 1 ? 0.34f : 0.62f;

        for (size_t i = 0; i < num_lines; ++i) {
            auto& analyzer = *analyzer_processors_[i];
            analyzer.processAnalyzer(canvas.deltaTime());

            const auto& style = channel_styles_[i];
            drawLine(canvas, analyzer.spectrumLine(), style.fill_color.withAlpha(fill_alpha), style.line_color);
        }

        if (num_lines > 1)
            drawLegend(canvas, num_lines);

        redraw();
    }

  private:
    struct ChannelStyle {
        const char* label;
        Color fill_color;
        Color line_color;
    };

    void drawLine(Canvas& canvas, const std::vector<tb::Point>& line, Color fill_color, Color line_color) {
        Path path;

        const auto line_thickness = 2;

        path.moveTo(0, height());
        for (const auto i : line)
            path.lineTo(i.x * width(), (1 - i.y)  * height() + (line_thickness + 1));

        path.lineTo(width(), height());

        const auto fade_out_start = 0.54f * height();
        const auto x = width() / 2;

        canvas.setColor(GraphLine::LineFillColor);
        canvas.setBrush(Brush::linear(Gradient(fill_color, fill_color.withAlpha(0)),
                                                { x, fade_out_start }, { x, height() }));
        canvas.fill(path);

        canvas.setColor(GraphLine::LineColor);
        canvas.setBrush(Brush::linear(Gradient(line_color, line_color.withAlpha(0)),
                                                { x, fade_out_start }, { x, height() }));
        canvas.fill(path.stroke(line_thickness));
    }

    void drawLegend(Canvas& canvas, size_t num_lines) {
        const Font font(12, resources::fonts::NotoSans_Bold_ttf);
        const auto w = 16;
        const auto h = 20;
        const auto y = 16;
        auto x = width() / 2 - w * static_cast<int>(num_lines) / 2;

        for (size_t i = 0; i < num_lines; ++i) {
            canvas.setColor(channel_styles_[i].line_color);
            canvas.text(channel_styles_[i].label, font, Font::kCenter, x, y, w, h);
            x += w;
        }
    }

    State& state_;
    AnalyzerProcessors& analyzer_processors_;
    bool was_separate_channels_ = false;

    const std::array<ChannelStyle, 2> channel_styles_ { {
        { "L", Color(0x358BDB), Color(0xFF63AFF2) },
        { "R", Color(0xDB8435), Color(0xFFF2AE63) },
    } };

    VISAGE_LEAK_CHECKER(AnalyzerFrame)
};
