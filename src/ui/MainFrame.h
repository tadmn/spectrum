
#pragma once

#include "AnalyzerFrame.h"
#include "AnalyzerFrequencyGridFrame.h"
#include "AnalyzerFrequencyGridLabelsFrame.h"
#include "AnalyzerDbGridFame.h"
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

        addChild(mFreqGrid);
        addChild(mDbGrid);
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
            mFreqGrid.setFrequencyRange(mAnalyzerProcessor.minFrequency(), mAnalyzerProcessor.maxFrequency());
            mFreqLabels.setFrequencyRange(mAnalyzerProcessor.minFrequency(), mAnalyzerProcessor.maxFrequency());
            mDbGrid.setDbRange(mAnalyzerProcessor.minDb(), 0.f);//todo
            mAnalyzer.updateLine();
        };
    }

    ~MainFrame() override { mAnalyzerProcessor.onBandsChanged = nullptr; }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(0xff1e1f22);
        canvas.fill(0, 0, width(), height());
    }

    void resized() override {
        auto b = localBounds();
        mFreqGrid.setBounds(b);
        mAnalyzer.setBounds(b);
        mDbGrid.setBounds(b);

        {
            auto b1 = b.trimTop(54);
            mButton.setBounds(b1.trimLeft(40).reduced(4));
            mSettings.setBounds(b1.reduced(0, 3, 3, 3));
        }

        mFreqLabels.setBounds(b.trimBottom(25));
    }

  private:
    AnalyzerProcessor& mAnalyzerProcessor;

    AnalyzerFrequencyGridFrame mFreqGrid;
    AnalyzerFrame mAnalyzer;
    AnalyzerFrequencyGridLabelsFrame mFreqLabels;
    AnalyzerDbGridFame mDbGrid;

    visage::ToggleIconButton mButton;
    SettingsFrame mSettings;
};