
#pragma once

#include "AnalyzerFrame.h"
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
        mAnalyzer(p), mButton(resources::icons::settings_svg.data, resources::icons::settings_svg.size) {
        mPalette.initWithDefaults();
        setPalette(&mPalette);

        addChild(mAnalyzer);

        mButton.onToggle() = [this](visage::Button*, bool) {
            if (mPaletteColorWindow == nullptr) {
                mPaletteColorWindow = std::make_unique<PaletteColorWindow>(mPalette);
            }

            mPaletteColorWindow->show(500, 500);
        };

        addChild(mButton);
    }

    ~MainFrame() override { }

    void resized() override {
        mButton.setBounds(15, 15, 65, 65);
        mAnalyzer.setBounds(bounds());
    }

  private:
    visage::Palette mPalette;

    AnalyzerFrame mAnalyzer;
    visage::ToggleIconButton mButton;

    std::unique_ptr<PaletteColorWindow> mPaletteColorWindow;
};