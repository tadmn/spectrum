
#pragma once

#include "AnalyzerFrame.h"
#include "SettingsFrame.h"
#include "embedded/Fonts.h"
#include "embedded/Icons.h"

#include <visage/widgets.h>

class PaletteColorWindow : public visage::ApplicationWindow {
  public:
    PaletteColorWindow(visage::Palette& palette) {
        setTitle("Spectrum Fill Color");
        const auto id = visage::GraphLine::LineFillColor;
        visage::Brush color;
        if (palette.color({}, id, color))
            mFillColorPicker.setColor(color.gradient().sample(0));

        mFillColorPicker.onColorChange().add([id, &palette](visage::Color c) {
            palette.setColor(id, c);
        });

        addChild(mFillColorPicker);
    }

    void resized() override { mFillColorPicker.setBounds(0, 0, width(), height()); }

  private:
    visage::ColorPicker mFillColorPicker;
};

class MainFrame : public visage::Frame {
  public:
    MainFrame(AnalyzerProcessor& p) :
        mAnalyzer(p), mButton(resources::icons::settings_svg.data, resources::icons::settings_svg.size),
        mSettings(p) {
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
        if (mSettings.isVisible())
            mSettings.setBounds(0, 0, width(), 100);
        else
            mSettings.setBounds(0, 0, 0, 0);

        mButton.setBounds(15, mSettings.bottom() + 15, 65, 65);
        mAnalyzer.setBounds(0, mSettings.bottom(), width(), height() - mSettings.bottom());
    }

  private:
    visage::Palette mPalette;

    AnalyzerFrame mAnalyzer;
    visage::ToggleIconButton mButton;

    SettingsFrame mSettings;
};