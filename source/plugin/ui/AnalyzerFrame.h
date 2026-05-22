
#pragma once

#include "AnalyzerProcessor.h"

#include "common/Common.h"

class AnalyzerFrame : public Frame {
  public:
    AnalyzerFrame(AnalyzerProcessor& p) : analyzer_processor_(p) {
        setIgnoresMouseEvents(true, true);
    }

    void draw(Canvas& canvas) override {
        analyzer_processor_.processAnalyzer(canvas.deltaTime());

        const auto& line = analyzer_processor_.spectrumLine();

        Path path;

        const auto line_thickness = 2;

        path.moveTo(0, height());
        for (const auto i : line)
            path.lineTo(i.x * width(), (1 - i.y)  * height() + (line_thickness + 1));

        path.lineTo(width(), height());

        // Blue
        const auto fill_color = Color(0x358BDB).withAlpha(0.62);
        const auto line_color = Color(0x63AFF2).withAlpha(1.0);

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

        redraw();
    }

  private:
    AnalyzerProcessor& analyzer_processor_;

    VISAGE_LEAK_CHECKER(AnalyzerFrame)
};