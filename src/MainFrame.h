
#pragma once

#include "AnalyzerFrame.h"
#include "SettingsFrame.h"
#include "AnalyzerGridFrame.h"
#include "embedded/Icons.h"

#include <visage/widgets.h>

class MainFrame : public visage::Frame {
  public:
    MainFrame(AnalyzerProcessor& p) :
    mAnalyzerProcessor(p),
        mAnalyzer(p), mButton(resources::icons::settings_svg.data, resources::icons::settings_svg.size),
        mSettings(p) {

        mGrid.setAlphaTransparency(0.35);
        mSettings.setAlphaTransparency(0.66);

        addChild(mGrid);
        addChild(mAnalyzer);

        mButton.onToggle() = [this](visage::Button*, bool on) {
            mSettings.setVisible(on);
            resized();
        };

        addChild(mSettings, false);
        addChild(mButton);

        assert(mAnalyzerProcessor.onBandsChanged == nullptr);
        mAnalyzerProcessor.onBandsChanged = [this] {
            mGrid.setFrequencyRange(mAnalyzerProcessor.minFrequency(), mAnalyzerProcessor.maxFrequency());
            mAnalyzer.updateLine();
        };
    }

    ~MainFrame() override {
        mAnalyzerProcessor.onBandsChanged = nullptr;
    }

    void resized() override {
        auto b = localBounds();
        mGrid.setBounds(b);
        mAnalyzer.setBounds(b);

        b = b.trimTop(54);
        mButton.setBounds(b.trimLeft(40).reduced(4));
        mSettings.setBounds(b.reduced(0, 3, 3, 3));
    }

  private:
    AnalyzerProcessor& mAnalyzerProcessor;

    AnalyzerGridFrame mGrid;
    AnalyzerFrameWithGradientFade mAnalyzer;

    visage::ToggleIconButton mButton;
    SettingsFrame mSettings;
};