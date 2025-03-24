
#pragma once

#include "AnalyzerFrame.h"
#include "SettingsFrame.h"
#include "embedded/Icons.h"

#include <visage/widgets.h>

class MainFrame : public visage::Frame {
  public:
    MainFrame(AnalyzerProcessor& p) :
        mAnalyzer(p), mButton(resources::icons::settings_svg.data, resources::icons::settings_svg.size),
        mSettings(p) {
        mSettings.setAlphaTransparency(0.75);
        mPalette.initWithDefaults();
        setPalette(&mPalette);

        addChild(mAnalyzer);

        mButton.onToggle() = [this](visage::Button*, bool on) {
            mSettings.setVisible(on);
            resized();
        };

        addChild(mSettings, false);
        addChild(mButton);
    }

    ~MainFrame() override { }

    void resized() override {
        mButton.setBounds(15, 15, 65, 65);
        mSettings.setBounds(mButton.right() + 15, 0, width() - mButton.width() - 15, 100);
        mAnalyzer.setBounds(0, 0, width(), height());
    }

  private:
    visage::Palette mPalette;

    AnalyzerFrame mAnalyzer;
    visage::ToggleIconButton mButton;

    SettingsFrame mSettings;
};