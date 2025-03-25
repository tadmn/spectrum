
#pragma once

#include "AnalyzerProcessor.h"

#include <visage/graphics.h>
#include <visage/widgets.h>

class AnalyzerFrame : public visage::Frame {
  public:
    AnalyzerFrame(AnalyzerProcessor& p) : mAnalyzerProcessor(p) {
        setIgnoresMouseEvents(true, false);
        mAnalyzerProcessor.onBandsChanged = [this] {
            updateLine();
            resized();
        };

        updateLine();
    }

    ~AnalyzerFrame() override { }

    void resized() override {
        mLine->setBounds(bounds());

        // Tether the first and last points to bottom left and bottom right corners
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

  private:
    void updateLine() {
        const auto numPoints = mAnalyzerProcessor.spectrumLine().size() + 2;
        if (mLine != nullptr && mLine->numPoints() == numPoints)
            return;

        mLine = std::make_unique<visage::GraphLine>(numPoints);
        mLine->setFill(true);
        mLine->setFillCenter(visage::GraphLine::kBottom);
        mLine->setBounds(0, 0, width(), height());
        addChild(*mLine);
    }

    AnalyzerProcessor& mAnalyzerProcessor;

    std::unique_ptr<visage::GraphLine> mLine;
};
