
#pragma once

#include "AnalyzerFrame.h"
#include "SettingsFrame.h"
#include "embedded/Icons.h"

#include <visage/widgets.h>

struct GradientOverlay : visage::Frame {
    void draw(visage::Canvas& c) override {
        const auto brush = visage::Brush::linear(visage::Gradient(0x00000000, 0xff000000),
                                                 { static_cast<float>(width()) / 2, 0 },
                                                 { static_cast<float>(width() / 2),
                                                   static_cast<float>(height()) });
        c.setColor(brush);
        c.fill(0, 0, width(), height());
    }
};

class MainFrame : public visage::Frame {
  public:
    MainFrame(AnalyzerProcessor& p) :
        mAnalyzer(p), mButton(resources::icons::settings_svg.data, resources::icons::settings_svg.size),
        mSettings(p) {
        mSettings.setAlphaTransparency(0.75);
        mPalette.initWithDefaults();
        setPalette(&mPalette);

        addChild(mAnalyzer);
        addChild(mGradientOverlay);

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

        const auto h = height() / 3.5;
        mGradientOverlay.setBounds(0, height() - h, width(), h);
    }

  private:
    visage::Palette mPalette;

    AnalyzerFrame mAnalyzer;
    GradientOverlay mGradientOverlay;
    visage::ToggleIconButton mButton;

    SettingsFrame mSettings;
};