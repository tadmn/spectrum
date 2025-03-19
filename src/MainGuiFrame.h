
#pragma once

#include "AnalyzerProcessor.h"
#include "Spectrum.h"

#include <visage/graphics.h>
#include <visage/widgets.h>

class Spectrum::MainGuiFrame : public visage::Frame {
  public:
    MainGuiFrame(Spectrum& p) : mPlugin(p) {
        setIgnoresMouseEvents(true, false);
        mAnalyzerProcessor = std::make_unique<AnalyzerProcessor>(kFftSize, 320, p.sampleRate(), 20,
                                                                 20'000, -100, 15.0, 0.85);
        updateLine();
    }

    ~MainGuiFrame() override { }

    void resized() override {
        mLine->setBounds(0, 0, width(), height());

        // Tether the first and last points to bottom left and bottom right corners
        mLine->setXAt(0, 0);
        mLine->setYAt(0, mLine->height());
        mLine->setXAt(mLine->numPoints() - 1, mLine->width());
        mLine->setYAt(mLine->numPoints() - 1, mLine->height());
    }

    void draw(visage::Canvas& canvas) override {
        FftComplexOutput fftOutput;

        {
            RealtimeObject::ScopedAccess<farbot::ThreadType::nonRealtime> f(mPlugin.mFftComplexOutput);
            fftOutput = *f;
        }

        mAnalyzerProcessor->process(canvas.deltaTime(), fftOutput);

        const auto& bands = mAnalyzerProcessor->bands();
        for (int i = 0; i < bands.size(); ++i) {
            mLine->setXAt(i + 1, bands[i].x0to1 * width());
            mLine->setYAt(i + 1, bands[i].y0to1 * height());
        }

        redraw();
    }

  private:
    void updateLine() {
        const auto numPoints = mAnalyzerProcessor->bands().size() + 2;
        if (mLine != nullptr && mLine->numPoints() == numPoints)
            return;

        mLine = std::make_unique<visage::GraphLine>(numPoints);
        mLine->setBounds(0, 0, width(), height());
        addChild(*mLine);
    }

    Spectrum& mPlugin;
    std::unique_ptr<AnalyzerProcessor> mAnalyzerProcessor;
    std::unique_ptr<visage::GraphLine> mLine;
};
