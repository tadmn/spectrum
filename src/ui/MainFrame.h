
#pragma once

#include "Common.h"
#include "GridFrame.h"
#include "AnalyzerFrequencyGridLabelsFrame.h"
#include "AnalyzerFrame.h"
#include "SettingsFrame.h"
#include "embedded/Icons.h"

#include <visage/widgets.h>

class MainFrame : public visage::Frame {
  public:
    MainFrame(AnalyzerProcessor& p) :
    mAnalyzerProcessor(p),
        mAnalyzer(p), mButton(resources::icons::settings_svg.data, resources::icons::settings_svg.size),
        mSettings(p) {

        mSettings.setAlphaTransparency(0.66);

        addChild(mGrid);
        addChild(mAnalyzer);
        addChild(mFreqLabels);

        mButton.onToggle() = [this](visage::Button*, bool on) {
            mSettings.setVisible(on);
            resized();
        };

        addChild(mSettings, false);
        addChild(mButton);

        assert(mAnalyzerProcessor.onBandsChanged == nullptr);
        mAnalyzerProcessor.onBandsChanged = [this] {
            mGrid.setFrequencyRange(mAnalyzerProcessor.minFrequency(), mAnalyzerProcessor.maxFrequency());
            mGrid.setDbRange(mAnalyzerProcessor.minDb(), 0.f);//todo
            mFreqLabels.setFrequencyRange(mAnalyzerProcessor.minFrequency(), mAnalyzerProcessor.maxFrequency());
            mAnalyzer.updateLine();
        };
    }

    ~MainFrame() override { mAnalyzerProcessor.onBandsChanged = nullptr; }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(common::backgroundColor());
        canvas.fill(0, 0, width(), height());
    }

    void resized() override {
        auto b = localBounds();
        mGrid.setBounds(b);
        mAnalyzer.setBounds(b);

        {
            auto b1 = b.trimTop(54);
            mButton.setBounds(b1.trimLeft(40).reduced(7));
            mSettings.setBounds(b1.reduced(0, 3, 3, 3));
        }

        mFreqLabels.setBounds(b.trimBottom(25));
    }

  private:
    AnalyzerProcessor& mAnalyzerProcessor;

    GridFrame mGrid;
    AnalyzerFrame mAnalyzer;
    AnalyzerFrequencyGridLabelsFrame mFreqLabels;

    visage::ToggleIconButton mButton;
    SettingsFrame mSettings;
};