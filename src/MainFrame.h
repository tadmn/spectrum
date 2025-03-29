
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
        mSettings.setAlphaTransparency(0.66);
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
        auto b = localBounds();
        mAnalyzer.setBounds(b);
        mGradientOverlay.setBounds(b.copy().trimBottom(0.56 * height()));

        b = b.trimTop(54);
        mButton.setBounds(b.trimLeft(40).reduced(4));
        mSettings.setBounds(b.reduced(0, 3, 3, 3));
    }

  private:
    visage::Palette mPalette;

    AnalyzerFrame mAnalyzer;
    GradientOverlay mGradientOverlay;
    visage::ToggleIconButton mButton;

    SettingsFrame mSettings;
};