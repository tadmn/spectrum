
#pragma once

#include "AnalyzerProcessor.h"

#include <visage/graphics.h>
#include <visage/widgets.h>

class AnalyzerFrame : public visage::Frame {
  public:
    AnalyzerFrame(AnalyzerProcessor& p) : mAnalyzerProcessor(p) {
        setIgnoresMouseEvents(true, false);
        updateLine();
    }

    ~AnalyzerFrame() override { }

    void resized() override {
        mLine->setBounds(localBounds());

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

    void updateLine() {
        const auto numPoints = mAnalyzerProcessor.spectrumLine().size() + 2;
        if (mLine != nullptr && mLine->numPoints() == numPoints)
            return;

        mLine = std::make_unique<visage::GraphLine>(numPoints);
        mLine->setFill(true);
        mLine->setFillCenter(visage::GraphLine::kBottom);
        mLine->setBounds(0, 0, width(), height());
        addChild(*mLine);

        resized();
    }

  private:
    AnalyzerProcessor& mAnalyzerProcessor;

    std::unique_ptr<visage::GraphLine> mLine;
};

class GradientOverlay : public visage::Frame {
public:
    GradientOverlay() {
        setIgnoresMouseEvents(true, false);
    }

    ~GradientOverlay() override {}

    void draw(visage::Canvas& c) override {
        const auto brush = visage::Brush::linear(visage::Gradient(0x00000000, 0xff000000),
                                                 { width() / 2, 0 }, { width() / 2, height() });
        c.setColor(brush);
        c.fill(0, 0, width(), height());
    }
};

class AnalyzerFrameWithGradientFade : public visage::Frame {
public:
    AnalyzerFrameWithGradientFade(AnalyzerProcessor& p) : mAnalyzerFrame(p) {
        addChild(mAnalyzerFrame);
        addChild(mGradientOverlay);
    }

    ~AnalyzerFrameWithGradientFade() override {}

    void resized() override {
        mAnalyzerFrame.setBounds(localBounds());
        mGradientOverlay.setBounds(localBounds().trimBottom(0.54 * height()));
    }

    void updateLine() {
        mAnalyzerFrame.updateLine();
    }

private:
    AnalyzerFrame mAnalyzerFrame;
    GradientOverlay mGradientOverlay;
};
