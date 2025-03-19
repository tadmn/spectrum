
#pragma once

#include "../cmake-build-debug/_deps/bgfx-src/bgfx/include/bgfx/bgfx.h"
#include "AnalyzerProcessor.h"
#include "SpectrumPlugin.h"

#include <visage/graphics.h>
#include <visage/widgets.h>

class AnalyzerFrame : public visage::Frame {
  public:
    AnalyzerFrame(AnalyzerProcessor& p) : mAnalyzerProcessor(p) {
        setIgnoresMouseEvents(true, false);
        mAnalyzerProcessor.onSettingsChanged = [this] {
            updateLine();
            resized();
        };

        updateLine();
    }

    ~AnalyzerFrame() override { }

    void resized() override {
        mLine->setBounds(0, 0, width(), height());

        // Tether the first and last points to bottom left and bottom right corners
        mLine->setXAt(0, 0);
        mLine->setYAt(0, mLine->height());
        mLine->setXAt(mLine->numPoints() - 1, mLine->width());
        mLine->setYAt(mLine->numPoints() - 1, mLine->height());
    }

    void draw(visage::Canvas& canvas) override {
        mAnalyzerProcessor.process(canvas.deltaTime());

        const auto& bands = mAnalyzerProcessor.bands();
        for (int i = 0; i < bands.size(); ++i) {
            mLine->setXAt(i + 1, bands[i].x0to1 * width());
            mLine->setYAt(i + 1, bands[i].y0to1 * height());
        }

        redraw();
    }

  private:
    void updateLine() {
        const auto numPoints = mAnalyzerProcessor.bands().size() + 2;
        if (mLine != nullptr && mLine->numPoints() == numPoints)
            return;

        mLine = std::make_unique<visage::GraphLine>(numPoints);
        mLine->setBounds(0, 0, width(), height());
        addChild(*mLine);
    }

    AnalyzerProcessor& mAnalyzerProcessor;

    std::unique_ptr<visage::GraphLine> mLine;
};
