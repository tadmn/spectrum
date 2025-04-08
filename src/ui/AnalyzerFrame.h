
#pragma once

#include "../AnalyzerProcessor.h"

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

        // Tether the first and last points to bottom left and bottom right corners
        mLine->setBounds(localBounds());
        mLine->setXAt(0, 0);
        mLine->setYAt(0, mLine->height());
        mLine->setXAt(mLine->numPoints() - 1, mLine->width());
        mLine->setYAt(mLine->numPoints() - 1, mLine->height());
    }

    void draw(visage::Canvas& canvas) override {
        mAnalyzerProcessor.process(canvas.deltaTime());

        const auto& line = mAnalyzerProcessor.spectrumLine();
        for (int i = 0; i < line.size(); ++i) {
            mLine->setXAt(i + 1, line[i].x * width());
            mLine->setYAt(i + 1, line[i].y * height());
        }

        redraw();
    }

    void updateLine() {
        const auto numPoints = mAnalyzerProcessor.spectrumLine().size() + 2;
        if (mLine != nullptr && mLine->numPoints() == numPoints)
            return;

        mLine = std::make_unique<visage::GraphLine>(static_cast<int>(numPoints));
        mLine->setPalette(&mPalette);
        mLine->setFill(true);
        mLine->setFillCenter(visage::GraphLine::kBottom);
        mLine->setBounds(0, 0, width(), height());
        addChild(*mLine);

        resized();
    }

  private:
    AnalyzerProcessor& mAnalyzerProcessor;

    visage::Palette mPalette;
    std::unique_ptr<visage::GraphLine> mLine;
};