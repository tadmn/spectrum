
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

        int i = 0;

        if (mAnalyzerProcessor.lineSmoothingInterpolationSteps() == 0) {
            mLine->setXAt(i, line.front().x * width());
            mLine->setYAt(i, height());
            i++;
        }

        for (int j = 0; j < line.size(); ++j, ++i) {
            mLine->setXAt(i, line[j].x * width());
            mLine->setYAt(i, line[j].y * height());
        }

        if (mAnalyzerProcessor.lineSmoothingInterpolationSteps() == 0) {
            mLine->setXAt(i, line.back().x * width());
            mLine->setYAt(i, height());
        }

        redraw();
    }

    void updateLine() {
        auto numPoints = mAnalyzerProcessor.spectrumLine().size();
        if (mAnalyzerProcessor.lineSmoothingInterpolationSteps() == 0)
            numPoints += 2; // We're going to tether the end points ourselves

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