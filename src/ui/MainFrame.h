
#pragma once

#include "Common.h"
#include "GridFrame.h"
#include "FrequencyGridLabelsFrame.h"
#include "DbGridLabelsFrame.h"
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

        mSettings.setAlphaTransparency(0.5);

        addChild(mGrid);
        addChild(mAnalyzer);
        addChild(mFreqLabels);
        addChild(mDbLabels);

        mButton.onToggle() = [this](visage::Button*, bool on) {
            mSettings.setVisible(on);
            resized();
        };

        addChild(mSettings, false);
        addChild(mButton);

        mAnalyzerProcessor.onBandsChanged = [this] { bandsChanged(); };
        mAnalyzerProcessor.onParametersChanged = [this] { parametersChanged(); };

        // Set initial settings
        bandsChanged();
        parametersChanged();
    }

    ~MainFrame() override {
        mAnalyzerProcessor.onBandsChanged = nullptr;
        mAnalyzerProcessor.onParametersChanged = nullptr;
    }

    void bandsChanged() {
        mGrid.setFrequencyRange(mAnalyzerProcessor.minFrequency(), mAnalyzerProcessor.maxFrequency());
        mFreqLabels.setFrequencyRange(mAnalyzerProcessor.minFrequency(), mAnalyzerProcessor.maxFrequency());
        mAnalyzer.updateLine();
    }

    void parametersChanged() {
        mSettings.updateSettings();
        constexpr auto kMaxDb = 0.f;
        mGrid.setDbRange(mAnalyzerProcessor.minDb(), kMaxDb);
        mDbLabels.setDbRange(mAnalyzerProcessor.minDb(), kMaxDb);
    }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(common::backgroundColor());
        canvas.fill(0, 0, width(), height());
    }

    void resized() override {
        auto b = localBounds();
        mGrid.setBounds(b);
        mAnalyzer.setBounds(b);

        mDbLabels.setBounds(visage::Bounds(b).trimRight(42));

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
    DbGridLabelsFrame mDbLabels;

    visage::ToggleIconButton mButton;
    SettingsFrame mSettings;
};