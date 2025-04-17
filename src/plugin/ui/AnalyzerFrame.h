
#pragma once

#include "AnalyzerProcessor.h"

#include <visage/graphics.h>
#include <visage/widgets.h>

class AnalyzerFrame : public visage::Frame {
  public:
    AnalyzerFrame(AnalyzerProcessor& p) : mAnalyzerProcessor(p) {
        mPalette.initWithDefaults();
        setIgnoresMouseEvents(true, false);
        updateLine();
    }

    ~AnalyzerFrame() override { }

    void resized() override {
        // Set line colors, including gradient fade-out for aesthetics
        {
            const visage::Color lineColor(0xffaa88ff);
            const visage::Color fillColor(0x669f88ff);
            const float fadeOutStart = 0.54 * height();
            const auto x = width() / 2;
            const auto brush = visage::Brush::linear(visage::Gradient(lineColor, lineColor.withAlpha(0)),
                                                     { x, fadeOutStart }, { x, height() });

            mPalette.setColor(visage::GraphLine::LineColor,
                              visage::Brush::linear(visage::Gradient(lineColor, lineColor.withAlpha(0)),
                                                    { x, fadeOutStart }, { x, height() }));

            mPalette.setColor(visage::GraphLine::LineFillColor,
                              visage::Brush::linear(visage::Gradient(fillColor, fillColor.withAlpha(0)),
                                                    { x, fadeOutStart }, { x, height() }));
        }

        mLine->setBounds(localBounds());
    }

    void draw(visage::Canvas& canvas) override {
        mAnalyzerProcessor.processAnalyzer(canvas.deltaTime());

        const auto& line = mAnalyzerProcessor.spectrumLine();

        for (int i = 0; i < line.size(); ++i) {
            mLine->setXAt(i, line[i].x * width());
            mLine->setYAt(i, line[i].y * height());
        }

        redraw();
    }

    void updateLine() {
        const auto numPoints = mAnalyzerProcessor.spectrumLine().size();
        if (mLine != nullptr && mLine->numPoints() == numPoints)
            return;

        mLine = std::make_unique<visage::GraphLine>(static_cast<int>(numPoints));
        mLine->setPalette(&mPalette);
        mLine->setFill(true);
        mLine->setFillCenter(visage::GraphLine::kBottom);
        addChild(*mLine);

        resized();
    }

  private:
    AnalyzerProcessor& mAnalyzerProcessor;

    visage::Palette mPalette;
    std::unique_ptr<visage::GraphLine> mLine;
};